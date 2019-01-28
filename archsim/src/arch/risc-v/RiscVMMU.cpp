/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/risc-v/RiscVMMU.h"

using namespace archsim;
using namespace archsim::arch::riscv;
using namespace archsim::abi::devices;

bool RiscVMMU::Initialise()
{
	return true;
}

const PageInfo RiscVMMU::GetInfo(Address virt_addr)
{
	PageInfo pi;

	pi.phys_addr = virt_addr;

	return pi;
}

MMU::TranslateResult RiscVMMU::Translate(archsim::core::thread::ThreadInstance* cpu, Address virt_addr, Address& phys_addr, AccessInfo info)
{
	auto pinfo = GetInfo(virt_addr);

	phys_addr = pinfo.phys_addr;

	return MMU::TXLN_OK;
}
