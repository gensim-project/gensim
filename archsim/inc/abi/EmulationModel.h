/*
 * File:   EmulationModel.h
 * Author: s0457958
 *
 * Created on 15 October 2013, 16:29
 */

#ifndef EMULATIONMODEL_H
#define EMULATIONMODEL_H

#include "util/TimerManager.h"

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
	namespace uarch
	{
		class uArch;
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
			unsigned long Value;
			unsigned long Size;

			inline bool Contains(uint32_t address) const
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

			bool AttachDevices();

			virtual gensim::Processor* GetBootCore() = 0;
			virtual gensim::Processor* GetCore(int id) = 0;
			virtual void ResetCores() = 0;
			virtual void HaltCores() = 0;

			virtual gensim::DecodeContext *GetNewDecodeContext(gensim::Processor &cpu) = 0;

			virtual bool PrepareBoot(System& system) = 0;

			virtual ExceptionAction HandleException(gensim::Processor& cpu, uint32_t category, uint32_t data) = 0;

			virtual bool LookupSymbol(unsigned long address, bool exact_match, const BinarySymbol *& symbol) const;
			virtual bool ResolveSymbol(std::string name, unsigned long& value);

			void AddSymbol(unsigned long value, unsigned long size, std::string name, BinarySymbolType type);
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

			std::ifstream *FindDeviceConfigFile(std::string dir, std::string devname);
			bool ConfigureDevice(std::string name, devices::Device *device);

			typedef std::map<int, SignalData *> SignalMap;
			SignalMap _captured_signals;

			typedef std::map<unsigned long, BinarySymbol *> SymbolMap;
			SymbolMap _symbols;
			SymbolMap _functions;
			uint32_t _function_max_size;
		};

	}
}

#endif /* EMULATIONMODEL_H */
