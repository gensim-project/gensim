/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   VirtIO.h
 * Author: s0457958
 *
 * Created on 16 September 2014, 12:34
 */

#ifndef VIRTIO_H
#define	VIRTIO_H

#include "define.h"
#include "abi/devices/Component.h"
#include "abi/devices/IRQController.h"

#include <vector>
#include <array>

#define VRING_DESC_F_INDIRECT 4

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class IRQLine;

			namespace virtio
			{
				struct VirtIOQueueEventBuffer {
				public:
					VirtIOQueueEventBuffer(host_addr_t data, uint32_t size) : data(data), size(size) { }

					host_addr_t data;
					uint32_t size;
				};

				class VirtQueue;

				class VirtIOQueueEvent
				{
				public:
					VirtIOQueueEvent(VirtQueue &owner, uint32_t index) : response_size(0), owner(owner), _index(index) { }

					~VirtIOQueueEvent()
					{
					}

					inline void AddReadBuffer(host_addr_t data, uint32_t size)
					{
						read_buffers.push_back(VirtIOQueueEventBuffer(data, size));
					}

					inline void AddWriteBuffer(host_addr_t data, uint32_t size)
					{
						write_buffers.push_back(VirtIOQueueEventBuffer(data, size));
					}

					inline void Clear()
					{
						response_size = 0;
						read_buffers.clear();
						write_buffers.clear();
					}

					inline uint32_t Index()
					{
						return _index;
					}

					std::vector<VirtIOQueueEventBuffer> read_buffers;
					std::vector<VirtIOQueueEventBuffer> write_buffers;

					VirtQueue &owner;

					uint32_t response_size;

				private:
					uint32_t _index;
				};

				class VirtIO : public RegisterBackedMemoryComponent
				{
				public:
					VirtIO(EmulationModel& parent_model, IRQLine& irq, Address base_address, uint32_t size, std::string name, uint32_t version, uint32_t device_id, uint8_t nr_queues);
					virtual ~VirtIO();

					bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;

					uint32_t ReadRegister(MemoryRegister& reg) override;
					void WriteRegister(MemoryRegister& reg, uint32_t value) override;

					bool Initialise() override;

				protected:
					virtual void ResetDevice() = 0;
					virtual uint8_t *GetConfigArea() const = 0;
					virtual uint32_t GetConfigAreaSize() const = 0;

					inline VirtQueue *GetCurrentQueue() const
					{
						return GetQueue(QueueSel.Get());
					}
					inline VirtQueue *GetQueue(uint8_t index) const
					{
						if (index > queues.size()) return NULL;
						else return queues[index];
					}

					inline void AssertInterrupt(uint32_t i)
					{
						InterruptStatus.Set(InterruptStatus.Get() | i);

						if(InterruptStatus.Get()) {
							irq.Assert();
						}
					}

					MemoryRegister HostFeaturesSel;
					MemoryRegister HostFeatures;

				private:
					void ProcessQueue(VirtQueue *queue);
					virtual void ProcessEvent(VirtIOQueueEvent* evt) = 0;

					uint8_t guest_page_shift;

					std::vector<VirtQueue *> queues;

					IRQLine& irq;

					MemoryRegister MagicValue;
					MemoryRegister Version;
					MemoryRegister DeviceID;
					MemoryRegister VendorID;

					MemoryRegister GuestFeatures;
					MemoryRegister GuestFeaturesSel;
					MemoryRegister GuestPageSize;

					MemoryRegister QueueSel;
					MemoryRegister QueueNumMax;
					MemoryRegister QueueNum;
					MemoryRegister QueueAlign;
					MemoryRegister QueuePFN;

					MemoryRegister QueueNotify;
					MemoryRegister InterruptStatus;
					MemoryRegister InterruptACK;
					MemoryRegister Status;
				};
			}
		}
	}
}

#endif	/* VIRTIO_H */

