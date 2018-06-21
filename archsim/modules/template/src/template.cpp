/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <abi/devices/Component.h>
#include <module/Module.h>
#include <cstdint>

#include <cstddef>

using namespace archsim::abi::devices;
using namespace archsim::abi;
using namespace archsim::module;

static ComponentDescriptor template_descriptor ("TemplateDevice", {{"SomeParameter", ComponentParameterDescriptor(ComponentParameter_U64, 0)}});
class TemplateDevice : public MemoryComponent
{
public:
	TemplateDevice(EmulationModel &model, archsim::Address base_address) : MemoryComponent(template_descriptor, model, base_address, 4096) {}
	~TemplateDevice() {}

	virtual bool Read(uint32_t offset, uint8_t size, uint32_t &data) override
	{
		data = GetParameter<uint64_t>("SomeParameter");
		return true;
	}
	virtual bool Write(uint32_t offset, uint8_t size, uint32_t data) override
	{
		data_ = data;
		return true;
	}

private:
	uint32_t data_;
};

ARCHSIM_MODULE()
{
	auto module = new ModuleInfo("TemplateDevice", "An example device");

	auto device_entry = new ModuleDeviceEntry("TemplateDevice", ARCHSIM_DEVICEFACTORY(TemplateDevice));
	module->AddEntry(device_entry);

	return module;
}

ARCHSIM_MODULE_END()
{

}
