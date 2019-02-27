/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/generic/timing/TickSource.h"
#include "abi/devices/IRQController.h"
#include "arch/risc-v/RiscVSystemCoprocessor.h"
#include "core/thread/ThreadInstance.h"
#include "util/LogContext.h"
#include "system.h"

DeclareLogContext(LogRiscVSystem, "RV-System");
using namespace archsim::arch::riscv;

RiscVSystemCoprocessor::RiscVSystemCoprocessor(archsim::core::thread::ThreadInstance* hart, RiscVMMU* mmu) : hart_(hart), mmu_(mmu), STATUS(hart->GetEmulationModel().GetSystem().GetPubSub())
{
	BitLockGuard guard(lock_);
	true_pending_interrupts_ = 0;

	MTVEC = 0;
	MSCRATCH = 0;
	MEPC = 0;
	MCAUSE = 0;
	MTVAL = 0;

	IE.Reset();
	IP.Reset();

	MIDELEG = 0;
	MEDELEG = 0;

	SIDELEG = 0;
	SEDELEG = 0;
	STVEC = 0;

	SSCRATCH = 0;
	SEPC = 0;
	SCAUSE = 0;
	STVAL = 0;

	MCOUNTEREN = 0;
	SCOUNTEREN = 0;

	WriteMSTATUS(0);
}

bool RiscVSystemCoprocessor::Initialise()
{
	return true;
}

bool RiscVSystemCoprocessor::Read64(uint32_t address, uint64_t& data)
{
	LC_DEBUG2(LogRiscVSystem) << "CSR Read 0x" << std::hex << address << "...";
	switch(address) {
		case 0x100:
			data = STATUS.ReadSSTATUS();
			break;

		case 0x104:
			data = IE.ReadSIE();
			break;

		case 0x105:
			data = STVEC;
			break;

		case 0x106:
			data = SCOUNTEREN;
			break;

		case 0x140:
			data = SSCRATCH;
			break;

		case 0x141:
			data = SEPC;
			break;

		case 0x142:
			data = SCAUSE;
			break;

		case 0x143:
			data = STVAL;
			break;

		case 0x144:
			data = ReadSIP();
			break;

		case 0x180: //SATP
			data = mmu_->GetSATP();
			break;

		case 0x300:
			data = ReadMSTATUS();
			break;

		case 0x301: { // MISA
			data = 0;
			// RV64
			data |= 2ULL << 62;

			// A
			data |= 1;

			// D
			data |= 1 << 3;

			// F
			data |= 1 << 5;

			// I
			data |= 1 << 8;

			// M
			data |= 1 << 12;

			// S
			data |= 1 << 18;

			// U
			data |= 1 << 20;

			break;
		}

		case 0x302: // MEDELEG
			data = MEDELEG;
			break;

		case 0x303: // MIDELEG
			data = MIDELEG;
			break;

		case 0x304:
			data = IE.ReadMIE();
			break;

		case 0x305:
			data = MTVEC;
			break;

		case 0x306:
			data = MCOUNTEREN;
			return true;

		case 0x340:
			data = MSCRATCH;
			break;

		case 0x341:
			data = MEPC;
			break;

		case 0x342:
			data = MCAUSE;
			break;

		case 0x343:
			data = MTVAL;
			break;

		case 0x344:
			data = ReadMIP();
			return true;

		case 0x3a0: // PMPCFG
		case 0x3a1:
		case 0x3a2:
		case 0x3a3:
			data = 0; // RAZ (...?)
			return true;

		case 0x3b0: // PMPADDR
		case 0x3b1:
		case 0x3b2:
		case 0x3b3:
		case 0x3b4:
		case 0x3b5:
		case 0x3b6:
		case 0x3b7:
		case 0x3b8:
		case 0x3b9:
		case 0x3ba:
		case 0x3bb:
		case 0x3bc:
		case 0x3bd:
		case 0x3be:
		case 0x3bf:
			data = 0; // RAZ (...?)
			return true;

		case 0xc01:
			// real time
			data = hart_->GetEmulationModel().GetSystem().GetTickSource()->GetCounter() * 1000;
			LC_DEBUG1(LogRiscVSystem) << "Reading TIME register at PC " << hart_->GetPC() << " -> " << data;
			break;

		case 0xf14: // Hardware Thread ID
			data = hart_->GetThreadID();
			break;

		default:
			LC_DEBUG1(LogRiscVSystem) << "Unimplemented CSR read: " << std::hex << address;
			UNIMPLEMENTED;
	}
	LC_DEBUG2(LogRiscVSystem) << "CSR Read 0x" << std::hex << address << " -> 0x" << std::hex << data;
	return true;
}

bool RiscVSystemCoprocessor::Write64(uint32_t address, uint64_t data)
{
	LC_DEBUG1(LogRiscVSystem) << "CSR Write 0x" << std::hex << address << " <- 0x" << std::hex << data << " at PC " << Address(hart_->GetPC());
	switch(address) {
		case 0x100:
			LC_DEBUG1(LogRiscVSystem) << "Writing SSTATUS=" << Address(data);
			WriteSSTATUS(data);

			break;

		case 0x104:
			LC_DEBUG1(LogRiscVSystem) << "Writing SIE=" << Address(data);
			IE.WriteSIE(data);
			CheckForInterrupts();
			break;

		case 0x105:
			STVEC = data;
			break;

		case 0x106:
			SCOUNTEREN = data;
			break;

		case 0x140:
			SSCRATCH = data;
			break;

		case 0x141:
			SEPC = data;
			break;

		case 0x142:
			SCAUSE = data;
			break;

		case 0x143:
			STVAL = data;
			break;

		case 0x144:
			WriteSIP(data);
			break;

		case 0x180: //SATP
			mmu_->SetSATP(data);
			// flush tlb
			Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, nullptr);
			Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, nullptr);
			break;

		case 0x300: // MSTATUS
			WriteMSTATUS(data);
			CheckForInterrupts();
			break;

		case 0x301: // MISA
			// ignore writes
			break;

		case 0x302: // MEDELEG
			MEDELEG = data;
			break;

		case 0x303: // MIDELEG
			MIDELEG = data;
			break;

		case 0x304:
			LC_DEBUG1(LogRiscVSystem) << "MIE set to " << Address(data);
			IE.WriteMIE(data);
			CheckForInterrupts();
			break;

		case 0x305:
			MTVEC = data;
			break;

		case 0x306:
			MCOUNTEREN = data;
			break;

		case 0x340:
			MSCRATCH = data;
			break;

		case 0x341:
			MEPC = data;
			break;

		case 0x342:
			MCAUSE = data;
			break;

		case 0x343:
			MTVAL = data;
			break;

		case 0x344:
			WriteMIP(data);
			break;

		case 0x3a0: // PMPCFG
		case 0x3a1:
		case 0x3a2:
		case 0x3a3:
			// ignore (...?)
			break;

		case 0x3b0: // PMPADDR
		case 0x3b1:
		case 0x3b2:
		case 0x3b3:
		case 0x3b4:
		case 0x3b5:
		case 0x3b6:
		case 0x3b7:
		case 0x3b8:
		case 0x3b9:
		case 0x3ba:
		case 0x3bb:
		case 0x3bc:
		case 0x3bd:
		case 0x3be:
		case 0x3bf:
			// ignore (...?)
			break;

		case 0xc01:
			// todo: read only
			break;

		case 0xf14: // Hardware Thread ID
			// Writes ignored
			break;

		default:
			LC_DEBUG1(LogRiscVSystem) << "Unimplemented CSR write: " << std::hex << address;
			UNIMPLEMENTED;
	}

	return true;
}

void RiscVSystemCoprocessor::MachinePendInterrupt(uint64_t mask)
{
	std::lock_guard<std::recursive_mutex> lock(lock_);
	IP.PendMask(mask);

	CheckForInterrupts();
}

void RiscVSystemCoprocessor::MachineUnpendInterrupt(uint64_t mask)
{
	std::lock_guard<std::recursive_mutex> lock(lock_);
	IP.ClearMask(mask);

	CheckForInterrupts();
}

static int get_pending_interrupt(uint64_t bits)
{
	for(int i = 0; i < 32; ++i) {
		if(bits & (1 << i)) {
			return i;
		}
	}
	return -1;
}

RiscVSystemCoprocessor::PendingInterrupt RiscVSystemCoprocessor::GetPendingInterrupt()
{
	PendingInterrupt pi = {-1, -1};
	std::lock_guard<std::recursive_mutex> lock(lock_);

	uint8_t priv_mode = hart_->GetExecutionRing();
	// check for M mode interrupts
	// M mode interrupts enabled if privilege < M, or if priv == M and MIE
	if(priv_mode < 3 || (priv_mode == 3 && STATUS.MIE)) {
		// M mode interrupts are enabled, check for a machine mode interrupt
		auto machmode_interrupts_pending = ReadMIP() & IE.ReadMIE() & ~MIDELEG;

		if(machmode_interrupts_pending) {
			LC_DEBUG1(LogRiscVSystem) << "Some machmode interrupts are pending: " << Address(machmode_interrupts_pending);

			// a machine mode interrupt is pending if MIE & MIP & ~MIDELEG
			// i.e., an interrupt is enabled, and pending, and the interrupt is not delegated
			pi.Priv = 3;
			pi.ID = get_pending_interrupt(machmode_interrupts_pending);

			LC_DEBUG1(LogRiscVSystem) << "Machmode interrupt " << pi.ID << " pending";

			// done.
			return pi;
		}
	}

	// check for S mode interrupts
	// S mode interrupts enabled if privilege < S, or if priv == S and SIE
	if(priv_mode < 1 || (priv_mode == 1 && STATUS.SIE)) {
		// a supervisor mode interrupt is pending if MIE & MIP & MIDELEG & ~SIDELEG
		// i.e., if an interrupt is enabled, and pending, and delegated to S mode, and not delegated to U mode
		auto supmode_interrupts_pending = ReadSIP() & IE.ReadSIE() & MIDELEG & ~SIDELEG;

		if(supmode_interrupts_pending) {
			LC_DEBUG1(LogRiscVSystem) << "Some supmode interrupts are pending: " << Address(supmode_interrupts_pending);

			pi.Priv = 1;
			pi.ID = get_pending_interrupt(supmode_interrupts_pending);

			LC_DEBUG1(LogRiscVSystem) << "Supmode interrupt " << pi.ID << " pending";

			// done.
			return pi;
		}
	}

	// check for U mode interrupts
	// U mode interrupts enabled if priv == S and SIE
	if(priv_mode == 0 && STATUS.UIE) {
		// a supervisor mode interrupt is pending if MIE & MIP & MIDELEG & ~SIDELEG
		// i.e., if an interrupt is enabled, and pending, and delegated to S mode, and not delegated to U mode
		if(IE.ReadMIE() & ReadMIP() & MIDELEG & SIDELEG) {
			// figure out which interrupt should be taken and service it in S mode
			UNIMPLEMENTED;

			// done
			return pi;
		}
	}

	// no interrupts are pending
	return pi;
}


void RiscVSystemCoprocessor::CheckForInterrupts()
{
	uint64_t current_pending_interrupts = IP.ReadMIP() & IE.ReadMIE();
	if(current_pending_interrupts != true_pending_interrupts_) {
		LC_DEBUG1(LogRiscVSystem) << "An interrupt should be asserted or rescinded";
		// an interrupt has been brought high or low since we were last here
		for(int i = 0; i < 32; ++i) {
			bool current_pending = (current_pending_interrupts >> i) & 1;
			bool true_pending = (true_pending_interrupts_ >> i) & 1;

			if(current_pending && !true_pending) {
				// interrupt not asserted but should be
				LC_DEBUG1(LogRiscVSystem) << "Asserting IRQ " << i;
				hart_->GetIRQLine(i)->Assert();
			}
			if(!current_pending && true_pending) {
				// interrupt is asserted but shouldn't be
				LC_DEBUG1(LogRiscVSystem) << "Rescinding IRQ " << i;
				hart_->GetIRQLine(i)->Rescind();
			}
		}

		// finally, update true_pending_interrupts
		true_pending_interrupts_ = current_pending_interrupts;
	}

	hart_->PendIRQ();
}

bool RiscVSystemCoprocessor::CanTakeInterrupt(uint8_t ring, uint32_t irq)
{
	// We are currently executing in ring (ring), and (irq) is pending. Should
	// we take a trap?
	// We should take a trap if:
	// - the IRQ is not delegated, and M mode irqs are enabled (machmode < M, or MIE)
	// - the IRQ is delegated to S, and S mode irqs are enabled (machmode < S, or SIE)

	if((ring < 3 || (ring == 3 && STATUS.MIE)) && (~MIDELEG & (1 << irq))) {
		return true;
	}
	if((ring < 1 || (ring == 1 && STATUS.SIE)) && (MIDELEG & (1 << irq))) {
		return true;
	}

	return false;
}

RiscVSystemCoprocessor::STATUS_t::STATUS_t(archsim::util::PubSubContext& pubsub) : pubsub_(pubsub)
{
	SD = 0;
	SXL = 0;
	UXL = 0;
	TSR = 0;
	TW = 0;
	TVM = 0;
	MXR = 0;
	SUM = 0;
	MPRV = 0;
	XS = 0;
	FS = 0;
	MPP = 0;
	SPP = 0;
	MPIE = 0;
	SPIE = 0;
	UPIE = 0;
	MIE = 0;
	SIE = 0;
	UIE = 0;
}


uint64_t RiscVSystemCoprocessor::STATUS_t::ReadMSTATUS()
{
	uint64_t data = 0;
	data |= (uint64_t)SD << 63;
	data |= (uint64_t)SXL << 34;
	data |= (uint64_t)UXL << 32;
	data |= (uint64_t)TSR << 22;
	data |= (uint64_t)TW << 21;
	data |= (uint64_t)TVM << 20;
	data |= (uint64_t)MXR << 19;
	data |= (uint64_t)SUM << 18;
	data |= (uint64_t)MPRV << 17;
	data |= (uint64_t)XS << 15;
	data |= (uint64_t)FS << 13;
	data |= (uint64_t)MPP << 11;
	data |= (uint64_t)SPP << 8;
	data |= (uint64_t)MPIE << 7;
	data |= (uint64_t)SPIE << 5;
	data |= (uint64_t)UPIE << 4;
	data |= (uint64_t)MIE << 3;
	data |= (uint64_t)SIE << 1;
	data |= (uint64_t)UIE << 0;

	return data;
}

void RiscVSystemCoprocessor::STATUS_t::WriteMSTATUS(uint64_t data)
{
	bool should_flush_tlb = false;

	SD = UNSIGNED_BITS_64(data, 63,62);
	SXL = UNSIGNED_BITS_64(data, 34, 33);
	UXL = UNSIGNED_BITS_64(data, 32, 31);
	TSR = BITSEL(data, 22);
	TW = BITSEL(data, 21);
	TVM = BITSEL(data, 20);
	MXR = BITSEL(data, 19);

	bool new_SUM = BITSEL(data, 18);
	if(SUM != new_SUM) {
		should_flush_tlb = true;
	}

	SUM = new_SUM;

	bool new_MPRV = BITSEL(data, 17);
	if(new_MPRV != MPRV) {
		should_flush_tlb = true;
	}
	MPRV = new_MPRV;
	XS = UNSIGNED_BITS_64(data, 16,15);
	FS = UNSIGNED_BITS_64(data, 14,13);
	MPP = UNSIGNED_BITS_64(data, 12,11);
	SPP = BITSEL(data, 8);
	MPIE = BITSEL(data, 7);
	SPIE = BITSEL(data, 5);
	UPIE = BITSEL(data, 4);
	MIE = BITSEL(data, 3);
	SIE = BITSEL(data, 1);
	UIE = BITSEL(data, 0);

	if(should_flush_tlb) {
		pubsub_.Publish(PubSubType::ITlbFullFlush, nullptr);
		pubsub_.Publish(PubSubType::DTlbFullFlush, nullptr);
	}
}

uint64_t RiscVSystemCoprocessor::STATUS_t::ReadSSTATUS()
{
	uint64_t data = 0;
	data |= (uint64_t)SD << 63;
	data |= (uint64_t)UXL << 32;
	data |= (uint64_t)MXR << 19;
	data |= (uint64_t)SUM << 18;
	data |= (uint64_t)XS << 15;
	data |= (uint64_t)FS << 13;
	data |= (uint64_t)SPP << 8;
	data |= (uint64_t)SPIE << 5;
	data |= (uint64_t)UPIE << 4;
	data |= (uint64_t)SIE << 1;
	data |= (uint64_t)UIE << 0;

	return data;
}

void RiscVSystemCoprocessor::STATUS_t::WriteSSTATUS(uint64_t data)
{
	bool should_flush_tlb = false;

	SD = UNSIGNED_BITS_64(data, 63,62);
	UXL = UNSIGNED_BITS_64(data, 32, 31);
	MXR = BITSEL(data, 19);

	bool new_SUM = BITSEL(data, 18);
	if(new_SUM != SUM) {
		should_flush_tlb = true;
	}
	SUM = new_SUM;
	XS = UNSIGNED_BITS_64(data, 16,15);
	FS = UNSIGNED_BITS_64(data, 14,13);
	SPP = BITSEL(data, 8);
	SPIE = BITSEL(data, 5);
	UPIE = BITSEL(data, 4);
	SIE = BITSEL(data, 1);
	UIE = BITSEL(data, 0);

	if(should_flush_tlb) {
		pubsub_.Publish(PubSubType::ITlbFullFlush, nullptr);
		pubsub_.Publish(PubSubType::DTlbFullFlush, nullptr);
	}
}

uint64_t RiscVSystemCoprocessor::ReadMSTATUS()
{
	BitLockGuard guard(lock_);
	return STATUS.ReadMSTATUS();
}

void RiscVSystemCoprocessor::WriteMSTATUS(uint64_t data)
{
	LC_DEBUG1(LogRiscVSystem) << "MSTATUS set to " << Address(data);

	BitLockGuard guard(lock_);
	STATUS.WriteMSTATUS(data);

	CheckForInterrupts();
}

void RiscVSystemCoprocessor::WriteSSTATUS(uint64_t data)
{
	LC_DEBUG1(LogRiscVSystem) << "SSTATUS set to " << Address(data);

	BitLockGuard guard(lock_);
	STATUS.WriteSSTATUS(data);

	CheckForInterrupts();
}


uint64_t RiscVSystemCoprocessor::ReadMIP()
{
	BitLockGuard guard(lock_);
	return IP.ReadMIP();
}

uint64_t RiscVSystemCoprocessor::IP_t::ReadMIP()
{
	uint64_t data = 0;
	data |= MEIP << 11;
	data |= SEIP << 9;
	data |= UEIP << 8;
	data |= MTIP << 7;
	data |= STIP << 5;
	data |= UTIP << 4;
	data |= MSIP << 3;
	data |= SSIP << 1;
	data |= USIP << 0;

	return data;
}

void RiscVSystemCoprocessor::WriteMIP(uint64_t newmip)
{
	BitLockGuard guard(lock_);
	IP.WriteMIP(newmip);
	CheckForInterrupts();
}

void RiscVSystemCoprocessor::IP_t::WriteMIP(uint64_t data)
{
	SEIP = BITSEL(data, 9);
	UEIP = BITSEL(data, 8);
	STIP = BITSEL(data, 5);
	UTIP = BITSEL(data, 4);
	SSIP = BITSEL(data, 1);
	USIP = BITSEL(data, 0);
}

uint64_t RiscVSystemCoprocessor::IP_t::ReadSIP()
{
	uint64_t data = 0;
	data |= SEIP << 9;
	data |= UEIP << 8;
	data |= STIP << 5;
	data |= UTIP << 4;
	data |= SSIP << 1;
	data |= USIP << 0;

	return data;
}

void RiscVSystemCoprocessor::IP_t::WriteSIP(uint64_t data)
{
	UEIP = BITSEL(data, 8);
	SSIP = BITSEL(data, 1);
	USIP = BITSEL(data, 0);
}

void RiscVSystemCoprocessor::IP_t::PendMask(uint64_t data)
{
	MEIP |= BITSEL(data, 11);
	SEIP |= BITSEL(data, 9);
	UEIP |= BITSEL(data, 8);
	MTIP |= BITSEL(data, 7);
	STIP |= BITSEL(data, 5);
	UTIP |= BITSEL(data, 4);
	MSIP |= BITSEL(data, 3);
	SSIP |= BITSEL(data, 1);
	USIP |= BITSEL(data, 0);
}

void RiscVSystemCoprocessor::IP_t::ClearMask(uint64_t data)
{
	data = ~data;

	MEIP &= BITSEL(data, 11);
	SEIP &= BITSEL(data, 9);
	UEIP &= BITSEL(data, 8);
	MTIP &= BITSEL(data, 7);
	STIP &= BITSEL(data, 5);
	UTIP &= BITSEL(data, 4);
	MSIP &= BITSEL(data, 3);
	SSIP &= BITSEL(data, 1);
	USIP &= BITSEL(data, 0);
}

void RiscVSystemCoprocessor::IP_t::Reset()
{
	MEIP = 0;
	SEIP = 0;
	UEIP = 0;
	MTIP = 0;
	STIP = 0;
	UTIP = 0;
	MSIP = 0;
	SSIP = 0;
	USIP = 0;
}


uint64_t RiscVSystemCoprocessor::ReadSIP()
{
	BitLockGuard guard(lock_);
	return IP.ReadSIP();
}

void RiscVSystemCoprocessor::WriteSIP(uint64_t newsip)
{
	BitLockGuard guard(lock_);
	IP.WriteSIP(newsip);
	CheckForInterrupts();
}

uint64_t RiscVSystemCoprocessor::IE_t::ReadMIE()
{
	uint64_t data = 0;

	data |= MEIE << 11;
	data |= SEIE << 9;
	data |= UEIE << 8;

	data |= MTIE << 7;
	data |= STIE << 5;
	data |= UTIE << 4;

	data |= MSIE << 3;
	data |= SSIE << 1;
	data |= USIE << 0;

	return data;
}

uint64_t RiscVSystemCoprocessor::IE_t::ReadSIE()
{
	uint64_t data = 0;

	data |= SEIE << 9;
	data |= UEIE << 8;

	data |= STIE << 5;
	data |= UTIE << 4;

	data |= SSIE << 1;
	data |= USIE << 0;

	return data;
}

void RiscVSystemCoprocessor::IE_t::WriteMIE(uint64_t data)
{
	MEIE = BITSEL(data, 11);
	SEIE = BITSEL(data, 9);
	UEIE = BITSEL(data, 8);

	MTIE = BITSEL(data, 7);
	STIE = BITSEL(data, 5);
	UTIE = BITSEL(data, 4);

	MSIE = BITSEL(data, 3);
	SSIE = BITSEL(data, 1);
	USIE = BITSEL(data, 0);
}

void RiscVSystemCoprocessor::IE_t::WriteSIE(uint64_t data)
{
	SEIE = BITSEL(data, 9);
	UEIE = BITSEL(data, 8);

	STIE = BITSEL(data, 5);
	UTIE = BITSEL(data, 4);

	SSIE = BITSEL(data, 1);
	USIE = BITSEL(data, 0);
}

void RiscVSystemCoprocessor::IE_t::Reset()
{
	MEIE = false;
	SEIE = false;
	UEIE = false;
	MSIE = false;
	SSIE = false;
	USIE = false;
	MTIE = false;
	STIE = false;
	UTIE = false;
}


// Don't support any accesses other than 64 bit
bool RiscVSystemCoprocessor::Read16(uint32_t address, uint16_t& data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Read32(uint32_t address, uint32_t& data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Read8(uint32_t address, uint8_t& data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Write16(uint32_t address, uint16_t data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Write32(uint32_t address, uint32_t data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Write8(uint32_t address, uint8_t data)
{
	UNEXPECTED;
}

