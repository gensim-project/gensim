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
#include "abi/devices/IRQController.h"
#include "abi/devices/Device.h"
#include "util/PubSubSync.h"

#include <mutex>

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

				using BitLockGuard = std::lock_guard<std::recursive_mutex>;

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
				void MachineUnpendInterrupt(uint64_t mask);

				void CheckForInterrupts();

				// Return the
				struct PendingInterrupt {
					int ID, Priv;
				};
				PendingInterrupt GetPendingInterrupt();

				// Are interrupts enabled for this ring?
				bool CanTakeInterrupt(uint8_t ring, uint32_t irq);

				void WriteMSTATUS(uint64_t data);
				void WriteSSTATUS(uint64_t data);
				uint64_t ReadMSTATUS();

				uint64_t ReadMIP();
				void WriteMIP(uint64_t newmip);
				uint64_t ReadSIP();
				void WriteSIP(uint64_t newsip);

				bool GetMTIE() const
				{
					return IE.MTIE;
				}


			private:
				uint64_t MTVEC;
				uint64_t MSCRATCH;
				uint64_t MEPC;
				uint64_t MCAUSE;
				uint64_t MTVAL;

				uint64_t MIDELEG;
				uint64_t MEDELEG;

				uint64_t SIDELEG;
				uint64_t SEDELEG;
				uint64_t STVEC;

				uint64_t SSCRATCH;
				uint64_t SEPC;
				uint64_t SCAUSE;
				uint64_t STVAL;

				uint64_t MCOUNTEREN;
				uint64_t SCOUNTEREN;

				class STATUS_t
				{
				public:
					STATUS_t(archsim::util::PubSubContext &pubsub);
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

				private:
					archsim::util::PubSubContext &pubsub_;
				};
				STATUS_t STATUS;

				struct IP_t {

					bool MEIP;
					bool SEIP;
					bool UEIP;
					bool MTIP;
					bool STIP;
					bool UTIP;
					bool MSIP;
					bool SSIP;
					bool USIP;

					uint64_t ReadMIP();
					void WriteMIP(uint64_t data);
					uint64_t ReadSIP();
					void WriteSIP(uint64_t data);

					void PendMask(uint64_t mask);
					void ClearMask(uint64_t mask);

					void Reset();
				};
				IP_t IP;

				struct IE_t {
					bool MEIE;
					bool SEIE;
					bool UEIE;

					bool MTIE;
					bool STIE;
					bool UTIE;

					bool MSIE;
					bool SSIE;
					bool USIE;

					uint64_t ReadMIE();
					void WriteMIE(uint64_t data);

					uint64_t ReadSIE();
					void WriteSIE(uint64_t data);

					void Reset();
				};
				IE_t IE;

				// Vector of interrupts which are enabled and pending
				// effectively calculated as MIP & MIE
				// If this is ever non-zero, an interrupt should be asserted
				uint32_t true_pending_interrupts_;

				archsim::core::thread::ThreadInstance *hart_;
				RiscVMMU *mmu_;
				std::recursive_mutex lock_;
			};
		}
	}
}

#endif /* RISCVSYSTEMCOPROCESSOR_H */

