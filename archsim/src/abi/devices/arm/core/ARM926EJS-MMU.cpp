/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ARM926EJS-MMU.cpp
 *
 *  Created on: 17 Jun 2014
 *      Author: harry
 */

#include <cassert>

#include "abi/devices/MMU.h"
#include "abi/devices/arm/core/ArmControlCoprocessor.h"
#include "abi/memory/MemoryModel.h"

#include "core/thread/ThreadInstance.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"

UseLogContext(LogArmCoreDevice);
DeclareChildLogContext(LogArmMMU, LogArmCoreDevice, "MMU");
DeclareChildLogContext(LogArmMMUTlb, LogArmMMU, "TLB");
DeclareChildLogContext(LogArmMMUTxln, LogArmMMU, "TXLN");
DeclareChildLogContext(LogArmMMUFault, LogArmMMU, "Fault");
DeclareChildLogContext(LogArmMMUAccess, LogArmMMU, "Access");
DeclareChildLogContext(LogArmMMUInfo, LogArmMMU, "Info");

#define GETINFO_CHECK 0

using namespace archsim::abi::devices;
using archsim::Address;

class ARM926EJSMMU : public MMU
{
private:

	class tx_l1_descriptor
	{
	public:
		explicit tx_l1_descriptor(uint32_t data) : Bits(data), AP(0), Domain(0), C(false), B(false), BaseAddr(-1), Type(TXE_FAULT)
		{
			switch(data & 0x3) {
				case 0:
					ConstructFault(data);
					break;
				case 1:
					ConstructCoarse(data);
					break;
				case 2:
					ConstructSection(data);
					break;
				case 3:
					ConstructFine(data);
					break;
				default:
					assert(false && "Unknown translation type");
			}
		}

		enum TxEntryType {
			TXE_FAULT,
			TXE_Coarse,
			TXE_Section,
			TXE_Fine
		};

		std::string Print() const
		{
			std::stringstream str;
			str << "L1D " << std::hex << Bits << " = ";
			switch(Type) {
				case TXE_FAULT:
					str << "FAULT";
					break;
				case TXE_Coarse:
					str << "Coarse";
					break;
				case TXE_Section:
					str << "Section";
					break;
				case TXE_Fine:
					str << "Fine";
					break;
				default:
					assert(false);
			}

			str << " 0x" << std::hex << std::setfill('0') << std::setw(8) << BaseAddr << " Domain:" << std::dec << std::setw(1) << (uint32_t) Domain << " AP:" << (uint32_t)AP;
			return str.str();
		}

		TxEntryType Type;
		uint32_t BaseAddr;
		uint32_t Bits;
		uint8_t Domain;
		uint8_t AP;
		bool C, B;

	private:


		void ConstructFault(uint32_t data)
		{
			Type = TXE_FAULT;
			BaseAddr = 0;
			Domain = 0;
			AP = 0;
			C = 0;
			B = 0;
		}

		void ConstructCoarse(uint32_t data)
		{
			Type = TXE_Coarse;
			BaseAddr = data & 0xfffffc00;
			Domain = (data >> 5) & 0xf;
		}

		void ConstructSection(uint32_t data)
		{
			Type = TXE_Section;
			BaseAddr = data & 0xfff00000;
			Domain = (data >> 5) & 0xf;
			AP = (data & 0xc00) >> 10;
			C = (data & 0x8) != 0;
			B = (data & 0x4) != 0;

			LC_DEBUG3(LogArmMMUAccess) << "Constructed section descriptor " << Print();
		}

		void ConstructFine(uint32_t data)
		{
			Type = TXE_Fine;
			BaseAddr = data & 0xfffff000;
			Domain = (data >> 5) & 0xf;
		}
	};

	class tx_l2_descriptor
	{
	public:
		enum TxEntryType {
			TXE_FAULT,
			TXE_Large,
			TXE_Small,
			TXE_Tiny
		};

		explicit tx_l2_descriptor(uint32_t data) : bits(data)
		{
			switch(data & 0x3) {
				case 0x0:
					base_addr = ap0 = ap1 = ap2 = ap3 = c = b = 0;
					type = TXE_FAULT;
					return;
				case 0x1:
					ConstructLarge(data);
					return;
				case 0x2:
					ConstructSmall(data);
					return;
				case 0x3:
					ConstructTiny(data);
					return;
				default:
					assert(false);
			}
		}

		void ConstructLarge(uint32_t data)
		{
			type = TXE_Large;
			base_addr = data & 0xffff0000;

			ap0 = (data & 0x030) >> 4;
			ap1 = (data & 0x0c0) >> 6;
			ap2 = (data & 0x300) >> 8;
			ap3 = (data & 0xc00) >> 10;

			c = b = false;
		}

		void ConstructSmall(uint32_t data)
		{
			type = TXE_Small;
			base_addr = data & 0xfffff000;

			ap0 = (data & 0x030) >> 4;
			ap1 = (data & 0x0c0) >> 6;
			ap2 = (data & 0x300) >> 8;
			ap3 = (data & 0xc00) >> 10;

			c = b = false;
		}

		void ConstructTiny(uint32_t data)
		{
			assert(false);
		}

		std::string Print() const
		{
			std::stringstream str;
			str << "L2D " << std::hex << bits << " = ";
			switch(type) {
				case TXE_FAULT:
					str << "FAULT";
					break;
				case TXE_Large:
					str << "Large";
					break;
				case TXE_Small:
					str << "Small";
					break;
				case TXE_Tiny:
					str << "Tiny";
					break;
				default:
					assert(false);
			}

			str << " 0x" << std::hex << std::setw(8) << std::setfill('0') << base_addr << " AP0:" << std::dec << std::setw(1) << (uint32_t)ap0;

			return str.str();
		}

		TxEntryType type;
		uint32_t base_addr;
		uint32_t bits;
		uint8_t ap3, ap2, ap1, ap0;
		bool c, b;

	};

	enum tlb_entry_state {
		TLB_INVALID,
		TLB_VALID
	};

	enum tlb_access_perms {
		TLB_USR_READ = 1,
		TLB_USR_WRITE = 2,
		TLB_KERNEL_READ = 4,
		TLB_KERNEL_WRITE = 8
	};

	struct tlb_entry {
		uint32_t virt_addr_page;
		uint32_t phys_addr_page;
		bool kernel_only;
		bool read_only;
		tlb_entry_state state;
	};

public:
	uint32_t faulting_domain;

	bool Initialise()
	{
		FlushCaches();
		cocoprocessor = (ArmControlCoprocessor*)Manager->GetDeviceByName("coprocessor");
		assert(cocoprocessor);
		return true;
	}

	void FlushCaches() override
	{
	}


	void Evict(Address virt_addr) override
	{
		// TODO
	}

	void handle_result(TranslateResult result, archsim::core::thread::ThreadInstance *cpu, Address mva, const struct AccessInfo info)
	{
		uint32_t new_fsr = 0;

		if(result != TXLN_OK) {
			LC_DEBUG1(LogArmMMU) << "Handling result " << result << " for access to " << std::hex << std::setw(8) << std::setfill('0') << mva << " for access " << info << " " << cpu->GetExecutionRing();
		}

//		bool is_kernel = permissions & 0x2;
		bool is_write = info.Write;

		//if(is_write) new_fsr |= 0x800;

		new_fsr |= faulting_domain << 4;

		switch(result) {
			case TXLN_OK:
				return;
			case TXLN_FAULT_SECTION:
				new_fsr |= 0x5;
				break;
			case TXLN_FAULT_PAGE:
				new_fsr |= 0x7;
				break;
			case TXLN_ACCESS_DOMAIN_SECTION:
				new_fsr |= 0x9;
				break;
			case TXLN_ACCESS_DOMAIN_PAGE:
				new_fsr |= 0xb;
				break;
			case TXLN_ACCESS_PERMISSION_SECTION:
				new_fsr |= 0xd;
				break;
			case TXLN_ACCESS_PERMISSION_PAGE:
				new_fsr |= 0xf;
				break;
			default:
				assert(false);
		}

		LC_DEBUG2(LogArmMMU) << "Writing fault status bits";
		cocoprocessor->set_fsr(new_fsr);
		cocoprocessor->set_far(mva.Get());
	}


	void fill_access_perms(PageInfo &pi, uint32_t AP)
	{
		pi.UserCanRead = pi.UserCanWrite = pi.KernelCanRead = pi.KernelCanWrite = 0;
		switch(AP) {
			case 0:
				if(cocoprocessor->get_cp1_R()) pi.UserCanRead = pi.KernelCanRead = 1;
				else if(cocoprocessor->get_cp1_S()) pi.KernelCanRead = 1;
				break;
			case 1:
				pi.KernelCanRead = pi.KernelCanWrite = 1;
				break;
			case 2:
				pi.KernelCanRead = pi.KernelCanWrite = pi.UserCanRead = 1;
				break;
			case 3:
				pi.KernelCanRead = pi.KernelCanWrite = pi.UserCanRead = pi.UserCanWrite = 1;
				break;
			default:
				assert(!"Unknown access type");
		}

	}

	const PageInfo GetSectionInfo(const tx_l1_descriptor &descriptor)
	{
		PageInfo pi;
		pi.Present = 1;
		pi.phys_addr = Address(descriptor.BaseAddr);
		pi.mask = 0x000fffff;

		uint32_t dacr = get_dacr();
		dacr >>= descriptor.Domain*2;
		dacr &= 0x3;

		switch(dacr) {
			case 0:
				pi.KernelCanRead = pi.KernelCanWrite = pi.UserCanRead = pi.UserCanWrite = 0;
				break;
			case 1:
				fill_access_perms(pi, descriptor.AP);
				break;
			case 3:
				pi.KernelCanRead = pi.KernelCanWrite = pi.UserCanRead = pi.UserCanWrite = 1;
				break;
			default:
				assert(!"Unimplemented");
		}

		return pi;
	}



	const PageInfo GetCoarseInfo(Address mva, const tx_l1_descriptor &descriptor)
	{
		PageInfo pi;

		const tx_l2_descriptor l2_descriptor = get_l2_descriptor_coarse(mva, descriptor);
		if(l2_descriptor.type == tx_l2_descriptor::TXE_FAULT) {
			pi.Present = false;
			return pi;
		}

		pi.Present = true;

		uint32_t dacr = get_dacr();
		dacr >>= descriptor.Domain*2;
		dacr &= 0x3;

		switch(dacr) {
			case 0:
				pi.KernelCanRead = pi.KernelCanWrite = pi.UserCanRead = pi.UserCanWrite = 0;
				break;
			case 1:
				fill_access_perms(pi, l2_descriptor.ap0);
				break;
			case 3:
				pi.KernelCanRead = pi.KernelCanWrite = pi.UserCanRead = pi.UserCanWrite = 1;
				break;
			default:
				assert(false);
		}

		switch(l2_descriptor.type) {
			case tx_l2_descriptor::TXE_Large:
				LC_DEBUG2(LogArmMMUTxln) << "L2 lookup resulted in large page descriptor";
				pi.phys_addr = Address((l2_descriptor.base_addr & 0xffff0000) | (mva.Get() & 0x0000ffff));
				pi.mask = 0xffff;
				break;
			case tx_l2_descriptor::TXE_Small:
				LC_DEBUG2(LogArmMMUTxln) << "L2 lookup resulted in small page descriptor";
				pi.phys_addr = Address((l2_descriptor.base_addr & 0xfffff000) | (mva.Get() & 0x00000fff));
				pi.mask = 0xfff;
				break;
			default:
				assert(false);
		}

		return pi;
	}

	const PageInfo GetInfo(Address virt_addr) override
	{
		LC_DEBUG1(LogArmMMUInfo) << "Getting info for MVA " << std::hex << virt_addr;

		if(!is_enabled()) {
			LC_DEBUG1(LogArmMMUInfo) << " - MMU is disabled. Returning identity mapping.";
			PageInfo pi;
			pi.phys_addr = virt_addr;
			pi.mask = 0xf;
			pi.UserCanRead = true;
			pi.UserCanWrite = true;
			pi.KernelCanRead = true;
			pi.KernelCanWrite = true;
			pi.Present = true;
			return pi;
		}

		tx_l1_descriptor first_level_lookup = get_l1_descriptor(virt_addr);

		switch(first_level_lookup.Type) {
			case tx_l1_descriptor::TXE_FAULT: {
				PageInfo pi;
				pi.Present = false;
				return pi;
			}
			case tx_l1_descriptor::TXE_Section:
				return GetSectionInfo(first_level_lookup);
			case tx_l1_descriptor::TXE_Coarse:
				return GetCoarseInfo(virt_addr, first_level_lookup);
			default:
				assert(!"Unimplemented");
				__builtin_unreachable();
		}

	}

	TranslateResult Translate(archsim::core::thread::ThreadInstance *cpu, Address mva, Address &phys_addr, const struct AccessInfo info) override
	{
		//Canary value to catch invalid accesses
		phys_addr = Address(0xf0f0f0f0);
		LC_DEBUG4(LogArmMMU) << "Translating MVA " << std::hex << mva;
		if(!is_enabled()) {
			phys_addr = mva;
			return TXLN_OK;
		}

		TranslateResult result = TXLN_FAULT_OTHER;

		tx_l1_descriptor first_level_lookup = get_l1_descriptor(mva);

		faulting_domain = first_level_lookup.Domain;

		switch(first_level_lookup.Type) {
			case tx_l1_descriptor::TXE_FAULT:
				LC_DEBUG2(LogArmMMUFault) << "L1 lookup resulted in section fault when translating " << std::hex << mva << " (" << first_level_lookup.Bits << ")";
				result = TXLN_FAULT_SECTION;
				break;
			case tx_l1_descriptor::TXE_Section:
				LC_DEBUG2(LogArmMMUTxln) << "L1 lookup resulted in section descriptor " << std::hex << mva << " (" << info << ")";
				result = translate_section(mva, first_level_lookup, phys_addr, info);
				break;
			case tx_l1_descriptor::TXE_Coarse:
				LC_DEBUG2(LogArmMMUTxln) << "L1 lookup resulted in coarse page descriptor " << std::hex << mva << " (" << info << ")";
				result = translate_coarse(mva, first_level_lookup, phys_addr, info);
				break;
			case tx_l1_descriptor::TXE_Fine:
				LC_DEBUG2(LogArmMMUTxln) << "L1 lookup resulted in fine page descriptor";
				result = translate_fine(mva, first_level_lookup, phys_addr, info);
				break;
			default:
				assert(false && "Unknown access type");
		}

		if(info.SideEffects) handle_result(result, cpu, mva, info);

#if defined(GETINFO_CHECK)
#if GETINFO_CHECK
		PageInfo pi = GetInfo(mva);
		switch(result) {
			case TXLN_OK:
				if(permissions & 0x2) {
					if(permissions & 0x1) assert(pi.KernelCanWrite);
					else assert(pi.UserCanWrite);
				} else {
					if(permissions & 0x1) assert(pi.KernelCanRead);
					else assert(pi.UserCanRead);
				}
				break;
			case TXLN_FAULT_OTHER:
			case TXLN_FAULT_PAGE:
			case TXLN_FAULT_SECTION:
				assert(!pi.Present);
				break;
			case TXLN_ACCESS_DOMAIN_PAGE:
			case TXLN_ACCESS_DOMAIN_SECTION:
			case TXLN_ACCESS_PERMISSION_PAGE:
			case TXLN_ACCESS_PERMISSION_SECTION:
				if(permissions & 0x2) {
					if(permissions & 0x1) assert(!pi.KernelCanWrite);
					else assert(!pi.UserCanWrite);
				} else {
					if(permissions & 0x1) assert(!pi.KernelCanRead);
					else assert(!pi.UserCanRead);
				}
				break;
			default:
				assert(false);
				break;
		}
#endif
#endif

		return result;
	}

private:

	ArmControlCoprocessor *cocoprocessor;

	inline uint32_t page_of(uint32_t addr)
	{
		return addr >> 12;
	}
	inline uint32_t page_start_of(uint32_t addr)
	{
		return page_of(addr) << 12;
	}


	uint32_t get_dacr() const
	{
		return cocoprocessor->get_dacr();
	}

	Address get_tx_table_base(Address mva) const
	{
		assert(cocoprocessor);
		return Address(cocoprocessor->get_ttbr());
	}

	tx_l1_descriptor get_l1_descriptor(Address mva)
	{
		Address tx_table_base = get_tx_table_base(mva);
		uint32_t table_idx = (mva.Get() & 0xfff00000) >> 20;
		uint32_t descriptor_addr = tx_table_base.Get() | (table_idx << 2);

		uint32_t data;
		LC_DEBUG4(LogArmMMU) << "Reading L1 descriptor from " << std::hex << descriptor_addr;
		GetPhysMem()->Read32(Address(descriptor_addr), data);
		tx_l1_descriptor descriptor(data);
		LC_DEBUG4(LogArmMMU) << "Got descriptor "<< descriptor.Print();
		return descriptor;
	}

	tx_l2_descriptor get_l2_descriptor_coarse(Address mva, const tx_l1_descriptor &l1_descriptor)
	{
		uint32_t base_addr = l1_descriptor.BaseAddr;
		uint32_t l2_table_index = (mva.Get() & 0x000ff000) >> 12;
		uint32_t l2_desc_addr = (base_addr & 0xfffffc00) | (l2_table_index << 2);

		uint32_t data;
		LC_DEBUG4(LogArmMMU) << "Reading Coarse L2 descriptor from " << std::hex << l2_desc_addr;
		GetPhysMem()->Read32(Address(l2_desc_addr), data);

		tx_l2_descriptor descriptor(data);
		LC_DEBUG4(LogArmMMU) << "Got descriptor " << descriptor.Print();

		return descriptor;
	}

	bool allow_access(uint32_t AP, bool kernel_mode, bool is_write)
	{
		switch(AP) {
			case 0:
				if(cocoprocessor->get_cp1_R()) return !is_write;
				else if(cocoprocessor->get_cp1_S()) return kernel_mode && !is_write;
				return false;
			case 1:
				return kernel_mode;
			case 2:
				return kernel_mode || (!is_write);
			case 3:
				return true;
			default:
				assert(!"Unknown access permission type");
				return false;
		}
	}

	TranslateResult translate_section(Address mva, tx_l1_descriptor &section_descriptor, Address &out_phys_addr, const struct AccessInfo info)
	{
		uint32_t section_base_addr = section_descriptor.BaseAddr;
		uint32_t section_index = mva.Get() & 0xfffff;

		//TODO: Access permissions
		//Check domain
		uint8_t region_domain = section_descriptor.Domain;
		uint32_t dacr = get_dacr();
		dacr >>= region_domain*2;
		dacr &= 0x3;

		bool kernel_mode = info.Kernel;
		bool is_write = info.Write;

		switch(dacr) {
			case 0:
				LC_DEBUG1(LogArmMMUAccess) << "A section domain access fault was triggered";
				faulting_domain = section_descriptor.Domain;
				return TXLN_ACCESS_DOMAIN_SECTION;
			case 1:
				//Check access permissions
				LC_DEBUG2(LogArmMMUAccess) << "Regular permission checks were performed with AP " << std::hex << section_descriptor.AP;
				if(!allow_access(section_descriptor.AP, kernel_mode, is_write)) {
					LC_DEBUG1(LogArmMMUAccess) << "A section access fault was triggered, AP:" << (uint32_t)section_descriptor.AP << ", K:" << kernel_mode << ", W:" << is_write;
					faulting_domain = section_descriptor.Domain;
					return TXLN_ACCESS_PERMISSION_SECTION;
				}
				break;
			case 3:
				//Always allow access
				LC_DEBUG2(LogArmMMUAccess) << "No permission checks were performed";
				break;
		}

		out_phys_addr = Address(section_base_addr | section_index);
//		update_tlb_entry(mva, out_phys_addr, kernel_mode, !is_write);
		return TXLN_OK;
	}

	TranslateResult translate_coarse(Address mva, tx_l1_descriptor &l1_descriptor, Address &out_phys_addr, const struct AccessInfo info)
	{
		tx_l2_descriptor l2_desc = get_l2_descriptor_coarse(mva, l1_descriptor);
		uint8_t region_domain = l1_descriptor.Domain;

		faulting_domain = region_domain;

		if(l2_desc.type == tx_l2_descriptor::TXE_FAULT) {
			LC_DEBUG1(LogArmMMUFault) << "L2 lookup resulted in page fault when mapping " << std::hex << mva;
			LC_DEBUG1(LogArmMMUFault) << "L1 descriptor: " << std::hex << l1_descriptor.Print();
			LC_DEBUG1(LogArmMMUFault) << "L2 descriptor: " << std::hex << l2_desc.Print();
			return TXLN_FAULT_PAGE;
		}

		//Check domain

		uint32_t dacr = get_dacr();
		uint32_t full_dacr = get_dacr();
		dacr >>= region_domain*2;
		dacr &= 0x3;

		bool kernel_mode = info.Kernel;
		bool is_write = info.Write;

		switch(dacr) {
			case 0:
				LC_DEBUG1(LogArmMMUAccess) << "L2 lookup results in domain access fault";
				return TXLN_ACCESS_DOMAIN_PAGE;
			case 1:
				if(!allow_access(l2_desc.ap0, kernel_mode, is_write)) {
					LC_DEBUG1(LogArmMMUAccess) << "L2 lookup results in permission access fault when mapping " << std::hex << mva << ", descriptor:" << std::hex << (uint32_t)l2_desc.bits << ", K:" << kernel_mode << ", W:" << is_write << ", AP:" << (uint32_t)l2_desc.ap0;
					LC_DEBUG1(LogArmMMUAccess) << "L1 descriptor " << std::hex << l1_descriptor.Bits;
					LC_DEBUG1(LogArmMMUAccess) << "domain:" << (uint32_t)region_domain << " full dacr " << std::hex << full_dacr;
					return TXLN_ACCESS_PERMISSION_PAGE;
				}
				break;
			case 3:
				LC_DEBUG1(LogArmMMUAccess) << "L2 lookup skipped checking access permissions";
				break;
		}

		switch(l2_desc.type) {
			case tx_l2_descriptor::TXE_Large:
				LC_DEBUG2(LogArmMMUTxln) << "L2 lookup resulted in large page descriptor";
				out_phys_addr = Address((l2_desc.base_addr & 0xffff0000) | (mva.Get() & 0x0000ffff));
				break;
			case tx_l2_descriptor::TXE_Small:
				LC_DEBUG2(LogArmMMUTxln) << "L2 lookup resulted in small page descriptor";
				out_phys_addr = Address((l2_desc.base_addr & 0xfffff000) | (mva.Get() & 0x00000fff));
				break;
			default:
				assert(false);
		}

//		update_tlb_entry(mva, out_phys_addr, kernel_mode, !is_write);

		return TXLN_OK;
	}

	TranslateResult translate_fine(Address mva, tx_l1_descriptor &sction_descriptor, Address &out_phys_addr, const struct AccessInfo info)
	{
		assert(!"Fine lookups not implemented");
		return TXLN_FAULT_OTHER;
	}
};


RegisterComponent(Device, ARM926EJSMMU, "ARM926EJSMMU", "MMU");
