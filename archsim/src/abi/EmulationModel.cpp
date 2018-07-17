/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * abi/EmulationModel.cpp
 */
#include "abi/Address.h"
#include "abi/EmulationModel.h"
#include "abi/memory/MemoryModel.h"
#include "abi/devices/Device.h"
#include "core/thread/ThreadInstance.h"

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

void EmulationModel::HandleInterrupt(archsim::core::thread::ThreadInstance* thread, archsim::abi::devices::CPUIRQLine* irq)
{
	UNIMPLEMENTED;
}

ExceptionAction EmulationModel::HandleMemoryFault(archsim::core::thread::ThreadInstance& thread, archsim::MemoryInterface& interface, archsim::Address address)
{
	LC_ERROR(LogEmulationModel) << "A memory fault occurred at address " << address;
	thread.SendMessage(archsim::core::thread::ThreadMessage::Halt);
	return ExceptionAction::AbortSimulation;
}

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

bool EmulationModel::LookupSymbol(Address address, bool exact_match, const BinarySymbol *& symbol_out) const
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

bool EmulationModel::ResolveSymbol(std::string name, Address& value)
{
	for (auto symbol : _functions) {
		if (symbol.second->Name == name) {
			value = symbol.first;
			return true;
		}
	}

	return false;
}


void EmulationModel::AddSymbol(Address value, unsigned long size, std::string name, BinarySymbolType type)
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
			i->second->Size = (next->first - i->first).Get();
		}
	}
}
