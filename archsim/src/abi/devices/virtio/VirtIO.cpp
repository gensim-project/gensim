/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/virtio/VirtIO.h"
#include "abi/devices/virtio/VirtQueue.h"
#include "abi/devices/IRQController.h"
#include "abi/EmulationModel.h"
#include "abi/memory/MemoryModel.h"
#include "util/LogContext.h"

#include <string.h>

UseLogContext(LogDevice);
DeclareChildLogContext(LogVirtIO, LogDevice, "VirtIO");

using namespace archsim::abi::devices::virtio;
using archsim::Address;

#define VIRTIO_CHAR_SHIFT(c, s) (((uint32_t)((uint8_t)c)) << (s))
#define VIRTIO_MAGIC (VIRTIO_CHAR_SHIFT('v', 0) | VIRTIO_CHAR_SHIFT('i', 8) | VIRTIO_CHAR_SHIFT('r', 16) | VIRTIO_CHAR_SHIFT('t', 24))

VirtIO::VirtIO(EmulationModel& parent_model, IRQLine& irq, Address base_address, uint32_t size, std::string name, uint32_t version, uint32_t device_id, uint8_t nr_queues)
	: RegisterBackedMemoryComponent(parent_model, base_address, size, name),
	  irq(irq),

	  MagicValue("magic", 0x00, 32, VIRTIO_MAGIC),
	  Version("version", 0x04, 32, version),
	  DeviceID("device-id", 0x08, 32, device_id),
	  VendorID("vendor-id", 0x0c, 32, 0x554d4551),

	  HostFeatures("host-features", 0x10, 32, 0, true, false),
	  HostFeaturesSel("host-features-sel", 0x14, 32, 0, false, true),

	  GuestFeatures("guest-features", 0x20, 32, 0, false, true),
	  GuestFeaturesSel("guest-features-sel", 0x24, 32, 0, false, true),
	  GuestPageSize("guest-page-size", 0x28, 32, 0, false, true),

	  QueueSel("queue-sel", 0x30, 32, 0, false, true),
	  QueueNumMax("queue-num-max", 0x34, 32, 0, true, false),
	  QueueNum("queue-num", 0x38, 32, 0, false, true),
	  QueueAlign("queue-align", 0x3c, 32, 0, false, true),
	  QueuePFN("queue-pfn", 0x40, 32, 0),

	  QueueNotify("queue-notify", 0x50, 32, 0, false, true),
	  InterruptStatus("interrupt-status", 0x60, 32, 0, true, false),
	  InterruptACK("interrupt-ack", 0x64, 32, 0, false, true),
	  Status("status", 0x70, 32, 0)
{
	AddRegister(MagicValue);
	AddRegister(Version);
	AddRegister(DeviceID);
	AddRegister(VendorID);

	AddRegister(HostFeatures);
	AddRegister(HostFeaturesSel);

	AddRegister(GuestFeatures);
	AddRegister(GuestFeaturesSel);
	AddRegister(GuestPageSize);

	AddRegister(QueueSel);
	AddRegister(QueueNumMax);
	AddRegister(QueueNum);
	AddRegister(QueueAlign);
	AddRegister(QueuePFN);

	AddRegister(QueueNotify);
	AddRegister(InterruptStatus);
	AddRegister(InterruptACK);
	AddRegister(Status);

	for (uint8_t i = 0; i < nr_queues; i++)
		queues.push_back(new VirtQueue(i));
}

bool VirtIO::Initialise()
{
	return true;
}


VirtIO::~VirtIO()
{
	for (auto queue : queues) {
		delete queue;
	}
}

bool VirtIO::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	LC_DEBUG3(LogVirtIO) << "[" << GetName() << "] Register Read offset=" << std::hex << offset << ", size=" << std::dec << (uint32_t)size;
	if (offset >= 0x100 && offset <= (0x100 + GetConfigAreaSize())) {
		uint32_t config_area_offset = offset - 0x100;

		LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Device Configuration Space Read offset=" << std::hex << config_area_offset << ", size=" << std::dec << (uint32_t)size;

		if (size > (GetConfigAreaSize() - config_area_offset))
			return false;

		uint8_t *ca = GetConfigArea();
		data = 0;
		memcpy((void *)&data, ca + config_area_offset, size);

		LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Config Data: " << std::hex << data;

		return true;
	} else {
		return RegisterBackedMemoryComponent::Read(offset, size, data);
	}
}

uint32_t VirtIO::ReadRegister(MemoryRegister& reg)
{
	LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Read Register: " << reg.GetName() << " = " << std::hex << reg.Get();

	if (reg == InterruptStatus) {
		return irq.IsAsserted() ? 1 : 0;
	} else if (reg == QueueNumMax) {
		if (GetCurrentQueue()->GetPhysAddr().Get() == 0) {
			return 0x400;
		} else {
			return 0;
		}
	} else if (reg == QueuePFN) {
		return GetCurrentQueue()->GetPhysAddr().Get() >> guest_page_shift;
	}

	return RegisterBackedMemoryComponent::ReadRegister(reg);
}

void VirtIO::WriteRegister(MemoryRegister& reg, uint32_t value)
{
	LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Write Register: " << reg.GetName() << " = " << std::hex << value;

	if (reg == Status) {
		if (value == 0) {
			HostFeaturesSel.Set(0);
			GuestFeaturesSel.Set(0);
			GuestPageSize.Set(0);

			ResetDevice();
		}
	} else if (reg == QueuePFN) {
		if (value == 0) {
			HostFeaturesSel.Set(0);
			GuestFeaturesSel.Set(0);
			GuestPageSize.Set(0);

			ResetDevice();
		} else {
			Address queue_phys_addr = Address(value << guest_page_shift);

			VirtQueue *cq = GetCurrentQueue();

			host_addr_t queue_host_addr;
			if (!parent_model.GetMemoryModel().LockRegion(Address(queue_phys_addr), 0x2000, queue_host_addr))
				assert(false);

			cq->SetBaseAddress(queue_phys_addr, queue_host_addr);
		}
	} else if (reg == QueueAlign) {
		GetCurrentQueue()->SetAlign(value);
	} else if (reg == QueueNum) {
		GetCurrentQueue()->SetSize(value);
	} else if (reg == GuestPageSize) {
		guest_page_shift = __builtin_ctz(value);
		if(guest_page_shift > 31) guest_page_shift = 0;
		LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Setting guest page size=" << std::hex << value << ", shift=" << std::dec << (uint32_t)guest_page_shift;
	} else if (reg == QueueNotify) {
		LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Queue Notify " << std::dec << QueueSel.Get();
		ProcessQueue(GetQueue(value));
		LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Queue Notification Complete, ISR=" << std::hex << InterruptStatus.Get();
	} else if (reg == InterruptACK) {
		InterruptStatus.Set(InterruptStatus.Get() & ~value);
		if (value != 0 && irq.IsAsserted()) {
			LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Rescinding Interrupt";
			irq.Rescind();
		}
	}

	if (InterruptStatus.Get() != 0) {
		if (!irq.IsAsserted()) {
			LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Asserting Interrupt";
			irq.Assert();
		}
	}

	RegisterBackedMemoryComponent::WriteRegister(reg, value);
}

void VirtIO::ProcessQueue(VirtQueue *queue)
{
	LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Processing Queue";

	uint16_t head_idx;
	const VirtRing::VirtRingDesc *descr;
	while ((descr = queue->PopDescriptorChainHead(head_idx)) != NULL) {
		VirtIOQueueEvent *evt = new VirtIOQueueEvent (*queue, head_idx);

		LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Popped a descriptor chain head " << std::dec << head_idx << " " << std::hex << descr->flags;

		bool have_next = false;

		do {
			LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Processing descriptor " << descr->flags << std::hex << " " << descr->paddr << " " << descr->len;
			host_addr_t descr_host_addr;
			if (!parent_model.GetMemoryModel().LockRegion(Address(descr->paddr), descr->len, descr_host_addr))
				assert(false);

			if (descr->flags & 2) {
				LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Adding WRITE buffer @ " << descr_host_addr << ", size = " << descr->len;
				evt->AddWriteBuffer(descr_host_addr, descr->len);
			} else {
				LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Adding READ buffer @ " << descr_host_addr << ", size = " << descr->len;
				evt->AddReadBuffer(descr_host_addr, descr->len);
			}

			have_next = (descr->flags & 1) == 1;
			if (have_next) {
				LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Following descriptor chain to " << (uint32_t)descr->next;
				descr = queue->GetDescriptor(descr->next);
			}
		} while (have_next);

		LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Processing event";
		ProcessEvent(evt);

	}
}
