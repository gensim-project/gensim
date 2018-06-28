/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/EmulationModel.h"
#include "abi/devices/IRQController.h"
#include "core/thread/ThreadInstance.h"
#include "core/MemoryInterface.h"
#include "util/LogContext.h"
#include "system.h"

#include <setjmp.h>

// These are deprecated and should be removed soon
DeclareLogContext(LogCPU, "CPU");
DeclareLogContext(LogBlockJitCpu, "BlockJIT");


using namespace archsim;
using namespace archsim::core::thread;

void FPState::Apply()
{
	// TODO: properly apply the floating point state represented by this FPState object.
	UNIMPLEMENTED;
}


ThreadInstance::ThreadInstance(util::PubSubContext &pubsub, const ArchDescriptor& arch, archsim::abi::EmulationModel &emu_model) : pubsub_(pubsub), descriptor_(arch), state_block_(), features_(pubsub), emu_model_(emu_model), register_file_(arch.GetRegisterFileDescriptor()), peripherals_(*this)
{
	// Need to fill in structures based on arch descriptor info

	// 2. Memory interfaces
	memory_interfaces_.resize(arch.GetMemoryInterfaceDescriptor().GetInterfaces().size());
	for(auto &interface_descriptor : arch.GetMemoryInterfaceDescriptor().GetInterfaces()) {
		memory_interfaces_[interface_descriptor.second.GetID()] = new MemoryInterface(interface_descriptor.second);
	}
	fetch_mi_ = memory_interfaces_.at(arch.GetMemoryInterfaceDescriptor().GetFetchInterface().GetID());

	// 3. Features
	for(auto feature : GetArch().GetFeaturesDescriptor().GetFeatures()) {
		GetFeatures().AddNamedFeature(feature.GetName(), feature.GetID());
	}

	// Set default FP state
	// TODO: this

	metrics_ = new archsim::core::thread::ThreadMetrics();

	// Set up default state block entries

	// Set up thread pointer back to this
	state_block_.AddBlock("thread_ptr", sizeof(this));
	state_block_.SetEntry<ThreadInstance*>("thread_ptr", this);

	// Set up ISA Mode ID
	mode_offset_ = state_block_.AddBlock("ModeID", sizeof(uint32_t));
	state_block_.SetEntry<uint32_t>("ModeID", 0);

	// Set up Ring ID
	ring_offset_ = state_block_.AddBlock("RingID", sizeof(uint32_t));
	state_block_.SetEntry<uint32_t>("RingID", 0);

	message_waiting_ = false;
	trace_source_ = nullptr;
}

void ThreadInstance::ReturnToSafepoint()
{
	longjmp(Safepoint, 1);
}

void ThreadInstance::SetExecutionRing(uint32_t new_ring)
{
	GetStateBlock().SetEntry<uint32_t>("RingID", new_ring);
	GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::PrivilegeLevelChange, this);
}

MemoryInterface& ThreadInstance::GetMemoryInterface(const std::string& interface_name)
{
	for(auto i : memory_interfaces_) {
		if(i->GetDescriptor().GetName() == interface_name) {
			return *i;
		}
	}

	throw std::logic_error("No memory interface found with given name " + interface_name);
}
MemoryInterface& ThreadInstance::GetMemoryInterface(uint32_t interface_name)
{
	return *memory_interfaces_.at(interface_name);
}

Address RegisterFileInterface::GetTaggedSlot(const std::string &tag) const
{
	auto descriptor = descriptor_.GetTaggedEntry(tag);

	switch(descriptor.GetEntrySize()) {
		case 4:
			return Address(*(uint32_t*)(GetData() + descriptor.GetOffset()));
		case 8:
			return Address(*(uint64_t*)(GetData() + descriptor.GetOffset()));
		default:
			UNIMPLEMENTED;
	}
}

void RegisterFileInterface::SetTaggedSlot(const std::string &tag, Address target)
{
	auto descriptor = descriptor_.GetTaggedEntry(tag);

	switch(descriptor.GetEntrySize()) {
		case 4:
			*(uint32_t*)(GetData() + descriptor.GetOffset()) = target.Get();
			break;
		case 8:
			*(uint64_t*)(GetData() + descriptor.GetOffset()) = target.Get();
			break;
		default:
			UNIMPLEMENTED;
	}
}

archsim::abi::ExceptionAction ThreadInstance::TakeException(uint64_t category, uint64_t data)
{
	auto result = emu_model_.HandleException(this, category, data);
	if(result != archsim::abi::ExceptionAction::ResumeNext) {
		ReturnToSafepoint();
	}
	return result;
}

archsim::abi::ExceptionAction ThreadInstance::TakeMemoryException(MemoryInterface& interface, Address address)
{
	auto result = emu_model_.HandleMemoryFault(*this, interface, address);
	if(result != archsim::abi::ExceptionAction::ResumeNext) {
		ReturnToSafepoint();
	}
	return result;
}


archsim::abi::devices::IRQLine* ThreadInstance::GetIRQLine(uint32_t irq_no)
{
	if(irq_lines_.size() <= irq_no) {
		irq_lines_.resize(irq_no+1);
	}

	if(irq_lines_[irq_no] == nullptr) {
		irq_lines_[irq_no] = new archsim::abi::devices::CPUIRQLine(this);
		irq_lines_[irq_no]->SetLine(irq_no);
	}
	return irq_lines_.at(irq_no);
}

void ThreadInstance::TakeIRQ()
{
	pending_irqs_++;
	SendMessage(ThreadMessage::Interrupt);
}

void ThreadInstance::RescindIRQ()
{
	pending_irqs_--;
}

archsim::core::execution::ExecutionResult ThreadInstance::HandleMessage()
{
	using archsim::core::execution::ExecutionResult;

	auto message = GetNextMessage();
	switch(message) {
		case ThreadMessage::Nop:
			return ExecutionResult::Continue;
		case ThreadMessage::Halt:
			return ExecutionResult::Halt;
		case ThreadMessage::Interrupt:
			HandleIRQ();
			return ExecutionResult::Exception;
		default:
			throw std::logic_error("Unexpected message");
	}
}

void ThreadInstance::HandleIRQ()
{
	for(auto irq : irq_lines_) {
		if(irq && irq->IsAsserted() && !irq->IsAcknowledged()) {
			GetEmulationModel().HandleInterrupt(this, irq);
			irq->Acknowledge();
		}
	}
}

void ThreadInstance::PendIRQ()
{
	bool should_pend_interrupts = false;
	for (auto irq : irq_lines_) {
		if (irq && irq->IsAsserted()) {
			should_pend_interrupts = true;
			break;
		}
	}

	if (should_pend_interrupts) {
		SendMessage(ThreadMessage::Interrupt);

		for (auto irq : irq_lines_) {
			if(irq) {
				irq->Raise();
			}
		}
	}
}
