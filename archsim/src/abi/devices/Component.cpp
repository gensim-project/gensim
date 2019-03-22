/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * abi/devices/Component.cpp
 */
#include "abi/devices/IRQController.h"
#include "abi/devices/SerialPort.h"
#include "abi/devices/generic/ps2/PS2Device.h"
#include "abi/devices/gfx/VirtualScreen.h"
#include "abi/devices/Component.h"
#include "util/LogContext.h"

#include <set>

UseLogContext(LogDevice);
DeclareChildLogContext(LogMemoryComponent, LogDevice, "MemoryComponent");

using namespace archsim::abi::devices;

// Work around GCC 6.3.0 bug
const ComponentDescriptor Component::dummy_descriptor ("dummy-descriptor");

void ComponentDescriptor::CheckParameterIndices()
{
	std::set<size_t> present;
	int param_index = 0;
	for(auto &i : parameter_descriptors_) {
		i.second.SetIndex(param_index++);
	}
}


ComponentParameterDescriptor::ComponentParameterDescriptor(ComponentParameterType type, bool is_collection) : type_(type), index_(0), is_collection_(false)
{

}

ComponentDescriptorInstance::ComponentDescriptorInstance(const ComponentDescriptor& descriptor) : descriptor_(descriptor)
{
	parameter_value_ptrs_.resize(descriptor_.GetParameterDescriptors().size());
	for(auto i : descriptor.GetParameterDescriptors()) {
		switch(i.second.GetType()) {
			case ComponentParameter_U64:
				parameter_value_ptrs_[i.second.GetIndex()] = new uint64_t();
				break;
			case ComponentParameter_String:
				parameter_value_ptrs_[i.second.GetIndex()] = new std::string("(undefined)");
				break;
			case ComponentParameter_Component:
				parameter_value_ptrs_[i.second.GetIndex()] = new Component*();
				break;
			case ComponentParameter_Thread:
				parameter_value_ptrs_[i.second.GetIndex()] = new archsim::core::thread::ThreadInstance*();
				break;
			default:
				abort();
		}
	}
}

ComponentDescriptorInstance::~ComponentDescriptorInstance()
{
	for(auto i : descriptor_.GetParameterDescriptors()) {
		switch(i.second.GetType()) {
			case ComponentParameter_U64:
				delete (uint64_t*)parameter_value_ptrs_.at(i.second.GetIndex());
				break;
			case ComponentParameter_String:
				delete (std::string*)parameter_value_ptrs_.at(i.second.GetIndex());
				break;
			case ComponentParameter_Component:
				delete (Component**)parameter_value_ptrs_.at(i.second.GetIndex());
				break;
			case ComponentParameter_Thread:
				delete (archsim::core::thread::ThreadInstance**)parameter_value_ptrs_.at(i.second.GetIndex());
				break;
			default:
				abort();
		}
	}
}

void* ComponentDescriptorInstance::GetParameterPointer(const std::string& parameter) const
{
	uint32_t index = GetDescriptor().GetParameterDescriptor(parameter).GetIndex();
	return parameter_value_ptrs_.at(index);
}

template<> void ComponentDescriptorInstance::SetParameter(const std::string& parameter, Component* value)
{
	assert(GetDescriptor().HasParameter(parameter));
	assert(GetDescriptor().GetParameterDescriptor(parameter).GetType() == ComponentParameter_Component);
	void *ptr = GetParameterPointer(parameter);
	Component **cptr = (Component**)ptr;

	*cptr = value;
}

template<> void ComponentDescriptorInstance::SetParameter(const std::string& parameter, archsim::core::thread::ThreadInstance* value)
{
	assert(GetDescriptor().HasParameter(parameter));
	assert(GetDescriptor().GetParameterDescriptor(parameter).GetType() == ComponentParameter_Thread);
	void *ptr = GetParameterPointer(parameter);
	archsim::core::thread::ThreadInstance **cptr = (archsim::core::thread::ThreadInstance**)ptr;

	*cptr = value;
}

template<> void ComponentDescriptorInstance::SetParameter(const std::string& parameter, uint64_t value)
{
	assert(GetDescriptor().HasParameter(parameter));
	assert(GetDescriptor().GetParameterDescriptor(parameter).GetType() == ComponentParameter_U64);
	void *ptr = GetParameterPointer(parameter);
	uint64_t *cptr = (uint64_t*)ptr;

	*cptr = value;
}

template<> void ComponentDescriptorInstance::SetParameter(const std::string& parameter, std::string value)
{
	assert(GetDescriptor().HasParameter(parameter));
	assert(GetDescriptor().GetParameterDescriptor(parameter).GetType() == ComponentParameter_String);
	void *ptr = GetParameterPointer(parameter);
	std::string *cptr = (std::string*)ptr;

	*cptr = value;
}

// Fix for gcc <7
namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			template<> Component *ComponentDescriptorInstance::GetParameter(const std::string &parameter) const
			{
				assert(GetDescriptor().HasParameter(parameter));
				assert(GetDescriptor().GetParameterDescriptor(parameter).GetType() == ComponentParameter_Component);
				void *ptr = GetParameterPointer(parameter);

				return *(Component **)ptr;
			}

			template<> archsim::core::thread::ThreadInstance *ComponentDescriptorInstance::GetParameter(const std::string &parameter) const
			{
				assert(GetDescriptor().HasParameter(parameter));
				assert(GetDescriptor().GetParameterDescriptor(parameter).GetType() == ComponentParameter_Thread);
				void *ptr = GetParameterPointer(parameter);

				return *(archsim::core::thread::ThreadInstance **)ptr;
			}

			template<> uint64_t ComponentDescriptorInstance::GetParameter(const std::string &parameter) const
			{
				assert(GetDescriptor().HasParameter(parameter));
				assert(GetDescriptor().GetParameterDescriptor(parameter).GetType() == ComponentParameter_U64);
				void *ptr = GetParameterPointer(parameter);
				return *(uint64_t*)ptr;
			}

			template<> std::string ComponentDescriptorInstance::GetParameter(const std::string &parameter) const
			{
				assert(GetDescriptor().HasParameter(parameter));
				assert(GetDescriptor().GetParameterDescriptor(parameter).GetType() == ComponentParameter_String);
				void *ptr = GetParameterPointer(parameter);
				return *(std::string*)ptr;
			}

		}
	}
}

Component::~Component() {}

// TODO: do this more nicely
#define CASTTOCOMPONENT(x) static_cast<Component*>(x)
template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::IRQLine *value)
{
	GetDescriptor().SetParameter(parameter, CASTTOCOMPONENT(value));
}
template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::MemoryComponent *value)
{
	GetDescriptor().SetParameter(parameter, CASTTOCOMPONENT(value));
}
template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::ConsoleSerialPort *value)
{
	GetDescriptor().SetParameter(parameter, CASTTOCOMPONENT(value));
}
template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::ConsoleOutputSerialPort *value)
{
	GetDescriptor().SetParameter(parameter, CASTTOCOMPONENT(value));
}
template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::generic::ps2::PS2KeyboardDevice *value)
{
	GetDescriptor().SetParameter(parameter, CASTTOCOMPONENT(value));
}
template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::generic::ps2::PS2MouseDevice *value)
{
	GetDescriptor().SetParameter(parameter, CASTTOCOMPONENT(value));
}
template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::gfx::VirtualScreen *value)
{
	GetDescriptor().SetParameter(parameter, CASTTOCOMPONENT(value));
}
#undef CASTTOCOMPONENT

MemoryComponent::MemoryComponent(EmulationModel &model, Address _base_address, uint32_t _size) : base_address_(_base_address), size_(_size), parent_model_(model)
{

}


MemoryComponent::~MemoryComponent()
{

}

RegisterBackedMemoryComponent::RegisterBackedMemoryComponent(EmulationModel& parent_model, Address base_address, uint32_t size, std::string name)
	: MemoryComponent(parent_model, base_address, size), name(name)
{
}

RegisterBackedMemoryComponent::~RegisterBackedMemoryComponent()
{
}


bool RegisterBackedMemoryComponent::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	MemoryRegister *rg = GetRegister(offset);
	if (!rg) {
		LC_WARNING(LogMemoryComponent) << "Device: " << name << " @ " << std::hex << GetBaseAddress() << ", register read failed: offset=" << std::hex << offset;
		return false;
	}

	data = ReadRegister(*rg);
	LC_DEBUG1(LogMemoryComponent) << "Device: " << name << " @ " << std::hex << GetBaseAddress() << ", register read: " << rg->GetName() << ": offset=" << std::hex << offset << ", size=" << std::dec << (uint32_t)size << ", value=" << std::hex << data;

	return true;
}

bool RegisterBackedMemoryComponent::Write(uint32_t offset, uint8_t size, uint64_t data)
{
	MemoryRegister *rg = GetRegister(offset);
	if (!rg) {
		LC_WARNING(LogMemoryComponent) << "Device: " << name << " @ " << std::hex << GetBaseAddress() << ", register write failed: offset=" << std::hex << offset;
		return false;
	}

	WriteRegister(*rg, data);

	LC_DEBUG1(LogMemoryComponent) << "Device: " << name << " @ " << std::hex << GetBaseAddress() << ", register write: " << rg->GetName() << ": offset=" << std::hex << offset << ", size=" << std::dec << (uint32_t)size << ", value=" << std::hex << data;

	return true;
}

void RegisterBackedMemoryComponent::AddRegister(MemoryRegister& rg)
{
	assert(rg.GetOffset() < GetSize());
	registers[rg.GetOffset()] = &rg;
}

MemoryRegister *RegisterBackedMemoryComponent::GetRegister(uint32_t offset)
{
	register_map_t::iterator rg;
	rg = registers.find(offset);
	if (rg == registers.end())
		return NULL;
	return rg->second;
}

uint32_t RegisterBackedMemoryComponent::ReadRegister(MemoryRegister& reg)
{
	return reg.Read();
}

void RegisterBackedMemoryComponent::WriteRegister(MemoryRegister& reg, uint32_t value)
{
	reg.Write(value);
}

MemoryRegister::MemoryRegister(std::string name, uint32_t offset, uint8_t width, uint32_t default_value, bool can_read, bool can_write)
	: name(name),
	  offset(offset),
	  width(width),
	  value(default_value & (((uint64_t)1 << width) - 1)),
	  can_read(can_read),
	  can_write(can_write)
{
}
