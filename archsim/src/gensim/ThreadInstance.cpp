/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "abi/EmulationModel.h"
#include "abi/devices/IRQController.h"
#include "gensim/ThreadInstance.h"
#include "gensim/MemoryInterface.h"

using namespace archsim;

ThreadInstance::ThreadInstance(util::PubSubContext &pubsub, const ArchDescriptor& arch, archsim::abi::EmulationModel &emu_model) : pubsub_(pubsub), descriptor_(arch), state_block_(), features_(pubsub), emu_model_(emu_model), mode_id_(0), ring_id_(0), register_file_(arch.GetRegisterFileDescriptor()), peripherals_(*this)
{
	// Need to fill in structures based on arch descriptor info
	
	// 2. Memory interfaces
	for(auto &interface_descriptor : arch.GetMemoryInterfaceDescriptor().GetInterfaces()) {
		memory_interfaces_[interface_descriptor.second.GetName()] = new MemoryInterface(interface_descriptor.second);
	}
	
	// 3. Features
	for(auto feature : GetArch().GetFeaturesDescriptor().GetFeatures()) {
		GetFeatures().AddNamedFeature(feature.GetName(), feature.GetID());
	}
	
	// Set default FP state
	// TODO: this
}

MemoryInterface& ThreadInstance::GetMemoryInterface(const std::string& interface_name)
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


MemoryInterface &ThreadInstance::GetFetchMI()
{
	return *memory_interfaces_.at(descriptor_.GetMemoryInterfaceDescriptor().GetFetchInterface().GetName());
}

archsim::abi::ExceptionAction ThreadInstance::TakeException(uint64_t category, uint64_t data)
{
	return emu_model_.HandleException(this, category, data);
}

archsim::abi::devices::IRQLine* ThreadInstance::GetIRQLine(uint32_t irq_no)
{
	if(irq_lines_.count(irq_no) == 0) {
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

ExecutionResult ThreadInstance::HandleMessage()
{
	auto message = GetNextMessage();
	switch(message) {
		case ThreadMessage::Nop: return ExecutionResult::Continue;
		case ThreadMessage::Halt: return ExecutionResult::Halt;
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
		if(irq.second->IsAsserted() && !irq.second->IsAcknowledged()) {
			GetEmulationModel().HandleInterrupt(this, irq.second);
			irq.second->Acknowledge();
		}
	}
}

void ThreadInstance::PendIRQ()
{
	bool should_pend_interrupts = false;
	for (auto irq : irq_lines_) {
		if (irq.second->IsAsserted()) {
			should_pend_interrupts = true;
			break;
		}
	}

	if (should_pend_interrupts) {
		SendMessage(ThreadMessage::Interrupt);

		for (auto irq : irq_lines_) {
			irq.second->Raise();
		}
	}
}
