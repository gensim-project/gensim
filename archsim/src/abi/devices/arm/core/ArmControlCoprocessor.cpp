/*
 * ArmCoprocessor.cpp
 *
 *  Created on: 18 Jun 2014
 *      Author: harry
 */
#include "system.h"

#include "abi/devices/MMU.h"
#include "abi/devices/PeripheralManager.h"
#include "abi/devices/arm/core/ArmControlCoprocessor.h"

#include "gensim/gensim_processor.h"

#include "translate/TranslationManager.h"

#include "util/LogContext.h"

#include <cassert>
#include <iomanip>

UseLogContext(LogArmCoreDevice);
UseLogContext(LogArmCoprocessor);
UseLogContext(LogArmCoprocessorCache);
UseLogContext(LogArmCoprocessorTlb);
UseLogContext(LogArmCoprocessorDomain);

using namespace archsim::abi::devices;

RegisterComponent(archsim::abi::devices::Device, ArmControlCoprocessor,
                  "arm926coprocessor", "ARMv5 ARM Control Coprocessor unit interface")
;

ArmControlCoprocessor::ArmControlCoprocessor() :
	cp1_M(false), cp1_S(false), cp1_R(false), far(0), fsr(0), ttbr(0), dacr(0xffffffff), V_descriptor(NULL)
{

}

bool ArmControlCoprocessor::access_cp0(bool is_read, uint32_t &data)
{
	if (!is_read && rn != 0 && rm != 0 && opc1 != 2 && opc2 != 0) {
		LC_ERROR(LogArmCoprocessor) << "Attempted invalid write to cp0";
		return false;
	}

	switch (rm) {
		case 0:
			switch (opc1) {
				case 0:
					switch (opc2) {
						case 0: // MAIN ID
							data = 0x41069265;		// ARMv5
//				data = 0x410fc083;		// Cortex A8
							return true;

						case 1: // CACHE TYPE
							data = 0x0f006006;	// ARMv5
//				data = 0x82048004;	// Cortex A8
							return true;

						case 2: // TCM STATUS
							data = 0x00004004; // ARMv5
//				data = 0;	// Cortex A8
							return true;

						case 3:	// TLB TYPE
							data = 0; //0x00202001;	// Cortex A8
							return true;

						case 5:	// MP ID
							data = 0;
							return true;
					}
					break;

				case 1:
					switch (opc2) {
						case 0: // CACHE SIZE IDENTIFICATION
							switch (CACHE_SIZE_SELECTION) {
								case 0:
									data = 0xe007e01a;
									return true;
								case 1:
									data = 0x2007e01a;
									return true;
								case 2:
									data = 0xf0000000;
									return true;
								default:
									data = 0;
									return true;
							}
							break;

						case 1:
							data = 0x0A000023;
							return true;
					}
					break;

				case 2:
					switch (opc2) {
						case 0: // CACHE SIZE SELECTION
							data = CACHE_SIZE_SELECTION;
							return true;
					}
					break;
			}
			break;

		case 1:
			switch (opc1) {
				case 0:
					switch (opc2) {
						case 0:
							//data = 0x00001031;
							data = 0x00001131;
							return true;

						case 4:
							//data = 0x01100003;
							data = 0x31100003;
							return true;

						case 5:
							data = 0x20000000;
							return true;
					}
					break;
			}
			break;

		case 2:
			switch (opc1) {
				case 0:
					switch (opc2) {
						case 0:
							data = 0x00101111;
							return true;

						case 3:
							data = 0x11112131;
							return true;

						case 5:
							data = 0x00000000;
							return true;
					}
					break;
			}
			break;
	}

	assert(false && "Unimplemented ID register");
	return false;
}

bool ArmControlCoprocessor::access_cp1(bool is_read, uint32_t &data)
{
	//System Control
	if (!is_read) {
		bool L2 = (data & (1 << 26)) != 0;
		bool EE = (data & (1 << 25)) != 0;
		bool VE = (data & (1 << 24)) != 0;
		bool XP = (data & (1 << 23)) != 0;
		bool U = (data & (1 << 22)) != 0;
		bool FI = (data & (1 << 21)) != 0;
		bool L4 = (data & (1 << 15)) != 0;
		bool RR = (data & (1 << 14)) != 0;
		bool V = (data & (1 << 13)) != 0;
		bool I = (data & (1 << 12)) != 0;
		bool Z = (data & (1 << 11)) != 0;
		bool F = (data & (1 << 10)) != 0;
		bool R = (data & (1 << 9)) != 0;
		bool S = (data & (1 << 8)) != 0;
		bool B = (data & (1 << 7)) != 0;
		bool L = (data & (1 << 6)) != 0;
		bool D = (data & (1 << 5)) != 0;
		bool P = (data & (1 << 4)) != 0;
		bool W = (data & (1 << 3)) != 0;
		bool C = (data & (1 << 2)) != 0;
		bool A = (data & (1 << 1)) != 0;
		bool M = (data & (1 << 0)) != 0;

		cp1_M = M;
		cp1_S = S;
		cp1_R = R;
		get_mmu()->set_enabled(M);

		UNIMPLEMENTED;
//		if(!V_descriptor) V_descriptor = &Manager->cpu.GetRegisterDescriptor("cpV");
//		uint8_t *v_reg =
//		    (uint8_t*) V_descriptor->DataStart;
//		*v_reg = V;

		LC_DEBUG1(LogArmCoprocessor) << "CP1 Write: "
		                             " L2:" << L2 << " EE:" << EE << " VE:" << VE << " XP:" << XP
		                             << " U:" << U << " FI:" << FI << " L4:" << L4 << " RR:" << RR
		                             << " V:" << V << " I:" << I << " Z:" << Z << " F:" << F << " R:"
		                             << R << " S:" << S << " B:" << B << " L:" << L << " D:" << D
		                             << " P:" << P << " W:" << W << " C:" << C << " A:" << A << " M:"
		                             << M << " 0x" << std::hex << std::setw(8) << std::setfill('0')
		                             << data << " at " << this->Manager->cpu.GetPC().Get();
	} else {
		data = 0;
		//NIBBLE 0
		data |= cp1_M << 0; // MMU
		data |= 0 << 1; //Strict alignment
		data |= 1 << 2; //L1 U$ or D$
		data |= 0 << 3; //Write buffer
		//NIBBLE 1
		data |= 0x7 << 4; //SBO
		data |= 0 << 7; //B (endianness)
		//NIBBLE 2
		data |= cp1_S << 8; //S (system protection)
		data |= cp1_R << 9; //R (rom protect)
		data |= 0 << 10; //F (implementation defined)
		data |= 0 << 11; //BPU
		//NIBBLE 3
		data |= 1 << 12; //I L1 I$

		UNIMPLEMENTED;
//		if(!V_descriptor) V_descriptor = &Manager->cpu.GetRegisterDescriptor("cpV");
		uint8_t *V =
		    (uint8_t*) V_descriptor->DataStart;
		assert(*V == 0 || *V == 1);
		data |= (uint32_t) ((uint32_t)*V) << 13; //V, exception vectors

		data |= 0 << 14; //RR
		data |= 0 << 15; //L4
		//NIBBLE 4
		data |= 1 << 16; //DT
		data |= 0 << 17; //SBZ
		data |= 0 << 18; //IT //QEMU HACK
		data |= 1 << 19; //SBZ //QEMU HACK
		//NIBBLE 5
		data |= 0 << 20; //ST
		data |= 0 << 21; //FI
		data |= 1 << 22; //U
		data |= 0 << 23; //XP
		//NIBBLE 6
		data |= 0 << 24; //VE
		data |= 0 << 25; //EE
		data |= 0 << 26; //L2

		LC_DEBUG1(LogArmCoprocessor) << "CP1 Read: "<< std::hex << std::setw(8) << std::setfill('0') << data;

	}
	return true;
}

bool ArmControlCoprocessor::access_cp2(bool is_read, uint32_t &data)
{
	//Translation table base

	if (!is_read) {

		switch (opc2) {
			case 0:
				ttbr = data;

				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);

				LC_DEBUG1(LogArmCoprocessorTlb)
				        << "Attempted write to TTBR0 "
				        << std::hex << data;
				break;
			case 1:
				LC_DEBUG1(LogArmCoprocessorTlb)
				        << "Attempted write to TTBR1 "
				        << std::hex <<data;
				break;
			case 2:
				LC_DEBUG1(LogArmCoprocessorTlb)
				        << "Attempted write to control reg "
				        << data;
				break;
			default:
				LC_WARNING(LogArmCoprocessorTlb)
				        << "Unknown access to Translation Table register";
				return false;
		}

	} else {
		if(opc2 == 0) data = ttbr;
		else LC_WARNING(LogArmCoprocessorTlb) << "Attempted read from unknown tlb field";
	}

	return true;
}
bool ArmControlCoprocessor::access_cp3(bool is_read, uint32_t &data)
{
	// Domain access control
	if(is_read) {
		LC_DEBUG1(LogArmCoprocessorDomain) << "Read of domain access control register";
		data = dacr;
	} else {
		LC_DEBUG1(LogArmCoprocessorDomain) << "Write of domain access control register";
		dacr = data;
		//		get_mmu()->FlushCaches();
		//		Manager->cpu.memory_model->FlushCaches();
	}
	return true;
}

// No cp4

bool ArmControlCoprocessor::access_cp5(bool is_read, uint32_t &data)
{
	//Data/unified access permission control
	//Instruction access permission control

	if (opc2 == 0) {
		if(!is_read) {
			set_dfsr(data);
		} else {
			LC_DEBUG1(LogArmCoprocessor) << "Reading from DFSR" << std::hex << dfsr;
			data = dfsr;
		}
	} else if (opc2 == 1) {
		if(!is_read) {
			set_ifsr(data);
		} else {
			LC_DEBUG1(LogArmCoprocessor) << "Reading from IFSR" << std::hex << ifsr;
			data = ifsr;
		}
	}


	return true;
}
bool ArmControlCoprocessor::access_cp6(bool is_read, uint32_t &data)
{
	//Data/unified region configuration
	//Instruction region configuration
	if (opc2 == 0) {
		if(!is_read) {
			set_dfar(data);
		} else {
			LC_DEBUG1(LogArmCoprocessor) << "Reading from DFAR " << std::hex << dfar;
			data = dfar;
		}
	} else if (opc2 == 2) {
		if(!is_read) {
			set_ifar(data);
		} else {
			LC_DEBUG1(LogArmCoprocessor) << "Reading from IFAR " << std::hex << ifar;
			data = ifar;
		}
	}

	return true;
}

bool ArmControlCoprocessor::access_cp7(bool is_read, uint32_t &data)
{
	// Cache management functions
	switch (rm) {
		case 0:
			if (opc2 == 4) {		//Wait for interrupt
				LC_DEBUG1(LogArmCoprocessor) << "Attempted WFI via MCR";
			} else {
				LC_WARNING(LogArmCoprocessor) << "Unknown c0 operation";
				return false;
			}
			break;
		case 5:
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Invalidate instruction cache";
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, NULL);
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorCache) << "Invalidate I$ line MVA "
					                                  << data;
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, NULL);
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Invalidate I$ line Set/way "
					        << data;
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, NULL);
					break;
				case 4:
					LC_DEBUG1(LogArmCoprocessorCache) << "Flush prefetch buffer";
					break;
				case 6:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Fluch branch target cache";
					break;
				case 7:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Flush branch target cache entry";
					break;
				default:
					LC_WARNING(LogArmCoprocessor) << "Unknown c5 operation";
					return false;
			}
			break;
		case 6:
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorCache) << "Invalidate D$";
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L1DCacheFlush, NULL);
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorCache) << "Invalidate D$ line MVA "
					                                  << data;
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L1DCacheFlush, NULL);
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Invalidate D$ line Set/way "
					        << data;
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L1DCacheFlush, NULL);
					break;
				default:
					LC_WARNING(LogArmCoprocessor) << "Unknown c6 operation";
					return false;
			}
			break;
		case 7:
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Invalidate all the things";
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, NULL);
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L1DCacheFlush, NULL);
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L2CacheFlush, NULL);
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Invalidate Unified $ line MVA "
					        << data;
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L2CacheFlush, NULL);
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Invalidate Unified $ line set/way "
					        << data;
					Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::L2CacheFlush, NULL);
					break;
			}
			break;
		case 10:
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorCache) << "Clean D$";
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorCache) << "Clean d$ line MVA "
					                                  << data;
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorCache) << "Clean d$ line set/way "
					                                  << data;
					break;
				case 3:
					LC_DEBUG1(LogArmCoprocessorCache) << "Test and clean";
					break;
				case 4:
					LC_DEBUG1(LogArmCoprocessorCache) << "Data Sync Barrier";
					break;
				case 5:
					LC_DEBUG1(LogArmCoprocessorCache) << "Data Mem Barrier";
					break;
				default:
					LC_WARNING(LogArmCoprocessor) << "Unknown c10 operation";
					return false;
			}
			break;
		case 11:
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorCache) << "Clean unified $";
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Clean unified $ line MVA " << data;
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Clean unified $ line set/way "
					        << data;
					break;
				default:
					LC_WARNING(LogArmCoprocessor) << "Unknown c11 operation";
					return false;
			}
			break;
		case 13:
			if (opc2 == 1) {
				LC_DEBUG1(LogArmCoprocessorCache)
				        << "Prefetch cache line MVA " << data;
			} else {
				LC_WARNING(LogArmCoprocessor) << "unknown c13 op";
				return false;
			}
			break;
		case 14:
			if(is_read) data = 1 << 30;
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Clean and invalidate d$";
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Clean and invalidate d$ line MVA "
					        << data;
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Clean and invalidate d$ line set/way "
					        << data;
					break;
				case 3:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Test, clean and invalidate";
					break;
				default:
					LC_WARNING(LogArmCoprocessor) << "Unknown c14 op";
					return false;
			}
			break;
		case 15:
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Clean and invalidate u$";
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Clean and invalidate u$ line MVA "
					        << data;
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorCache)
					        << "Clean and invalidate u$ line set/way "
					        << data;
					break;
				case 3:
					LC_WARNING(LogArmCoprocessor) << "Unknown c15 op";
					return false;
			}
			break;
		default:
			LC_WARNING(LogArmCoprocessor) << "Unknown cr7 operation";
			return false;
	}

	return true;
}

bool ArmControlCoprocessor::access_cp8(bool is_read, uint32_t &data)
{
	// TLB Control
	assert(!is_read); //this register is write-only

	switch (rm) {
		case 7:

			//TODO: change this to actually produce correct behaviour
			//		assert(Manager->cpu.in_kernel_mode());

			Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
			Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);

			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorTlb)
					        << " Invalidate unified entire tlb";
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorTlb)
					        << " Invalidate unified entry";
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorTlb)
					        << " Invalidate unified on ASID match";
					break;
				default:
					LC_WARNING(LogArmCoprocessorTlb)
					        << " Unknown unified operation";
					return false;
			}
			break;
		case 5:
			Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorTlb)
					        << " Invalidate instruction tlb";
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorTlb)
					        << " Invalidate instruction tlb entry";
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorTlb)
					        << " Invalidate instruction on ASID match";
					break;
				default:
					LC_WARNING(LogArmCoprocessorTlb)
					        << " Unknown instruction operation";
					return false;
			}
			break;
		case 6:
			Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);
			switch (opc2) {
				case 0:
					LC_DEBUG1(LogArmCoprocessorTlb) << " Invalidate data tlb";
					break;
				case 1:
					LC_DEBUG1(LogArmCoprocessorTlb)
					        << " Invalidate data tlb entry";
					break;
				case 2:
					LC_DEBUG1(LogArmCoprocessorTlb)
					        << " Invalidate data tlb on asid match";
					break;
				default:
					LC_WARNING(LogArmCoprocessorTlb)
					        << " Unknown data operation";
					return false;
			}
			break;
		default:
			LC_WARNING(LogArmCoprocessorTlb) << "Unknown tlb ctrl operation";
			return false;
	}
	return true;
}

bool ArmControlCoprocessor::access_cp9(bool is_read, uint32_t &data)
{
	if (opc1 != 0)
		return false;

	switch (rm) {
		case 12:
			switch (opc2) {
				case 0:	// PMCR
					return pmu.PMCR(is_read, data);
				case 1:	// PMCNTENSET
					return pmu.PMCNTENSET(is_read, data);
				case 2:	// PMCNTENCLR
					return pmu.PMCNTENCLR(is_read, data);
				case 5:	// PMSELR
					return pmu.PMSELR(is_read, data);
				default:
					return false;
			}
			break;
		case 13:
			switch (opc2) {
				case 0:
					return pmu.CCNTVAL(is_read, data);
				case 1:
					return pmu.PMXEVTYPER(is_read, data);
				case 2:
					return pmu.PMNxVAL(is_read, data);
				default:
					return false;
			}
			break;
		default:
			return false;
	}
}

bool ArmControlCoprocessor::access_cp10(bool is_read, uint32_t &data)
{
	fprintf(stderr, "Access_cp10: Read: %d Data: %x", is_read, data);
	return false;
}


bool ArmControlCoprocessor::access_cp15(bool is_read, uint32_t &data)
{
	LC_WARNING(LogArmCoprocessor) << "Attempted unknown cp15 operation";
	return true;
}

MMU *ArmControlCoprocessor::get_mmu()
{
	return (MMU*)Manager->GetDeviceByName("mmu");
}
