/*
 * abi/EmulationModel.cpp
 */
#include "abi/EmulationModel.h"
#include "abi/devices/Device.h"

#include "gensim/gensim_processor.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "util/SimOptions.h"

#include <dirent.h>

using namespace archsim::abi;

DefineComponentType(EmulationModel);

DeclareLogContext(LogEmulationModel, "EmulationModel");
DeclareChildLogContext(LogEmulationModelSignals, LogEmulationModel, "Signals");

UseLogContext(LogSystem);

EmulationModel::EmulationModel()
	: timer_mgr(new util::timing::RealTimeTimerSource<std::chrono::high_resolution_clock, std::chrono::microseconds>()),
	  system(NULL),
	  uarch(NULL),
	  _function_max_size(0)
{
	timer_mgr.start();
}

void EmulationModel::HandleInterrupt(archsim::ThreadInstance* thread, archsim::abi::devices::CPUIRQLine* irq)
{
	UNIMPLEMENTED;
}


gensim::Processor *EmulationModel::GetBootCore() { UNIMPLEMENTED; }

EmulationModel::~EmulationModel()
{
}

bool EmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	this->system = &system;
	this->uarch = &uarch;

	if (!GetComponentInstance(archsim::options::MemoryModel, memory_model)) {
		LC_ERROR(LogSystem) << "Unable to create memory model '" << archsim::options::MemoryModel.GetValue() << "'";
		return false;
	}

	if (!memory_model->Initialise()) {
		LC_ERROR(LogSystem) << "Unable to initialise memory model '" << archsim::options::MemoryModel.GetValue() << "'";
		return false;
	}

	return true;
}

void EmulationModel::Destroy()
{
	memory_model->Destroy();
	delete memory_model;
}

std::ifstream *EmulationModel::FindDeviceConfigFile(std::string dirname, std::string devname)
{
	DIR *dir = opendir(dirname.c_str());
	if (!dir)
		return NULL;

	char target_file[256];
	snprintf(target_file, sizeof(target_file) - 1, "%s.cfg", devname.c_str());

	dirent* ent;
	while ((ent = readdir(dir)) != NULL) {
		if (strcmp(ent->d_name, target_file) == 0) {
			std::ifstream *stream = new std::ifstream(ent->d_name);
			closedir(dir);

			return stream;
		}
	}

	closedir(dir);
	return NULL;
}

bool EmulationModel::ConfigureDevice(std::string name, devices::Device *device)
{
	std::ifstream *config_file;
	std::map<std::string, std::string> settings;

	config_file = FindDeviceConfigFile(".", name);
	if (!config_file) {
		LC_WARNING(LogEmulationModel) << "No device configuration file for " << name;
		return true;
	}

	assert(config_file->good());

	std::string line;
	while (std::getline(*config_file, line)) {
		char *native_line = strdup(line.c_str());
		char *name = strtok(native_line, "=");
		char *value = strtok(NULL, "=");

		settings[std::string(name)] = std::string(value);
		free(native_line);
	}

	config_file->close();
	delete config_file;

	auto attach = settings.at("attach");
	if (!attach.empty()) {
		LC_DEBUG1(LogEmulationModel) << "Attaching device to: " << attach;
		GetBootCore()->peripherals.AttachDevice(name, atoi(attach.c_str()));
	}

	return device->Configure(GetBootCore(), settings);
}

bool EmulationModel::AttachDevices()
{
	for (const auto requested_device : *archsim::options::EnabledDevices.GetValue()) {
		devices::Device *device;

		if (!GetComponentInstance(requested_device, device)) {
			LC_ERROR(LogEmulationModel) << "Unable to instantiate device: " << requested_device;
			return false;
		}

		LC_DEBUG1(LogEmulationModel) << "Initialising device: " << requested_device;

		GetBootCore()->peripherals.RegisterDevice(requested_device, device);

		if (!ConfigureDevice(requested_device, device)) {
			LC_ERROR(LogEmulationModel) << "Device configuration failed for: " << requested_device;
			return false;
		}
	}

	return true;
}

bool EmulationModel::CaptureSignal(int signal, uint32_t pc, void *priv)
{
	SignalData *data;

	auto regsig = _captured_signals.find(signal);
	if (regsig == _captured_signals.end())
		data = new SignalData();
	else
		data = regsig->second;

	LC_DEBUG1(LogEmulationModelSignals) << "Capturing signal " << signal << " @ " << std::hex << pc;

	data->pc = pc;
	data->priv = priv;
	_captured_signals[signal] = data;

	return true;
}

bool EmulationModel::ReleaseSignal(int signal)
{
	auto regsig = _captured_signals.find(signal);
	if (regsig == _captured_signals.end())
		return false;

	LC_DEBUG1(LogEmulationModelSignals) << "Releasing signal " << signal;
	_captured_signals.erase(regsig);
	return true;
}

bool EmulationModel::GetSignalData(int signal, SignalData*& priv)
{
	auto regsig = _captured_signals.find(signal);
	if (regsig == _captured_signals.end())
		return false;

	priv = regsig->second;
	return true;
}

bool EmulationModel::HandleSignal(int signal, void *si, void *unused)
{
	auto regsig = _captured_signals.find(signal);
	if (regsig == _captured_signals.end())
		return false;

	LC_DEBUG1(LogEmulationModelSignals) << "Handling captured signal " << signal;
	return AssertSignal(signal, regsig->second);
}

bool EmulationModel::AssertSignal(int signal, SignalData *data)
{
	return false;
}

bool EmulationModel::InvokeSignal(int signal, uint32_t next_pc, SignalData *data)
{
	return false;
}

bool EmulationModel::LookupSymbol(unsigned long address, bool exact_match, const BinarySymbol *& symbol_out) const
{
	if (exact_match) {
		SymbolMap::const_iterator symbol = _functions.find(address);
		if (symbol == _functions.end())
			return false;

		symbol_out = symbol->second;
		return true;
	} else {
		auto i = _functions.lower_bound(address);
		if(i == _functions.begin()) return false;
		i--;

		for(; i != _functions.end(); ++i) {
			if(i->second->Contains(address)) {
				symbol_out = i->second;
				return true;
			}
			if(i->first > address) return false;
		}

		return false;
	}
}

bool EmulationModel::ResolveSymbol(std::string name, unsigned long& value)
{
	for (auto symbol : _functions) {
		if (symbol.second->Name == name) {
			value = symbol.first;
			return true;
		}
	}

	return false;
}


void EmulationModel::AddSymbol(unsigned long value, unsigned long size, std::string name, BinarySymbolType type)
{
	BinarySymbol *symbol = new BinarySymbol();

	symbol->Name = name;
	symbol->Type = type;
	symbol->Value = value;
	symbol->Size = size;

	if(type == FunctionSymbol) {
		_functions[value] = symbol;
		if(name.size() > _function_max_size) _function_max_size = name.size();
	} else _symbols[value] = symbol;
}

void EmulationModel::FixupSymbols()
{
	if(_functions.size() < 2) {
		return;
	}
	auto second_from_last = _functions.end();
	second_from_last--;
	for(auto i = _functions.begin(); i != second_from_last; ++i) {
		if(i->second->Size == 0) {
			auto next = i;
			next++;
			i->second->Size = next->first - i->first;
		}
	}
}
