/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ARMv6-MMU.cpp
 *
 *  Created on: 12 Jan 2016
 *      Author: kuba
 */

//#include "abi/devices/arm/core/ARMv6-MMU.h"
#include "abi/devices/MMU.h"
#include "abi/devices/arm/core/ArmControlCoprocessorv6.h"
#include "abi/memory/MemoryModel.h"
#include "core/thread/ThreadInstance.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"

UseLogContext(LogArmCoreDevice);
DeclareChildLogContext(LogArmMMUv6, LogArmCoreDevice, "MMUv6");
DeclareChildLogContext(LogArmMMUTlbv6, LogArmMMUv6, "TLBv6");
DeclareChildLogContext(LogArmMMUTxlnv6, LogArmMMUv6, "TXLNv6");
DeclareChildLogContext(LogArmMMUFaultv6, LogArmMMUv6, "Faultv6");
DeclareChildLogContext(LogArmMMUAccessv6, LogArmMMUv6, "Accessv6");
DeclareChildLogContext(LogArmMMUInfov6, LogArmMMUv6, "Infov6");

#define GETINFO_CHECK 0

class ARMv6MMU;

namespace archsim
{
	namespace abi
	{
		namespace devices
		{


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
							ConstructFault(data);
							break;
						default:
							assert(false && "Unknown translation type");
					}
				}

				enum TxEntryType {
					TXE_FAULT,
					TXE_Coarse,
					TXE_Section,
					TXE_Supersection
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
						case TXE_Supersection:
							str << "Supersection";
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
				uint8_t SBZ0, SBZ1;
				uint8_t P;
				uint8_t NS;
				uint8_t TEX;
				uint8_t XN;
				uint8_t NG;
				uint8_t S;
				uint8_t APX;
				uint8_t Supersection;
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

					LC_DEBUG3(LogArmMMUAccessv6) << "Constructed fault descriptor " << Print();
				}

				void ConstructCoarse(uint32_t data)
				{
					Type = TXE_Coarse;
					BaseAddr = data & 0xfffffc00;
					P = (data >> 8) & 1;
					Domain = (data >> 5) & 0xf;
					SBZ0 = (data >> 4) & 1;
					NS = (data >> 3) & 1;
					SBZ1 = (data >> 2) & 1;

					LC_DEBUG3(LogArmMMUAccessv6) << "Constructed coarse descriptor " << Print();
				}

				void ConstructSection(uint32_t data)
				{
					Type = TXE_Section;
					BaseAddr = data & 0xfff00000;
					NS = (data >> 19) & 1;
					Supersection = (data >> 17) & 1;
					NG = (data >> 17) & 1;
					S = (data >> 16) & 1;
					APX = (data >> 15) & 1;
					TEX = (data >> 12) & 0x7;
					AP = (data >> 10) & 0x3;
					Domain = (data >> 5) & 0xf;
					P = (data >> 9) & 1;
					XN = (data >> 4) & 1;
					C = data & 0x8;
					B = data & 0x4;

					AP = AP | (APX << 2);

					if(Supersection) assert(false &&  "Supersections unimplemented");

					LC_DEBUG3(LogArmMMUAccessv6) << "Constructed section descriptor " << Print();
				}

				void ConstructSupersection(uint32_t data)
				{
					Type = TXE_Section;
					BaseAddr = data & 0xff000000;
					SBZ0 = (data >> 20) & 0x8;
					NS = (data >> 19) & 1;
					Supersection = 1;
					NG = (data >> 17) & 1;
					S = (data >> 16) & 1;
					APX = (data >> 15) & 1;
					TEX = (data >> 12) & 0x7;
					AP = (data >> 10) & 0x3;
					Domain = (data >> 5) & 0xf;
					P = (data >> 9) & 1;
					XN = (data >> 4) & 1;
					C = data & 0x8;
					B = data & 0x4;

					LC_DEBUG3(LogArmMMUAccessv6) << "Constructed supersection descriptor " << Print();
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
							base_addr = bits = xn = s = ng = ap = apx = tex = sbz = c = b = 0;
							type = TXE_FAULT;
							return;
						case 0x1:
							ConstructLarge(data);
							return;
						case 0x2:
						case 0x3:
							ConstructSmall(data);
							return;

						default:
							LC_DEBUG3(LogArmMMUFaultv6) << "data: " << std::hex << data;
							assert(false);
					}
				}

				void ConstructLarge(uint32_t data)
				{
					type = TXE_Large;
					base_addr = data & 0xffff0000;

					xn = (data >> 15) & 1;
					tex = (data >> 12) & 0x7;
					ng = (data >> 11) & 1;
					s = (data >> 10) & 1;

					apx = (data >> 9) & 1;
					ap = (data >> 4) & 0x3;
					ap |= apx << 2;

					c = data & 0x8;
					b = data & 0x4;
				}

				void ConstructSmall(uint32_t data)
				{
					type = TXE_Small;
					base_addr = data & 0xfffff000;

					xn = (data >> 0) & 1;
					tex = (data >> 6) & 0x7;
					ng = (data >> 11) & 1;
					s = (data >> 10) & 1;

					apx = (data >> 9) & 1;
					ap = (data >> 4) & 0x3;
					ap |= apx << 2;

					c = data & 0x8;
					b = data & 0x4;
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

					str << " 0x" << std::hex << std::setw(8) << std::setfill('0') << base_addr << " AP0:" << std::dec << std::setw(1) << (uint32_t)ap;

					return str.str();
				}

				TxEntryType type;
				uint32_t base_addr;
				uint32_t bits;
				uint8_t xn, s, ng;
				uint8_t ap, apx;
				uint8_t tex, sbz;
				bool c, b;
			};

			class ARMv6MMU : public MMU
			{
			public:
				uint32_t faulting_domain;

				bool Initialise()
				{
					FlushCaches();
					cocoprocessor = (ArmControlCoprocessorv6*)Manager->GetDeviceByName("coprocessor");
					assert(cocoprocessor);
					return true;
				}

				const PageInfo GetInfo(uint32_t virt_addr) override
				{
					LC_DEBUG1(LogArmMMUInfov6) << "Getting info for MVA " << std::hex << virt_addr;

					if(!is_enabled()) {
						LC_DEBUG1(LogArmMMUInfov6) << " - MMU is disabled. Returning identity mapping.";
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

					LC_DEBUG4(LogArmMMUInfov6) << "GetInfo";

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

				const PageInfo GetSectionInfo(const tx_l1_descriptor &descriptor)
				{
					PageInfo pi;
					pi.Present = 1;
					pi.phys_addr = descriptor.BaseAddr;
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

				const PageInfo GetCoarseInfo(uint32_t mva, const tx_l1_descriptor &descriptor)
				{
					PageInfo pi;

					LC_DEBUG4(LogArmMMUInfov6) << "GetCoarseInfo";
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
							fill_access_perms(pi, l2_descriptor.ap);
							break;
						case 3:
							pi.KernelCanRead = pi.KernelCanWrite = pi.UserCanRead = pi.UserCanWrite = 1;
							break;
						default:
							assert(false);
					}

					switch(l2_descriptor.type) {
						case tx_l2_descriptor::TXE_Large:
							LC_DEBUG2(LogArmMMUTxlnv6) << "L2 lookup resulted in large page descriptor";
							pi.phys_addr = (l2_descriptor.base_addr & 0xffff0000) | (mva & 0x0000ffff);
							pi.mask = 0xffff;
							break;
						case tx_l2_descriptor::TXE_Small:
							LC_DEBUG2(LogArmMMUTxlnv6) << "L2 lookup resulted in small page descriptor";
							pi.phys_addr = (l2_descriptor.base_addr & 0xfffff000) | (mva & 0x00000fff);
							pi.mask = 0xfff;
							break;
						default:
							assert(false);
					}

					return pi;
				}

				void fill_access_perms(PageInfo &pi, uint32_t AP)
				{
					pi.UserCanRead = pi.UserCanWrite = pi.KernelCanRead = pi.KernelCanWrite = 0;
					switch(AP) {
						case 0:
							// no access
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
						case 4:
							// no access
							break;
						case 5:
							pi.KernelCanRead = 1;
							break;
						case 6:
							pi.KernelCanRead = pi.UserCanRead = 1;
							break;
						case 7:
							pi.KernelCanRead = pi.UserCanRead = 1;
							break;
						default:
							assert(!"Unknown access type");
					}

				}

				TranslateResult Translate(archsim::core::thread::ThreadInstance *cpu, uint32_t virt_addr, uint32_t &phys_addr, AccessInfo info) override
				{
					//Canary value to catch invalid accesses
					phys_addr = 0xf0f0f0f0;
					LC_DEBUG4(LogArmMMUv6) << "Translating MVA " << std::hex << virt_addr;
					if(!is_enabled()) {
						phys_addr = virt_addr;
						return TXLN_OK;
					}

					TranslateResult result = TXLN_FAULT_OTHER;

					/* First-level descriptor indicates whether the access is to a section or to a page table.
					 * If the access is to a page table, the MMU determines the page table type and fetches
					 * a second-level descriptor.
					 */

					tx_l1_descriptor first_level_lookup = get_l1_descriptor(virt_addr);

					faulting_domain = first_level_lookup.Domain;

					switch(first_level_lookup.Type) {
						case tx_l1_descriptor::TXE_FAULT:
							LC_DEBUG4(LogArmMMUFaultv6) << "L1 lookup resulted in section fault when translating " << std::hex << virt_addr << " (" << first_level_lookup.Bits << ")";
							result = TXLN_FAULT_SECTION;
							break;
						case tx_l1_descriptor::TXE_Section:
							LC_DEBUG4(LogArmMMUTxlnv6) << "L1 lookup resulted in section descriptor " << std::hex << virt_addr << " (" << info << ")";
							result = translate_section(virt_addr, first_level_lookup, phys_addr, info);
							break;
						case tx_l1_descriptor::TXE_Supersection:
							LC_DEBUG4(LogArmMMUTxlnv6) << "mmu-v6: we do not support supersections\n";
							assert(false);
							break;
						case tx_l1_descriptor::TXE_Coarse:
							LC_DEBUG4(LogArmMMUTxlnv6) << "L1 lookup resulted in coarse page descriptor " << std::hex << virt_addr << " (" << info << ")";
							result = translate_coarse(virt_addr, first_level_lookup, phys_addr, info);
							break;
						default:
							assert(false && "Unknown access type");
					}

					if(info.SideEffects) handle_result(result, cpu, virt_addr, info);

					return result;
				}

				void handle_result(TranslateResult result, archsim::core::thread::ThreadInstance *cpu, uint32_t mva, const struct AccessInfo info)
				{
					uint32_t new_fsr = 0;

					if(result != TXLN_OK) {
						LC_DEBUG1(LogArmMMUv6) << "Handling result " << result << " for access to " << std::hex << std::setw(8) << std::setfill('0') << mva << " for access " << info << " " << cpu->GetExecutionRing();
					}

					//		bool is_kernel = permissions & 0x2;
					bool is_write = info.Write;

					if(is_write) new_fsr |= 0x800;

					if(!info.Fetch)
						new_fsr |= (faulting_domain & 0xf) << 4;

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

					LC_DEBUG1(LogArmMMUFaultv6) << "MVA: " << std::hex << std::setw(8) << std::setfill('0') << mva << " FSR: " << new_fsr << " Info: " << info;
					LC_DEBUG2(LogArmMMUv6) << "Writing fault status bits";
					if(info.Fetch) {
						cocoprocessor->set_ifsr(new_fsr);
						cocoprocessor->set_ifar(mva);
					} else {
						cocoprocessor->set_dfsr(new_fsr);
						cocoprocessor->set_dfar(mva);
					}
				}

			private:

				ArmControlCoprocessorv6 *cocoprocessor;

				tx_l1_descriptor get_l1_descriptor(uint32_t mva)
				{
					uint32_t tx_table_base = get_tx_table_base(mva);
					uint32_t table_idx = (mva & 0xfff00000) >> 20;
					uint32_t descriptor_addr = tx_table_base | (table_idx << 2);

					uint32_t data;
					LC_DEBUG4(LogArmMMUv6) << "Reading L1 descriptor from " << std::hex << descriptor_addr;
					GetPhysMem()->Read32(descriptor_addr, data);
					LC_DEBUG4(LogArmMMUv6) << "Descriptor data"<< data;
					tx_l1_descriptor descriptor(data);
					LC_DEBUG4(LogArmMMUv6) << "Got descriptor "<< descriptor.Print();
					return descriptor;
				}

				uint32_t get_tx_table_base(uint32_t mva) const
				{
					assert(cocoprocessor);
					return cocoprocessor->get_ttbr() & ~0x3fff;
				}

				TranslateResult translate_section(uint32_t mva, tx_l1_descriptor &section_descriptor, uint32_t &out_phys_addr, const struct AccessInfo info)
				{
					uint32_t section_base_addr = section_descriptor.BaseAddr;
					uint32_t section_index = mva & 0xfffff;

					//TODO: Access permissions
					//Check domain
					uint8_t region_domain = section_descriptor.Domain;
					uint32_t dacr = get_dacr(); // DACR - Domain Access Control Register
					dacr >>= region_domain*2;
					dacr &= 0x3;

					bool kernel_mode = info.Kernel;
					bool is_write = info.Write;

					LC_DEBUG1(LogArmMMUAccessv6) << "Translate Section DACR: " << dacr;

					switch(dacr) {
						case 2:
							LC_DEBUG1(LogArmMMUAccessv6) << "A section domain access fault was triggered";
							faulting_domain = section_descriptor.Domain;
							return TXLN_ACCESS_DOMAIN_SECTION;
						case 1:
							//Check access permissions
							LC_DEBUG2(LogArmMMUAccessv6) << "Regular permission checks were performed with AP " << std::hex << section_descriptor.AP;
							if(!allow_access(section_descriptor, info)) {
								LC_DEBUG1(LogArmMMUAccessv6) << "A section access fault was triggered, AP:" << (uint32_t)section_descriptor.AP << ", K:" << kernel_mode << ", W:" << is_write;
								faulting_domain = section_descriptor.Domain;
								return TXLN_ACCESS_PERMISSION_SECTION;
							}
							break;
					}

					out_phys_addr = section_base_addr | section_index;
					//		update_tlb_entry(mva, out_phys_addr, kernel_mode, !is_write);
					return TXLN_OK;
				}

				TranslateResult translate_coarse(uint32_t mva, tx_l1_descriptor &l1_descriptor, uint32_t &out_phys_addr, const struct AccessInfo info)
				{
					tx_l2_descriptor l2_desc = get_l2_descriptor_coarse(mva, l1_descriptor);
					uint8_t region_domain = l1_descriptor.Domain;

					faulting_domain = region_domain;

					if(l2_desc.type == tx_l2_descriptor::TXE_FAULT) {
						LC_DEBUG2(LogArmMMUFaultv6) << "L2 lookup resulted in page fault when mapping " << std::hex << mva;
						LC_DEBUG3(LogArmMMUFaultv6) << "L1 descriptor: " << std::hex << l1_descriptor.Print();
						LC_DEBUG3(LogArmMMUFaultv6) << "L2 descriptor: " << std::hex << l2_desc.Print();
						return TXLN_FAULT_PAGE;
					}

					//Check domain

					uint32_t dacr = get_dacr();
					uint32_t full_dacr = get_dacr();
					dacr >>= region_domain*2;
					dacr &= 0x3;

					bool kernel_mode = info.Kernel;
					bool is_write = info.Write;
					bool is_fetch = info.Fetch;

					LC_DEBUG1(LogArmMMUAccessv6) << "Translate Coarse DACR: " << dacr;

					switch(dacr) {
						case 2:
							return TranslateResult::TXLN_ACCESS_DOMAIN_PAGE;
					}

					switch(l2_desc.type) {
						case tx_l2_descriptor::TXE_FAULT:
							LC_DEBUG1(LogArmMMUAccessv6) << "L2 lookup results in page fault.";
							return TranslateResult::TXLN_FAULT_PAGE;
						case tx_l2_descriptor::TXE_Small:
							LC_DEBUG2(LogArmMMUTxlnv6) << "L2 lookup resulted in small page descriptor";
							if (dacr == 1) {
								if (!allow_access(l2_desc, info)) {
									return TranslateResult::TXLN_ACCESS_PERMISSION_PAGE;
								}
							}
							out_phys_addr = (l2_desc.base_addr & 0xfffff000) | (mva & 0x00000fff);
							break;
						case tx_l2_descriptor::TXE_Large:
							LC_DEBUG2(LogArmMMUTxlnv6) << "L2 lookup resulted in large page descriptor";
							if (dacr == 1) {
								if (!allow_access(l2_desc, info)) {
									return TranslateResult::TXLN_ACCESS_PERMISSION_PAGE;
								}
							}
							out_phys_addr = (l2_desc.base_addr & 0xffff0000) | (mva & 0x0000ffff);
							break;
						default:
							assert(false);
					}

					//		update_tlb_entry(mva, out_phys_addr, kernel_mode, !is_write);

					return TXLN_OK;
				}

				uint32_t get_dacr() const
				{
					return cocoprocessor->get_dacr();
				}

				bool allow_access(tx_l2_descriptor &desc, const struct AccessInfo &info)
				{
					if(info.Fetch && desc.xn) return false;

					switch(desc.ap) {
						case 0:
							//			if(cocoprocessor->get_cp1_R()) return !is_write;
							//			else if(cocoprocessor->get_cp1_S()) return kernel_mode && !is_write;
							return false;
						case 1:
							return info.Kernel;
						case 2:
							return info.Kernel || (!info.Write);
						case 3:
							return true;
						case 4:
							return false;
						case 5:
							return info.Kernel && !info.Write;
						case 6:
							return !info.Write;
						case 7:
							return !info.Write;
						default:
							assert(!"Unknown access permission type");
							return false;
					}
				}

				bool allow_access(tx_l1_descriptor &desc, const struct AccessInfo &info)
				{
					if(info.Fetch && desc.XN) return false;

					switch(desc.AP) {
						case 0:
							//			if(cocoprocessor->get_cp1_R()) return !is_write;
							//			else if(cocoprocessor->get_cp1_S()) return kernel_mode && !is_write;
							return false;
						case 1:
							return info.Kernel;
						case 2:
							return info.Kernel || (!info.Write);
						case 3:
							return true;
						case 4:
							return false;
						case 5:
							return info.Kernel && !info.Write;
						case 6:
							return !info.Write;
						case 7:
							return !info.Write;
						default:
							assert(!"Unknown access permission type");
							return false;
					}
				}

				tx_l2_descriptor get_l2_descriptor_coarse(uint32_t mva, const tx_l1_descriptor &l1_descriptor)
				{
					uint32_t base_addr = l1_descriptor.BaseAddr;
					uint32_t l2_table_index = (mva >> 12) & 0xff;
					uint32_t l2_desc_addr = (base_addr & 0xfffffc00) | (l2_table_index << 2);

					uint32_t data;
					LC_DEBUG4(LogArmMMUv6) << "Reading Coarse L2 descriptor from " << std::hex << l2_desc_addr;
					GetPhysMem()->Read32(l2_desc_addr, data);

					LC_DEBUG4(LogArmMMUv6) << "Descriptor data: " << std::hex << data;

					tx_l2_descriptor descriptor(data);
					LC_DEBUG4(LogArmMMUv6) << "Got descriptor " << descriptor.Print();

					return descriptor;
				}
			};

			RegisterComponent(Device, ARMv6MMU, "ARMv6MMU", "MMU");

		}
	}
}

