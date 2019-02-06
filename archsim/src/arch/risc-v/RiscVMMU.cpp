/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/risc-v/RiscVMMU.h"
#include "abi/memory/MemoryModel.h"
#include "core/thread/ThreadInstance.h"
#include "util/LogContext.h"

using namespace archsim;
using namespace archsim::arch::riscv;
using namespace archsim::abi::devices;

DeclareLogContext(LogRiscVMMU, "RISCV-MMU");

Address::underlying_t RiscVMMU::PTE::GetPPN() const
{
	return data_ >> 10;
}

Address::underlying_t RiscVMMU::PTE::GetPPN(Mode mode, int level) const
{
	switch(mode) {
		case Sv32:
			switch(level) {
				case 0:
					return (data_ >> 10) & 0x3ff;
				case 1:
					return (data_ >> 20) & 0xfff;
				default:
					UNEXPECTED;
			}
		case Sv39:
			switch(level) {
				case 0:
					return (data_ >> 10) & 0x1ff;
				case 1:
					return (data_ >> 19) & 0x1ff;
				case 2:
					return (data_ >> 28) & 0x3ffffff;
				default:
					UNEXPECTED;
			}
		case Sv48:
			switch(level) {
				case 0:
					return (data_ >> 10) & 0x1ff;
				case 1:
					return (data_ >> 19) & 0x1ff;
				case 2:
					return (data_ >> 28) & 0x1ff;
				case 3:
					return (data_ >> 37) & 0x1ffff;
				default:
					UNEXPECTED;
			}
		default:
			UNEXPECTED;
	}
}

bool RiscVMMU::Initialise()
{
	mode_ = NoTxln;

	return true;
}

const PageInfo RiscVMMU::GetInfo(Address virt_addr)
{
	auto pteinfo = GetInfoLevel(virt_addr, Address(GetSATP() * Address::PageSize), GetPTLevels(GetMode()));
	return std::get<1>(pteinfo);
}

RiscVMMU::Mode RiscVMMU::GetMode() const
{
	return mode_;
}

uint64_t RiscVMMU::GetSATP() const
{
	uint8_t mode_bits = 0;
	switch(mode_) {
		case NoTxln:
			mode_bits = 0;
			break;
		case Sv32:
			mode_bits = 1;
			break;
		case Sv39:
			mode_bits = 8;
			break;
		case Sv48:
			mode_bits = 9;
			break;
		default:
			UNEXPECTED;
	}

	// construct satp from parts
	return page_table_ppn_ | (uint64_t)asid_ << 22 | (uint64_t)mode_bits << 60;
}

void RiscVMMU::SetSATP(uint64_t new_satp)
{
	uint64_t new_pt_ppn = new_satp & 0xfffffffffff;
	uint64_t new_asid = (new_satp >> 44) & 0xffff;
	uint64_t new_mode = new_satp >> 60;

	page_table_ppn_ = new_pt_ppn;
	asid_ = new_asid;

	switch(new_mode) {
		case 0:
			mode_ = NoTxln;
			break;
		case 1:
			mode_ = Sv32;
			break;
		case 8:
			mode_ = Sv39;
			break;
		case 9:
			mode_ = Sv48;
			break;
		default:
			UNEXPECTED;
	}
}


int RiscVMMU::GetPTLevels(Mode mode) const
{
	switch(mode) {
		case Sv32:
			return 2;
		case Sv39:
			return 3;
		case Sv48:
			return 4;
		default:
			UNEXPECTED;
	}
}

MMU::TranslateResult RiscVMMU::Translate(archsim::core::thread::ThreadInstance* cpu, Address virt_addr, Address& phys_addr, AccessInfo info)
{
	auto satp = GetSATP();

	// No translation in NoTxln mode, or in Machine mode
	if(GetMode() == NoTxln || info.Ring == 3) {
		phys_addr = virt_addr;
		return TXLN_OK;
	}

	// TODO: satp.ppn size depends on xlen
	auto satp_ppn = satp & 0xfffffffffff;
	auto pt_base = satp_ppn * Address::PageSize;

	auto pteinfo = GetInfoLevel(virt_addr, Address(pt_base), GetPTLevels(GetMode())-1);
	auto pageinfo = std::get<1>(pteinfo);

	if(pageinfo.Present) {
		LC_DEBUG1(LogRiscVMMU) << "Page Present";

		// check permissions!
		phys_addr = pageinfo.phys_addr.PageBase() | (virt_addr & ~pageinfo.mask);

		// check dirty/access bits (should this be done in hardware? configurable?)
	} else {
		// page not present
		LC_DEBUG1(LogRiscVMMU) << "Page NOT Present";

		// TODO: difference between access fault and page fault
		if(info.SideEffects) {
			LC_DEBUG1(LogRiscVMMU) << "Triggering exception";

			uint32_t cause = 0;
			if(info.Fetch) {
				cause = 12;
			} else if(info.Write) {
				cause = 15;
			} else {
				cause = 13;
			}
			cpu->GetEmulationModel().HandleException(cpu, cause, virt_addr.Get());
			return MMU::TXLN_FAULT_PAGE;
		}
	}

	return MMU::TXLN_OK;
}

RiscVMMU::PTEInfo RiscVMMU::GetInfoLevel(Address virt_addr, Address table, int level)
{
	LC_DEBUG1(LogRiscVMMU) << "Getting info for " << virt_addr << ", table " << table << ", level " << level;

	auto pte_address = table + GetVPN(GetMode(), virt_addr, level) * GetPTESize(GetMode());
	auto pte = LoadPTE(GetMode(), pte_address);

	LC_DEBUG1(LogRiscVMMU) << "PTE Address is " << pte_address << ", PTE is " << std::hex << pte.GetData();

	// TODO: if loadpte violates pmp or pma, raise access exception

	if(pte.GetV() == 0 || (pte.GetR() == 0 && pte.GetW() == 1)) {
		// TODO: page fault
		PageInfo pi;
		pi.Present = false;

		LC_DEBUG1(LogRiscVMMU) << "PTE is invalid/not present";

		return {pte_address, pi};
	}

	if(pte.GetR() || pte.GetX()) {
		// leaf entry, we're done.
		LC_DEBUG1(LogRiscVMMU) << "PTE is leaf, ppn is " << pte.GetPPN();

		PageInfo pi;
		pi.Present = true;
		pi.KernelCanRead = pte.GetR();
		pi.KernelCanWrite = pte.GetW();
		pi.KernelCanExecute = pte.GetX();
		if(pte.GetU()) {
			pi.UserCanRead = pi.KernelCanRead;
			pi.UserCanWrite = pi.KernelCanWrite;
			pi.UserCanExecute = pi.KernelCanExecute;
		}
		pi.phys_addr = Address(pte.GetPPN() << 12);

		switch(GetMode()) {
			case Sv39:
				switch(level) {
					case 0:
						pi.mask = ~0xfff;
						break;
					case 1:
						pi.mask = ~0x1fffff;
						break;
					case 2:
						pi.mask = ~0x3fffffff;
						break;
					default:
						UNEXPECTED;
				}
				break;
			default:
				UNIMPLEMENTED;
		}

		return {pte_address, pi};
	} else {
		level -= 1;

		if(level < 0) {
			// TODO: raise a page fault exception
			PageInfo pi;
			pi.Present = false;
			return {pte_address, pi};
		}

		return GetInfoLevel(virt_addr, Address(pte.GetPPN() * Address::PageSize), level);
	}
}

RiscVMMU::PTE RiscVMMU::LoadPTE(Mode mode, Address address)
{
	switch(mode) {
		case Sv32: {
			uint32_t data;
			GetPhysMem()->Read32(address, data);
			return PTE(data);
		}
		case Sv39:
		case Sv48: {
			uint64_t data;
			GetPhysMem()->Read64(address, data);
			return PTE(data);
		}
		default:
			UNEXPECTED;
	}
}

Address::underlying_t RiscVMMU::GetVPN(Mode mode, Address addr, int vpn)
{
	switch(mode) {
		case Sv32:
			switch(vpn) {
				case 0:
					return (addr.Get() >> 12) & 0x3ff;
				case 1:
					return (addr.Get() >> 22) & 0x3ff;
				default:
					UNEXPECTED;
			}
		case Sv39:
			switch(vpn) {
				case 0:
					return (addr.Get() >> 12) & 0x1ff;
				case 1:
					return (addr.Get() >> 21) & 0x1ff;
				case 2:
					return (addr.Get() >> 30) & 0x1ff;
				default:
					UNEXPECTED;
			}
		case Sv48:
			switch(vpn) {
				case 0:
					return (addr.Get() >> 12) & 0x1ff;
				case 1:
					return (addr.Get() >> 21) & 0x1ff;
				case 2:
					return (addr.Get() >> 30) & 0x1ff;
				case 3:
					return (addr.Get() >> 39) & 0x1ff;
				default:
					UNEXPECTED;
			}
		default:
			UNIMPLEMENTED;
	}
}

Address::underlying_t RiscVMMU::GetPTESize(Mode mode)
{
	switch(mode) {
		case Sv32:
			return 4;
		case Sv39:
		case Sv48:
			return 8;
		default:
			UNEXPECTED;
	}
}
