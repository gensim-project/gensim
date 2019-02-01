/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   RiscVSystemCoprocessor.h
 * Author: harry
 *
 * Created on 29 January 2019, 13:21
 */

#ifndef RISCVSYSTEMCOPROCESSOR_H
#define RISCVSYSTEMCOPROCESSOR_H

#include "arch/risc-v/RiscVMMU.h"
#include "abi/devices/Device.h"

namespace archsim
{
	namespace arch
	{
		namespace riscv
		{

			/**
			 * Device intended to co-ordinate between different parts of the
			 * RISC-V system: core, MMU, PLIC, etc.
			 */
			class RiscVSystemCoprocessor : public archsim::abi::devices::Device
			{
			public:
				RiscVSystemCoprocessor(archsim::core::thread::ThreadInstance *hart, RiscVMMU *mmu);

				virtual bool Initialise();

				bool Read8(uint32_t address, uint8_t& data) override;
				bool Read16(uint32_t address, uint16_t& data) override;
				bool Read32(uint32_t address, uint32_t& data) override;

				bool Write8(uint32_t address, uint8_t data) override;
				bool Write16(uint32_t address, uint16_t data) override;
				bool Write32(uint32_t address, uint32_t data) override;

				virtual bool Read64(uint32_t address, uint64_t& data);
				virtual bool Write64(uint32_t address, uint64_t data);

				void MachinePendInterrupt(uint64_t mask);

				void CheckForInterrupts();

				void WriteMSTATUS(uint64_t data);
				uint64_t ReadMSTATUS();



			private:
				uint64_t MTVEC;
				uint64_t MSCRATCH;
				uint64_t MEPC;
				uint64_t MCAUSE;
				uint64_t MTVAL;

				uint64_t MIE;
				uint64_t MIP;

				uint64_t MIDELEG;
				uint64_t MEDELEG;

				uint64_t SIDELEG;
				uint64_t SEDELEG;
				uint64_t SIE;
				uint64_t STVEC;

				uint64_t SSCRATCH;
				uint64_t SEPC;
				uint64_t SCAUSE;
				uint64_t STVAL;
				uint64_t SIP;

				uint64_t MCOUNTEREN;
				uint64_t SCOUNTEREN;

				struct STATUS_t {
					bool SD;
					uint8_t SXL;
					uint8_t UXL;
					bool TSR;
					bool TW;
					bool TVM;
					bool MXR;
					bool SUM;
					bool MPRV;
					uint8_t XS;
					uint8_t FS;
					uint8_t MPP;
					bool SPP;
					bool MPIE;
					bool SPIE;
					bool UPIE;
					bool MIE;
					bool SIE;
					bool UIE;

					uint64_t ReadMSTATUS();
					void WriteMSTATUS(uint64_t data);

					uint64_t ReadSSTATUS();
					void WriteSSTATUS(uint64_t data);
				};
				STATUS_t STATUS;

				archsim::core::thread::ThreadInstance *hart_;
				RiscVMMU *mmu_;
			};
		}
	}
}

#endif /* RISCVSYSTEMCOPROCESSOR_H */

