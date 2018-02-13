/*
 * ArmCoprocessor.cpp
 */
#include "system.h"

#include "abi/devices/MMU.h"
#include "abi/devices/PeripheralManager.h"
#include "abi/devices/arm/core/ArmControlCoprocessorv6.h"

#include "gensim/gensim_processor.h"

#include "translate/TranslationManager.h"

#include "util/LogContext.h"

#include <cassert>
#include <iomanip>

UseLogContext(LogArmCoreDevice);
DeclareChildLogContext(LogArmCoprocessor, LogArmCoreDevice, "ARM-COPROC");
DeclareChildLogContext(LogArmCoprocessorCache, LogArmCoprocessor, "CACHE");
DeclareChildLogContext(LogArmCoprocessorTlb, LogArmCoprocessor, "TLB");
DeclareChildLogContext(LogArmCoprocessorDomain, LogArmCoprocessor, "Domain");

using namespace archsim::abi::devices;

RegisterComponent(archsim::abi::devices::Device, ArmControlCoprocessorv6,
                  "armcoprocessor", "ARMv6 ARM Control Coprocessor unit interface")
;

ArmControlCoprocessorv6::ArmControlCoprocessorv6() : cp1_M(false), cp1_S(false), cp1_R(false), far(0), fsr(0), ttbr0(0), ttbr1(0), ttbcr(0),dacr(0xffffffff), V_descriptor(NULL), cpacr(0), actlr(0)
{
	sctl_word = 0x00c50078;
}

bool ArmControlCoprocessorv6::access_cp0(bool is_read, uint32_t &data)
{
	//	fprintf(stderr, "access c0: rm: %x opc1: %x opc2: %x read?: %d\n", rm, opc1, opc2, is_read);
	//	if (!is_read && rn != 0 && rm != 0 && opc1 != 2 && opc2 != 0) {
	//		LC_ERROR(LogArmCoprocessor) << "Attempted invalid write to cp0";
	//		return false;
	//	}

	if (is_read) {
		switch (rm) {
			case 0:
				switch (opc1) {
					case 0:
						switch (opc2) {
							case 0: // MAIN ID
								//data = 0x41069265;		// ARMv5
								//data = 0x410fc083;		// Cortex A8
								data = 0x410fc080;		// QEMU Cortex A8
								return true;

							case 1: // CACHE TYPE
								//data = 0x0f006006;	// ARMv5
								data = 0x82048004;	// Cortex A8
								return true;

							case 2: // TCM STATUS
								//data = 0x00004004; // ARMv5
								data = 0;	// Cortex A8
								return true;

							case 3:	// TLB TYPE
								data = 0; //0x00202001;	// Cortex A8
								return true;

							case 5:	// MP ID
								//					data = 0;
								data = 0x00000000;
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
								//qemu value
								data = (1 << 27) | (2 << 24) | 3;
								//data = 0x0A200023;
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
								// ID_PFR0
								data = 0x00001031;
								//data = 0x00001131;
								return true;

							case 4:
								// ID_MMFR0
								//data = 0x00100003;
								data = 0x31100003;
								return true;

							case 5:
								// ID_MMFR1
								data = 0x20000000;
								return true;
							case 7:
								// ID_MMFR3
								data = 0x00000011;
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
								// ID_ISAR0
								data = 0x00101111;
								return true;

							case 3:
								// ID_ISAR3
								data = 0x11112131;
								return true;

							case 5:
								// ID_ISAR5
								data = 0x00000000;
								return true;
						}
						break;
				}
				break;
		}
	} else {
		switch (rm) {
			case 0:
				switch (opc1) {
					case 2:
						switch (opc2) {
							case 0:
								CACHE_SIZE_SELECTION = data;
								return true;
						}
						break;
				}
				break;
		}
	}
	assert(false && "Unimplemented ID register");
	return false;
}

bool ArmControlCoprocessorv6::access_cp1(bool is_read, uint32_t &data)
{
//	fprintf(stderr, " *** access c1: rm: %x opc1: %x opc2: %x read?: %d\n", rm, opc1, opc2, is_read);
	switch(rm) {
		case 0: {
			switch(opc2) {
				case 0: {
					//System Control
					if (!is_read) {
						bool AFE = (data & (1 << 29)) != 0;
						bool TRE = (data & (1 << 28)) != 0;
						bool L2 = (data & (1 << 26)) != 0;
						bool EE = (data & (1 << 25)) != 0;
						bool VE = (data & (1 << 24)) != 0;
						bool XP = (data & (1 << 23)) != 0;
						bool U = (data & (1 << 22)) != 0;
						bool FI = (data & (1 << 21)) != 0;
						bool HA = (data & (1 << 17)) != 0;
						bool L4 = (data & (1 << 15)) != 0;
						bool RR = (data & (1 << 14)) != 0;
						bool V = (data & (1 << 13)) != 0;
						bool I = (data & (1 << 12)) != 0;
						bool Z = (data & (1 << 11)) != 0;
						bool SW = (data & (1 << 10)) != 0;
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

						this->AFE = AFE;
						this->TRE = TRE;

						cp1_M = M;
						cp1_S = S;
						cp1_R = R;
						get_mmu()->set_enabled(M);

						if(!V_descriptor) V_descriptor = &Manager->cpu.GetRegisterDescriptor("cpV");
						uint8_t *v_reg =
						    (uint8_t*) V_descriptor->DataStart;
						*v_reg = V;

						LC_DEBUG1(LogArmCoprocessor) << "CP1 Write: "
						                             " L2:" << L2 << " EE:" << EE << " VE:" << VE << " XP:" << XP
						                             << " U:" << U << " FI:" << FI << " L4:" << L4 << " RR:" << RR
						                             << " V:" << V << " I:" << I << " Z:" << Z << " SW:" << SW << " R:"
						                             << R << " S:" << S << " B:" << B << " L:" << L << " D:" << D
						                             << " P:" << P << " W:" << W << " C:" << C << " A:" << A << " M:"
						                             << M << " 0x" << std::hex << std::setw(8) << std::setfill('0')
						                             << data << " at " << this->Manager->cpu.read_pc();

						sctl_word = data;
						sctl_word |= 0b00000000110001010000000001110000;
						sctl_word &= 0b01111011111001110111110011111111;

						//TODO: reflect supported/unsupported features in sctl_word;

						if(AFE || TRE || HA || SW) {
//							fprintf(stderr, "OMG SOME CRAZY FEATURES WERE ENABLED %u %u %u %u\n", AFE, TRE, HA, SW);
//							usleep(10000000);
						}


						// do not support AFE or TEX
//						sctl_word &=~0b00110000000000000000000000000000;

					} else {
						data = sctl_word;
					}

					break;
				}
				case 1: {
					if(is_read) {
						data = actlr;
						LC_DEBUG1(LogArmCoprocessor) << "ACTLR Read: " << std::hex << data;
					} else {
						actlr = data;
						LC_DEBUG1(LogArmCoprocessor) << "ACTLR Write: " << std::hex << data;
					}

					break;
				}
				case 2: {
					if(is_read) {
						data = cpacr;
						LC_DEBUG1(LogArmCoprocessor) << "CPACR Read: " << std::hex << data;
					} else {
						cpacr = data;
						LC_DEBUG1(LogArmCoprocessor) << "CPACR Write: " << std::hex << data;

						// check cpacr
						uint32_t cpacr_fp = (cpacr >> 20);
						if(cpacr_fp == 0) Manager->cpu.GetFeatures().SetFeatureLevel("ARM_FPU_ENABLED_CPACR", 0);
						else if(cpacr_fp == 0x5) Manager->cpu.GetFeatures().SetFeatureLevel("ARM_FPU_ENABLED_CPACR", 1);
						else Manager->cpu.GetFeatures().SetFeatureLevel("ARM_FPU_ENABLED_CPACR", 2);

						if(cpacr >> 31) Manager->cpu.GetFeatures().SetFeatureLevel("ARM_NEON_ENABLED_CPACR", 0);
						else Manager->cpu.GetFeatures().SetFeatureLevel("ARM_NEON_ENABLED_CPACR", 1);

					}
					break;
				}
				default: {
					assert(false);
					return false;
				}
			}
			break;
		}
		default: {
			assert(false);
			return false;
		}
	}

	return true;
}

archsim::util::Counter64 ttbr_switch;
bool ArmControlCoprocessorv6::access_cp2(bool is_read, uint32_t &data)
{
	//Translation table base

	if (!is_read) {

		switch (opc2) {
			case 0:
				ttbr_switch++;
				ttbr0 = data;

				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);

				LC_DEBUG1(LogArmCoprocessorTlb)
				        << "Attempted write to TTBR0 "
				        << std::hex << data;
				break;
			case 1:
				ttbr1 = data;
				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);

				LC_DEBUG1(LogArmCoprocessorTlb)
				        << "Attempted write to TTBR1 "
				        << std::hex <<data;
				break;
			case 2:
				ttbcr = data;
				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);

//			fprintf(stderr, "Wrote to TTBCR! %x\n", ttbcr);
//			usleep(500000);

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
		if (rm == 0 && opc1 == 0) {
			if (opc2 == 0) {
				data = ttbr0;
			} else if (opc2 == 1) {
				data = ttbr1;
			} else if (opc2 == 2) {
				data = ttbcr;
			}
		} else {
			LC_WARNING(LogArmCoprocessorTlb) << "Attempted read from unknown tlb field";
			LC_WARNING(LogArmCoprocessorTlb) << "access c2: rm: " << rm << " opc1: " << opc1 << " opc2 " << opc2 << " read: " << is_read;

		}
	}

//	fprintf(stderr, "access c2: rm: %x opc1: %x opc2: %x read?: %d data: %x\n", rm, opc1, opc2, is_read, data);

	return true;
}
bool ArmControlCoprocessorv6::access_cp3(bool is_read, uint32_t &data)
{
	// Domain access control
	if(is_read) {
		LC_DEBUG1(LogArmCoprocessorDomain) << "Read of domain access control register";
		data = dacr;
	} else {
		LC_DEBUG1(LogArmCoprocessorDomain) << "Write of domain access control register";

		if(dacr != data) {
			Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
			Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);
		}

		dacr = data;
	}

	//	fprintf(stderr, "access c3: rm: %x opc1: %x opc2: %x read?: %d, data: %x, pc: %x \n", rm, opc1, opc2, is_read, data, this->Manager->cpu.read_pc());
	return true;
}

// No cp4

bool ArmControlCoprocessorv6::access_cp5(bool is_read, uint32_t &data)
{
	//	fprintf(stderr, "access c5: rm: %x opc1: %x opc2: %x read?: %d\n", rm, opc1, opc2, is_read);
	//Data/unified access permission control
	//Instruction access permission control

	if (rm == 0) {
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
		} else {
			assert(false);
		}
	} else {
		assert(false);
	}

	return true;
}
bool ArmControlCoprocessorv6::access_cp6(bool is_read, uint32_t &data)
{
	//	fprintf(stderr, "access c6: rm: %x opc1: %x opc2: %x read?: %d\n", rm, opc1, opc2, is_read);
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
	} else {
		assert(false);
	}

	return true;
}

bool ArmControlCoprocessorv6::access_cp7(bool is_read, uint32_t &data)
{
	//	fprintf(stderr, "access c7: rm: %x opc1: %x opc2: %x read?: %d\n", rm, opc1, opc2, is_read);
	// Cache management functions
	assert(opc1 == 0);

	auto &pubsub = Manager->GetEmulationModel()->GetSystem().GetPubSub();
	bool undef = true;

	if (is_read) {
		LC_ERROR(LogArmCoprocessor) << "Unknown CP7 read: " << opc1 << " " << rm << " " << opc2;
		assert(false);
	} else {
		switch(opc1) {
			case 0: {

				switch(rm) {
					case 1: {
						switch(opc2) {
							case 0: // ICIALLUIS
								pubsub.Publish(PubSubType::L1ICacheFlush, nullptr);
								undef = false;
								break;
							case 6: // BPIALLIS
								undef = false;
								break;
						}
						break;
					}
					case 5: {
						switch(opc2) {
							case 0: // ICIALLU
							case 1: // ICIMVAU
								pubsub.Publish(PubSubType::L1ICacheFlush, nullptr);
								undef = false;
								break;
							case 4: // ISB
							case 6: // BPIALL
							case 7: // BPIMVA
								undef = false;
								break;

						}
						break;
					}
					case 6: {
						switch(opc2) {
							case 1: // DCIMVAC
							case 2: // DCISW
								pubsub.Publish(PubSubType::L1DCacheFlush, nullptr);
								undef = false;
								break;
						}
						break;
					}
					case 10: {
						switch(opc2) {
							case 1: // DCCMVAC
							case 2: // DCCSU
								pubsub.Publish(PubSubType::L1DCacheFlush, nullptr);
								undef = false;
								break;
							case 4: // DSB
								undef = false;
								break;
							case 5: // DMB
								undef = false;
								break;
						}
						break;
					}
					case 11: {
						switch(opc2) {
							case 1: // DCCMVAU
								pubsub.Publish(PubSubType::L1DCacheFlush, nullptr);
								undef = false;
								break;
						}
						break;
					}
					case 14: {
						switch(opc2) {
							case 1: // DCCIMVAC
							case 2: // DCCIMSW
								pubsub.Publish(PubSubType::L1DCacheFlush, nullptr);
								undef = false;
								break;
						}
						break;
					}
				}

				break;
			}
		}
	}

	if(undef) {
		LC_ERROR(LogArmCoprocessor) << "Unknown CP7 write: " << opc1 << " " << rm << " " << opc2;
		assert(false);
		return false;
	}

	return true;
}

bool ArmControlCoprocessorv6::access_cp8(bool is_read, uint32_t &data)
{
	//	fprintf(stderr, "access c8: rm: %x opc1: %x opc2: %x read?: %d\n", rm, opc1, opc2, is_read);
	// TLB Control
	assert(!is_read); //this register is write-only

	switch (rm) {
		case 3:
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
			LC_ERROR(LogArmCoprocessorTlb) << "Unknown tlb ctrl operation";
			return false;
	}
	return true;
}

bool ArmControlCoprocessorv6::access_cp9(bool is_read, uint32_t &data)
{
	//	fprintf(stderr, "access c9: rm: %x opc1: %x opc2: %x read?: %d\n", rm, opc1, opc2, is_read);
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

bool ArmControlCoprocessorv6::access_cp10(bool is_read, uint32_t &data)
{
	//	fprintf(stderr, "access c10: rm: %x opc1: %x opc2: %x read?: %d\n", rm, opc1, opc2, is_read);
	if (is_read) {
		switch (rm) {
			case 2:
				switch (opc1) {
					case 0:
						switch (opc2) {
							case 0:
								PRIMARY_REGION_REMAP = data;
								return true;
							case 1:
								NORMAL_REGION_REMAP = data;
								return true;
						}
						break;
				}
				break;
		}
	} else {
		switch (rm) {
			case 2:
				switch (opc1) {
					case 0:
						switch (opc2) {
							case 0:
								data = PRIMARY_REGION_REMAP;
								return true;
							case 1:
								data = NORMAL_REGION_REMAP;
								return true;
						}
						break;
				}
				break;
		}
	}
	//	fprintf(stderr, "Access_cp10: Read: %d Data: %x", is_read, data);
	return false;
}

bool ArmControlCoprocessorv6::access_cp13(bool is_read, uint32_t &data)
{
	assert(opc1 == 0);
	assert(rm == 0);
	if (is_read) {
		switch(opc2) {
			case 0:
				// FCSE ID
				assert(false);
				break;
			case 1:
				data = contextidr;
				break;
			case 2:
				data = tpidrurw;
				break;
			case 3:
				data = tpidruro;
				break;
			case 4:
				data = tpidrprw;
				break;
			default:
				LC_ERROR(LogArmCoprocessor) << "Unknown CP13 read: " << opc1 << " " << rm << " " << opc2;
				assert(false);
		}
	} else {
		switch(opc2) {
			case 0:
				// FCSE ID
				assert(false);
				break;
			case 1:
				contextidr = data;

				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);

//				if(Manager->cpu.HasTraceManager())
//					Manager->cpu.GetTraceManager()->Trace_Reg_Write(true, 0xf0, contextidr);
//				fprintf(stderr, "*** *** *** OMG A CONTEXTIDR WRITE %x at %x (%llu)\n", contextidr, Manager->cpu.read_pc(), Manager->cpu.metrics.interp_inst_count.get_value());

				break;
			case 2:
				tpidrurw = data;
				break;
			case 3:
				tpidruro = data;
				break;
			case 4:
				tpidrprw = data;
				break;
			default:
				LC_ERROR(LogArmCoprocessor) << "Unknown CP13 write: " << opc1 << " " << rm << " " << opc2;
				assert(false);
		}
	}

	return true;
}

bool ArmControlCoprocessorv6::access_cp15(bool is_read, uint32_t &data)
{
	if(is_read) return true;
	if(data == 0) {
		Manager->cpu.StartTracing();
		LC_INFO(LogArmCoprocessor) << "Enabling tracing";
	} else {
		Manager->cpu.StopTracing();
		LC_INFO(LogArmCoprocessor) << "Disabling tracing";
	}
	return true;
}

MMU *ArmControlCoprocessorv6::get_mmu()
{
	return (MMU*)Manager->GetDeviceByName("mmu");
}
