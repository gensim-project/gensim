/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   EmulationModel.h
 * Author: s0457958
 *
 * Created on 15 October 2013, 16:29
 */

#ifndef EMULATIONMODEL_H
#define EMULATIONMODEL_H

#include "util/TimerManager.h"
#include "abi/Address.h"

#include <fstream>
#include <set>
#include <map>

class System;

namespace gensim
{
	class DecodeContext;
	class Processor;
	class TraceManager;
}
namespace archsim
{
	class MemoryInterface;
	
	namespace uarch
	{
		class uArch;
	}
	
	namespace core {
		namespace thread {
			class ThreadInstance;
		}
	}

	namespace abi
	{
		namespace memory
		{
			class MemoryModel;
		}
		namespace devices
		{
			class Device;
			class DeviceManager;
			class SystemComponent;
			class CoreComponent;
			class MemoryComponent;
			class CPUIRQLine;
		}

		namespace loader
		{
			class ElfBinaryLoader;
			class UserElfBinaryLoader;
		}

		enum BinarySymbolType {
			FunctionSymbol,
			ObjectSymbol
		};

		struct BinarySymbol {
			BinarySymbolType Type;
			std::string Name;
			Address Value;
			unsigned long Size;

			inline bool Contains(Address address) const
			{
				return (address >= Value) && (address < (Value + Size));
			}
		};

		enum ExceptionAction {
			None,
			ResumeNext,
			AbortInstruction,
			AbortSimulation
		};

		struct SignalData {
			uint32_t pc;
			void *priv;
		};

		class EmulationModel
		{
		public:
			EmulationModel();
			virtual ~EmulationModel();

			bool CaptureSignal(int signal, uint32_t pc, void* priv);
			bool ReleaseSignal(int signal);
			bool GetSignalData(int signal, SignalData*& priv);
			bool HandleSignal(int signal, void *si, void *unused);
			virtual bool AssertSignal(int signal, SignalData* data);
			virtual bool InvokeSignal(int signal, uint32_t next_pc, SignalData* data);

			virtual bool Initialise(System& system, archsim::uarch::uArch& uarch);
			virtual void Destroy();

			virtual void HaltCores() = 0;

			virtual gensim::DecodeContext *GetNewDecodeContext(archsim::core::thread::ThreadInstance &cpu) = 0;

			virtual bool PrepareBoot(System& system) = 0;

			virtual ExceptionAction HandleException(archsim::core::thread::ThreadInstance* thread, uint32_t category, uint32_t data) = 0;
			virtual ExceptionAction HandleMemoryFault(archsim::core::thread::ThreadInstance &thread, archsim::MemoryInterface &interface, archsim::Address address);
			virtual void HandleInterrupt(archsim::core::thread::ThreadInstance* thread, archsim::abi::devices::CPUIRQLine *irq);

			virtual bool LookupSymbol(Address address, bool exact_match, const BinarySymbol *& symbol) const;
			virtual bool ResolveSymbol(std::string name, Address& value);

			void AddSymbol(Address value, unsigned long size, std::string name, BinarySymbolType type);
			void FixupSymbols();

			virtual void PrintStatistics(std::ostream& stream) = 0;

			util::timing::TimerManager timer_mgr;

			inline abi::memory::MemoryModel& GetMemoryModel() const
			{
				return *memory_model;
			}

			inline archsim::uarch::uArch& GetuArch() const
			{
				return *uarch;
			}

			inline System& GetSystem() const
			{
				return *system;
			}

			inline uint32_t GetMaxSymbolSize() const
			{
				return _function_max_size;
			}

		private:
			System *system;

			abi::memory::MemoryModel *memory_model;
			archsim::uarch::uArch *uarch;

			typedef std::map<int, SignalData *> SignalMap;
			SignalMap _captured_signals;

			typedef std::map<Address, BinarySymbol *> SymbolMap;
			SymbolMap _symbols;
			SymbolMap _functions;
			uint32_t _function_max_size;
		};

	}
}

#endif /* EMULATIONMODEL_H */
