/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   RiscVSystemEmulationModel.h
 * Author: harry
 *
 * Created on 01 February 2019, 09:50
 */

#ifndef RISCVSYSTEMEMULATIONMODEL_H
#define RISCVSYSTEMEMULATIONMODEL_H

#include "abi/LinuxSystemEmulationModel.h"

namespace archsim
{
	namespace arch
	{
		namespace riscv
		{
			class RiscVSystemEmulationModel : public archsim::abi::LinuxSystemEmulationModel
			{
			public:
				RiscVSystemEmulationModel(int xlen);
				virtual ~RiscVSystemEmulationModel();

				gensim::DecodeContext* GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu) override;

				bool Initialise(System& system, archsim::uarch::uArch& uarch) override;

				archsim::abi::ExceptionAction HandleException(archsim::core::thread::ThreadInstance* cpu, uint64_t category, uint64_t data) override;
				archsim::abi::ExceptionAction HandleMemoryFault(archsim::core::thread::ThreadInstance& thread, archsim::MemoryInterface& interface, archsim::Address address) override;
				void HandleInterrupt(archsim::core::thread::ThreadInstance* thread, archsim::abi::devices::CPUIRQLine* irq) override;

				bool PrepareCore(archsim::core::thread::ThreadInstance& core) override;
			};
		}
	}
}

#endif /* RISCVSYSTEMEMULATIONMODEL_H */

