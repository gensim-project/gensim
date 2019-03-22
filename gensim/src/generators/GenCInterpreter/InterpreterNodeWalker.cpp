/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/GenCInterpreter/InterpreterNodeWalker.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAWalkerFactory.h"
#include "genC/ir/IRAction.h"
#include "genC/Parser.h"
#include "genC/ir/IREnums.h"
#include "genC/Intrinsics.h"

#include <typeindex>

#define INSTRUMENT_FLOATS 1

using namespace gensim::genc;

namespace gensim
{

	namespace GenCInterpreter
	{
		using namespace genc::ssa;

		class SSAGenCWalker : public SSANodeWalker
		{
		public:

			SSAGenCWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSANodeWalker(_statement, _factory)
			{
			}

			virtual bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const override
			{
				return EmitFixedCode(output, end_label, fully_fixed);
			}

			virtual std::string GetDynamicValue() const override
			{
				return GetFixedValue();
			}

		};

		class SSAAllocRegisterStatementWalker : public SSAGenCWalker
		{
		public:

			SSAAllocRegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}
			// Don't need to do anything here

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}
		};

		class SSABinaryArithmeticStatementWalker : public SSAGenCWalker
		{
		public:

			SSABinaryArithmeticStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			std::string GetFixedValue() const
			{
				//		std::stringstream output;
				//		const SSABinaryArithmeticStatement &stmt = (const SSABinaryArithmeticStatement &)Statement;
				//
				//		output << "(" << Factory.GetOrCreate(stmt.LHS)->GetFixedValue() << genc::BinaryOperator::PrettyPrintOperator(stmt.Type) << Factory.GetOrCreate(stmt.RHS)->GetFixedValue() << ")";
				//		return output.str();
				return Statement.GetName();
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSABinaryArithmeticStatement &stmt = (const SSABinaryArithmeticStatement &) Statement;

				switch(stmt.Type) {
					case genc::BinaryOperator::RotateRight:
					case genc::BinaryOperator::RotateLeft: {
						std::string typestring = stmt.GetType().GetCType();
						output << typestring << " " << stmt.GetName() << ";";

						int bits;
						switch (stmt.LHS()->GetType().SizeInBytes()) {
							case 1:
								bits = 8;
								break;
							case 2:
								bits = 16;
								break;
							case 4:
								bits = 32;
								break;
							case 8:
								bits = 64;
								break;
							default:
								throw std::logic_error("Unsupported rotate-right data size");
						}

						output << "{" <<
						       typestring << " lhs = " << Factory.GetOrCreate(stmt.LHS())->GetFixedValue() << ";" <<
						       typestring << " rhs = " << Factory.GetOrCreate(stmt.RHS())->GetFixedValue() << ";";

						if(stmt.Type == genc::BinaryOperator::RotateRight) {
							output << stmt.GetName() << "= (lhs >> rhs) | (lhs << (" << bits << "-rhs));";
						} else if(stmt.Type == genc::BinaryOperator::RotateLeft) {
							output << stmt.GetName() << "= (lhs << rhs) | (lhs >> (" << bits << "-rhs));";
						} else {
							throw std::logic_error("Unknown operator");
						}
						output << "}";

						return true;
					}
					default:
						break;
				}

				// currently special case for SAR until new infrastructure for interpreter (and signed operations) developed
				if(stmt.Type == genc::BinaryOperator::SignedShiftRight) {
					auto signed_type = stmt.LHS()->GetType();
					signed_type.Signed = true;
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = ((" << signed_type.GetCType() << ")" << Factory.GetOrCreate(stmt.LHS())->GetFixedValue() << " /* signed */ >> " << Factory.GetOrCreate(stmt.RHS())->GetFixedValue() << ");";
				} else {
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = (" << Factory.GetOrCreate(stmt.LHS())->GetFixedValue() << genc::BinaryOperator::PrettyPrintOperator(stmt.Type) << Factory.GetOrCreate(stmt.RHS())->GetFixedValue() << ");";
				}

				return true;
			}
		};

		class SSACastStatementWalker : public SSAGenCWalker
		{
		public:

			SSACastStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			std::string GetFixedValue() const override
			{
				const SSACastStatement &stmt = (const SSACastStatement &) Statement;

				switch (stmt.GetCastType()) {
					case SSACastStatement::Cast_ZeroExtend:
					case SSACastStatement::Cast_SignExtend:
					case SSACastStatement::Cast_Truncate: {
						if (stmt.GetType().Reference) return Factory.GetOrCreate(stmt.Expr())->GetFixedValue();
						std::string cast_stmt = Factory.GetOrCreate(stmt.Expr())->GetFixedValue();

						// if we are switching signedness, change signedness  before extending
						if(stmt.Expr()->GetType().Signed != stmt.GetType().Signed) {
							SSAType partial_type = stmt.Expr()->GetType();
							partial_type.Signed = stmt.GetType().Signed;
							cast_stmt = "((" + partial_type.GetCType() + ")" + cast_stmt + ")";
						}
						cast_stmt = "((" + stmt.GetType().GetCType() + ")" + cast_stmt + ")";

						return cast_stmt;
					}
					case SSACastStatement::Cast_Reinterpret:
						return Statement.GetName();
//
//						switch (stmt.GetType().BaseType.PlainOldDataType) {
//							case genc::IRPlainOldDataType::DOUBLE:
//								return "(bitcast_u64_double(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + "))";
//							case genc::IRPlainOldDataType::FLOAT:
//								return "(bitcast_u32_float(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + "))";
//							case genc::IRPlainOldDataType::INT64:
//								return "(bitcast_double_u64(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + "))";
//							case genc::IRPlainOldDataType::INT32:
//								return "(bitcast_float_u32(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + "))";
//							default:
//								throw std::logic_error("Unhandled");
//						}

					case SSACastStatement::Cast_Convert: {
						auto option = stmt.GetOption();

						if(option != SSACastStatement::Option_RoundDefault) {
							return stmt.GetName();
						}

						return "(" + stmt.GetType().GetCType() + ")(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + ")";
					}
					case SSACastStatement::Cast_VectorSplat: {
						return stmt.GetType().GetCType() + "(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + ")";
					}
				}
				assert(false && "Unknown cast type");
				UNEXPECTED;
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const  override
			{
				const SSACastStatement &stmt = (const SSACastStatement &)Statement;

				if(stmt.GetCastType() == SSACastStatement::Cast_Reinterpret) {
					output << stmt.GetType().GetCType() << " " << Statement.GetName() << ";";
					output << "{";
					output << "auto value = " << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
					output << Statement.GetName() << " = *(" << Statement.GetType().GetCType() << "*)&value;";
					output << "}";
					return true;
				}

				if(stmt.GetCastType() == SSACastStatement::Cast_Convert) {
					auto option = stmt.GetOption();
					std::stringstream str;

					switch(option) {
						case SSACastStatement::Option_RoundDefault:
							// nothing to do here: this is handled by GetFixedValue
							return true;
						case SSACastStatement::Option_RoundTowardZero:
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";
							output << "{";
							output << "auto mode = thread->GetFPState().GetRoundingMode();";
							output << "thread->GetFPState().SetRoundingMode(archsim::core::thread::RoundingMode::RoundTowardZero);";
							output << stmt.GetName() << " = " << "(" << stmt.GetType().GetCType() << ")" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
							output << "thread->GetFPState().SetRoundingMode(mode);";
							output << "}";
							break;

						default:
							throw std::logic_error("Unhandled");
					}
				}

				return true;
				/*
				const SSACastStatement &stmt = (const SSACastStatement &)Statement;
				output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = (" << stmt.TargetType.GetCType() << ")(" << Factory.GetOrCreate(stmt.Expr)->GetFixedValue() << ");";
				return true;*/
			}
		};

		class SSAConstantStatementWalker : public SSAGenCWalker
		{
		public:

			SSAConstantStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAConstantStatement &stmt = (const SSAConstantStatement &) Statement;
				return true;
			}

			virtual std::string GetFixedValue() const
			{
				const SSAConstantStatement &stmt = (const SSAConstantStatement &) Statement;
				std::stringstream str;
				if (stmt.GetType().IsFloating()) {
					switch (stmt.Constant.Type()) {
						case genc::IRConstant::Type_Float_Single:
							str << "(float)(" << stmt.Constant.Flt() << ")";
							break;
						case genc::IRConstant::Type_Float_Double:
							str << "(double)(" << stmt.Constant.Dbl() << ")";
							break;
						default:
							throw std::logic_error("");
					}

				} else {
					switch(stmt.Constant.Type()) {
						case genc::IRConstant::Type_Integer:
							str << "(" << stmt.GetType().GetCType() << ")(" << stmt.Constant.Int() << "ULL)";
							break;
						case genc::IRConstant::Type_Vector:
							str << "wutils::Vector<" << stmt.GetType().GetElementType().GetCType() << ", " << stmt.Constant.GetVector().Width() << ">({";
							for(unsigned i = 0; i < stmt.Constant.GetVector().Width(); ++i) {
								if(i) {
									str << ", ";
								}
								str << stmt.Constant.GetVector().GetElement(i).Int();
							}
							str << "})";
							break;
						default:
							UNIMPLEMENTED;
					}
				}
				return str.str();
			}
		};

		class SSAFreeRegisterStatementWalker : public SSAGenCWalker
		{
		public:

			SSAFreeRegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			// Don't need to do anything here

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}
		};

		class SSAIfStatementWalker : public SSAGenCWalker
		{
		public:

			SSAIfStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAIfStatement &stmt = (const SSAIfStatement &) (Statement);

				output << "if(" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ") "
				       //						"  puts(\"C I T " << stmt.GetName() << " " << stmt.GetISA() << " " << stmt.GetLineNumber() << "\");"
				       "  goto __block_" << stmt.TrueTarget()->GetName() << ";"
				       "else "
				       //						"  puts(\"C I F " << stmt.GetName() << " " << stmt.GetISA() << " " << stmt.GetLineNumber() << "\");"
				       "  goto __block_" << stmt.FalseTarget()->GetName() << ";";
				return true;
			}
		};

		class SSADeviceReadStatementWalker : public SSAGenCWalker
		{
		public:

			SSADeviceReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label, bool fully_fixed) const
			{
				const SSADeviceReadStatement &stmt = (const SSADeviceReadStatement&) (Statement);

				const SSAStatement *dev_id = stmt.Device();
				const SSAStatement *addr = stmt.Address();
				const SSASymbol *target = stmt.Target();

				int size = target->GetType().SizeInBytes() * 8;

				output << "uint32_t " << stmt.GetName() << " = thread->GetPeripherals().GetDevice(" << Factory.GetOrCreate(dev_id)->GetFixedValue() << ")->Read" << size << "(" << Factory.GetOrCreate(addr)->GetFixedValue() << ", " << target->GetName() << ");";

				return true;
			}
		};

		class SSAIntrinsicStatementWalker : public SSAGenCWalker
		{
		public:

			SSAIntrinsicStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAIntrinsicStatement &stmt = (const SSAIntrinsicStatement &) (Statement);

				switch (stmt.GetID()) {
					case IntrinsicID::BSwap32:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ";
						output << "genc_bswap32(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::BSwap64:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ";
						output << "genc_bswap64(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::WritePC:
						output << "thread->SetPC(archsim::Address(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "));";
						break;

					case IntrinsicID::CLZ32:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_clz(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::CLZ64:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_clzll(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::CTZ32:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_ctz(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::CTZ64:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_ctzll(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::TakeException:
						output << "thread->TakeException(" << (Factory.GetOrCreate(stmt.Args(0))->GetFixedValue()) << "," << (Factory.GetOrCreate(stmt.Args(1))->GetFixedValue()) << ");";
						break;
					case IntrinsicID::HaltCpu:
						output << "UNIMPLEMENTED; // haltcpu\n";
						break;
					case IntrinsicID::PopCount32:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_popcount(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::ReadPC:
						// nothing here
						break;
					case IntrinsicID::Trap:
						output << "throw std::logic_error(\"Trap. " << Statement.GetDiag().Filename() << ":" << std::to_string(Statement.GetDiag().Line()) << "\");";
						break;
					case IntrinsicID::SetCpuMode:
						output << "thread->SetModeID(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::WriteDevice32:
						output << "thread->GetPeripherals().GetDevice(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ")->Write32(" << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << ", " << Factory.GetOrCreate(stmt.Args(2))->GetFixedValue() << ");";
						break;
					case IntrinsicID::WriteDevice64:
						output << "thread->GetPeripherals().GetDevice(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ")->Write64(" << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << ", " << Factory.GetOrCreate(stmt.Args(2))->GetFixedValue() << ");";
						break;

					case IntrinsicID::PopInterrupt:
//						output << "UNIMPLEMENTED; // pop_interrupt\n";
//						output << "pop_interrupt();";
						break;
					case IntrinsicID::PushInterrupt:
//						output << "push_interrupt(1);";
//						output << "UNIMPLEMENTED; // push_interrupt\n";
						break;
					case IntrinsicID::GetCpuMode:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = thread->GetModeID();";
						break;
					case IntrinsicID::SetExecutionRing:
						output << "thread->SetExecutionRing(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::EnterKernelMode:
						output << "thread->SetExecutionRing(1);";
						break;
					case IntrinsicID::EnterUserMode:
						output << "thread->SetExecutionRing(0);";
						break;
					case IntrinsicID::PendInterrupt:
						output << "thread->PendIRQ();";
						break;

					case IntrinsicID::DoubleAbs:
					case IntrinsicID::FloatAbs:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = fabs(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case IntrinsicID::DoubleIsSNaN:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = double_is_snan(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case IntrinsicID::DoubleIsQNaN:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = double_is_qnan(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case IntrinsicID::FloatIsSNaN:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = float_is_snan(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case IntrinsicID::FloatIsQNaN:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = float_is_qnan(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case IntrinsicID::DoubleSqrt:
					case IntrinsicID::FloatSqrt:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = sqrt(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case IntrinsicID::ADC32:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = genc_adc(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(2))->GetFixedValue() << ");";
						break;
					case IntrinsicID::ADC64:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = genc_adc64(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(2))->GetFixedValue() << ");";
						break;
					case IntrinsicID::SBC32:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = genc_sbc(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(2))->GetFixedValue() << ");";
						break;
					case IntrinsicID::SBC64:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = genc_sbc64(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(2))->GetFixedValue() << ");";
						break;

					case IntrinsicID::ADC8_Flags:
					case IntrinsicID::ADC16_Flags:
					case IntrinsicID::ADC32_Flags:
					case IntrinsicID::ADC64_Flags:
					case IntrinsicID::SBC8_Flags:
					case IntrinsicID::SBC16_Flags:
					case IntrinsicID::SBC32_Flags:
					case IntrinsicID::SBC64_Flags: {
						//SZ0A0P1C0000000V
						auto a = Factory.GetOrCreate(stmt.Args(0));
						auto b = Factory.GetOrCreate(stmt.Args(1));
						auto c = Factory.GetOrCreate(stmt.Args(2));

						std::string intrinsic = "";
						std::string operation = "";
						switch(stmt.GetID()) {
							case IntrinsicID::ADC8_Flags:
								intrinsic = "genc_adc8_flags";
								operation = "+";
								break;
							case IntrinsicID::ADC16_Flags:
								intrinsic = "genc_adc16_flags";
								operation = "+";
								break;
							case IntrinsicID::ADC32_Flags:
								intrinsic = "genc_adc_flags";
								operation = "+";
								break;
							case IntrinsicID::ADC64_Flags:
								intrinsic = "genc_adc64_flags";
								operation = "+";
								break;
							case IntrinsicID::SBC32_Flags:
								intrinsic = "genc_sbc_flags";
								operation = "-";
								break;
							case IntrinsicID::SBC8_Flags:
								intrinsic = "genc_sbc8_flags";
								operation = "-";
								break;
							case IntrinsicID::SBC16_Flags:
								intrinsic = "genc_sbc16_flags";
								operation = "-";
								break;
							case IntrinsicID::SBC64_Flags:
								intrinsic = "genc_sbc64_flags";
								operation = "-";
								break;
							default:
								GASSERT(false);
						}

						output << "{";
						output << "uint16_t flags = " << intrinsic << "(" << a->GetFixedValue() << "," << b->GetFixedValue() << "," << c->GetFixedValue() << ");";
						auto C_desc = Statement.Parent->Parent->Arch->GetRegFile().GetTaggedRegSlot("C");
						auto V_desc = Statement.Parent->Parent->Arch->GetRegFile().GetTaggedRegSlot("V");
						auto N_desc = Statement.Parent->Parent->Arch->GetRegFile().GetTaggedRegSlot("N");
						auto Z_desc = Statement.Parent->Parent->Arch->GetRegFile().GetTaggedRegSlot("Z");

						output << "interface.write_register_" << C_desc->GetID() << "<trace>((flags >> 8) & 1);";
						output << "interface.write_register_" << V_desc->GetID() <<"<trace>(flags & 1);";
						output << "interface.write_register_" << Z_desc->GetID() << "<trace>((flags >> 14) & 1);";
						output << "interface.write_register_" << N_desc->GetID() <<"<trace>((flags >> 15) & 1);";
						output << "}";
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << a->GetFixedValue() << operation << "(" << b->GetFixedValue() << " + " << c->GetFixedValue() << ");";
						break;
					}

					case IntrinsicID::UpdateZNFlags32:
						output << "interface.write_register_Z(!(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "));";
						output << "interface.write_register_N(((" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ") & 0x80000000) != 0);";
						break;
					case IntrinsicID::UpdateZNFlags64:
						output << "interface.write_register_Z(!(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "));";
						output << "interface.write_register_N(((" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ") & 0x8000000000000000ull) != 0);";
						break;

					case IntrinsicID::GetFeature:
//						output << "UNIMPLEMENTED; // getfeature\n";
//						output << "uint32_t " << stmt.GetName() << ";";
						output << "uint32_t " << stmt.GetName() << " = thread->GetFeatures().GetFeatureLevel(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::SetFeature:
						output << "thread->GetFeatures().SetFeatureLevel(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ", " << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << ");";
						break;

					case IntrinsicID::FPGetFlush:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = (uint32_t)thread->GetFPState().GetFlushMode();";
						break;
					case IntrinsicID::FPGetRounding:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = (uint32_t)thread->GetFPState().GetRoundingMode();";
						break;
					case IntrinsicID::FPSetFlush:
						output << "thread->GetFPState().SetFlushMode((archsim::core::thread::FlushMode)" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case IntrinsicID::FPSetRounding:
						output << "thread->GetFPState().SetRoundingMode((archsim::core::thread::RoundingMode)" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case IntrinsicID::UMULH:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";
						output << "{";
						output << "unsigned __int128 a = " << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ";";
						output << "unsigned __int128 b = " << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << ";";
						output << "unsigned __int128 c = a * b;";
						output << stmt.GetName() << " = c >> 64;";
						output << "}";
						break;

					case IntrinsicID::FMA32:
					case IntrinsicID::FMA64:
					case IntrinsicID::TriggerIRQ:
					case IntrinsicID::SMULH:
					case IntrinsicID::SMULL:
					case IntrinsicID::UMULL:
						if(stmt.HasValue()) {
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";
						}
						output << "UNIMPLEMENTED;";
						break;

					case IntrinsicID::MemLock: {
						auto interface_id = stmt.Args(0);
						output << "{";
						output << "auto &interface = thread->GetMemoryInterface(" << Factory.GetOrCreate(interface_id)->GetFixedValue() << ");";
						output << "LC_DEBUG1(LogCPU) << \"Thread \" << thread->GetThreadID() << \" locking interface " << Factory.GetOrCreate(interface_id)->GetFixedValue() << "\";";
						output << "interface.GetMonitor()->Lock(thread);";
						output << "}";
						break;
					}
					case IntrinsicID::MemUnlock: {
						auto interface_id = stmt.Args(0);
						output << "{";
						output << "auto &interface = thread->GetMemoryInterface(" << Factory.GetOrCreate(interface_id)->GetFixedValue() << ");";
						output << "LC_DEBUG1(LogCPU) << \"Thread \" << thread->GetThreadID() << \" unlocking interface " << Factory.GetOrCreate(interface_id)->GetFixedValue() << "\";";
						output << "interface.GetMonitor()->Unlock(thread);";
						output << "}";
						break;
					}

					case IntrinsicID::MemMonitorAcquire: {
						auto interface_id = stmt.Args(0);
						auto address_arg = stmt.Args(1);
						output << "{";
						output << "auto &interface = thread->GetMemoryInterface(" << Factory.GetOrCreate(interface_id)->GetFixedValue() << ");";
						output << "auto addr = archsim::Address(" << Factory.GetOrCreate(address_arg)->GetFixedValue() << ");";
						output << "interface.GetMonitor()->AcquireMonitor(thread, addr);";
						output << "}";
						break;
					}

					case IntrinsicID::MemMonitorWrite8:
					case IntrinsicID::MemMonitorWrite16:
					case IntrinsicID::MemMonitorWrite32:
					case IntrinsicID::MemMonitorWrite64: {
						auto interface_id = stmt.Args(0);
						auto address_arg = stmt.Args(1);
						auto value_arg = stmt.Args(2);
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = 0;";
						output << "{";
						output << "auto &interface = thread->GetMemoryInterface(" << Factory.GetOrCreate(interface_id)->GetFixedValue() << ");";
						output << "auto addr = archsim::Address(" << Factory.GetOrCreate(address_arg)->GetFixedValue() << ");";
						output << "auto value = " << Factory.GetOrCreate(value_arg)->GetFixedValue() << ";";
						output << "if(interface.GetMonitor()->LockMonitor(thread, addr)) {";
						output << stmt.GetName() << " = 1;";
						output << "archsim::MemoryResult mem_result = ";
						switch(stmt.GetID()) {
							case IntrinsicID::MemMonitorWrite8:
								output << "interface.Write8(addr, value);";
								break;
							case IntrinsicID::MemMonitorWrite16:
								output << "interface.Write16(addr, value);";
								break;
							case IntrinsicID::MemMonitorWrite32:
								output << "interface.Write32(addr, value);";
								break;
							case IntrinsicID::MemMonitorWrite64:
								output << "interface.Write64(addr, value);";
								break;
							default:
								UNEXPECTED;
						}

						output << "interface.GetMonitor()->Notify(thread, addr);";
						output << "interface.GetMonitor()->UnlockMonitor(thread, addr);";

						output << "if(mem_result != archsim::MemoryResult::OK) {";
						output << "  thread->TakeMemoryException(interface, addr);";
						output << "}";

						output << "}";
						output << "}";
						break;
					}

					// Unimplemented
					case IntrinsicID::ProbeDevice:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = 0;";
						output << "UNIMPLEMENTED;";
						break;

					/* Previously external functions */
					case IntrinsicID::FlushITLBEntry:
					case IntrinsicID::FlushDTLBEntry:
						if(stmt.HasValue()) {
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ";
						}
						output << "thread->fn_" << stmt.GetSignature().GetName() << "(";
						for(int i = 0; i < stmt.ArgCount(); ++i) {
							if(i != 0) {
								output << ", ";
							}
							output << "(" << Factory.GetOrCreate(stmt.Args(i))->GetFixedValue() << ")";
						}
						output << ");";
						break;

					default:
						throw std::logic_error("Unrecognised intrinsic: " + stmt.GetSignature().GetName());
				}
				return true;
			}

			std::string GetFixedValue() const
			{
				const SSAIntrinsicStatement &stmt = (const SSAIntrinsicStatement &) (Statement);
				switch (stmt.GetID()) {
					case IntrinsicID::ReadPC:
						return "interface.read_pc().Get()";
					default:
						return stmt.GetName();
				}
			}
		};

		class SSAJumpStatementWalker : public SSAGenCWalker
		{
		public:

			SSAJumpStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAJumpStatement &stmt = (const SSAJumpStatement &) (Statement);

				output << "goto __block_" << stmt.Target()->GetName() << ";";
				return true;
			}
		};

		class SSAMemoryReadStatementWalker : public SSAGenCWalker
		{
		public:

			SSAMemoryReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAMemoryReadStatement &stmt = (const SSAMemoryReadStatement &) (Statement);

				// look up correct memory interface
				auto interface = stmt.GetInterface();

				output << "{";
				output << "archsim::Address addr = archsim::Address(" << Factory.GetOrCreate(stmt.Addr())->GetFixedValue() << ");";
				output << "auto &interface = thread->GetMemoryInterface(" << interface->GetID() << ");";
				output << "archsim::MemoryResult _value = interface.Read" << (stmt.Width * 8) << "(addr," << stmt.Target()->GetName() << ");";
				output << "if(_value != archsim::MemoryResult::OK) {";
				// trigger exception
				output << "  thread->TakeMemoryException(interface, addr);";
				output << "}";
				output << "if(trace) { thread->GetTraceSource()->Trace_Mem_Read(1, addr.Get(), " << stmt.Target()->GetName() << ", " << (uint32_t)(stmt.Width) << "); }";
				output << "}";

				return true;
			}
		};

		class SSAMemoryWriteStatementWalker : public SSAGenCWalker
		{
		public:

			SSAMemoryWriteStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAMemoryWriteStatement &stmt = (const SSAMemoryWriteStatement &) (Statement);

				// look up correct memory interface
				auto interface = stmt.GetInterface();

				output << "{";
				output << "auto &mem_interface = thread->GetMemoryInterface(" << interface->GetID() << ");";
				output << "archsim::Address addr = archsim::Address(" << Factory.GetOrCreate(stmt.Addr())->GetFixedValue() << ");";
				output << "archsim::MemoryResult _value = mem_interface.Write" << (stmt.Width * 8) << "(addr," << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ");";
				output << "if(_value != archsim::MemoryResult::OK) {";
				output << "  thread->TakeMemoryException(mem_interface, addr);";
				output << "}";
				output << "if(trace) { thread->GetTraceSource()->Trace_Mem_Write(1, " << Factory.GetOrCreate(stmt.Addr())->GetFixedValue() << ", " << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ", " << (uint32_t)stmt.Width << "); }";
				output << "}";
				return true;
			}
		};

		class SSAReadStructMemberStatementWalker : public SSAGenCWalker
		{
		public:

			SSAReadStructMemberStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}

			std::string GetFixedValue() const
			{
				const SSAReadStructMemberStatement &stmt = (const SSAReadStructMemberStatement &) (Statement);
				std::stringstream str;

				if (stmt.MemberNames.at(0) == "IsPredicated") {
					str << stmt.Target()->GetName() << ".GetIsPredicated()";
				} else if(stmt.MemberNames.at(0) == "PredicateInfo") {
					str << stmt.Target()->GetName() << ".GetPredicateInfo()";
				} else {
					str << stmt.Target()->GetName() << stmt.FormatMembers();
				}
				return str.str();
			}
		};

		class SSARegisterStatementWalker : public SSAGenCWalker
		{
		public:

			SSARegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSARegisterStatement &stmt = (const SSARegisterStatement &) (Statement);

				auto &regfile = stmt.GetContext().GetArchDescription().GetRegFile();

				if (stmt.IsBanked) {
					auto &reg_desc = regfile.GetBankByIdx(stmt.Bank);
					auto idx_node = Factory.GetOrCreate(stmt.RegNum());

					if (stmt.IsRead) {
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = interface.read_register_bank_" << reg_desc.ID << "<trace>(" << idx_node->GetFixedValue() << ");";
					} else {
						auto value_node = Factory.GetOrCreate(stmt.Value());
						output << "interface.write_register_bank_" << reg_desc.ID << "<trace>(" << idx_node->GetFixedValue() << ", " << value_node->GetFixedValue() << ");";
					}
				} else {
					auto &reg_desc = regfile.GetSlotByIdx(stmt.Bank);

					if (stmt.IsRead) {
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = interface.read_register_" << reg_desc.GetID() << "<trace>();";
					} else {
						auto value_node = Factory.GetOrCreate(stmt.Value());
						output << "interface.write_register_" << reg_desc.GetID() << "<trace>(" << value_node->GetFixedValue() << ");";
					}
				}

				return true;
			}
		};

		class SSAReturnStatementWalker : public SSAGenCWalker
		{
		public:

			SSAReturnStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAReturnStatement &stmt = (const SSAReturnStatement&) this->Statement;
				if (stmt.Value()) {
					// write value to return register
					output << "_rval_ = " << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ";";
				}
				output << "goto " << end_label << ";";
				return true;
			}
		};

		class SSASelectStatementWalker : public SSAGenCWalker
		{
		public:

			SSASelectStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSASelectStatement &stmt = (const SSASelectStatement &) (Statement);
				if(stmt.GetType().VectorWidth > 1) {
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << Factory.GetOrCreate(stmt.Cond())->GetFixedValue() << ".Select(" << Factory.GetOrCreate(stmt.TrueVal())->GetFixedValue() << ", " << Factory.GetOrCreate(stmt.FalseVal())->GetFixedValue() << ");";
				} else {
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << Factory.GetOrCreate(stmt.Cond())->GetFixedValue() << " ? (" << Factory.GetOrCreate(stmt.TrueVal())->GetFixedValue() << ") : (" << Factory.GetOrCreate(stmt.FalseVal())->GetFixedValue() << ");";
				}

				return true;
			}
		};

		class SSASwitchStatementWalker : public SSAGenCWalker
		{
		public:

			SSASwitchStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSASwitchStatement &stmt = (const SSASwitchStatement &) (Statement);

				int i = 0;
				output << "switch(" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ") {";
				for (auto v : stmt.GetValues()) {
					output << "case (" << Factory.GetOrCreate(v.first)->GetFixedValue() << "): "
					       "  goto __block_" << v.second->GetName() << ";";
				}
				output << " default: "
				       "  goto __block_" << stmt.Default()->GetName() << "; "
				       "}";
				return true;
			}
		};

		class SSAVariableReadStatementWalker : public SSAGenCWalker
		{
		public:

			SSAVariableReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}

			std::string GetFixedValue() const
			{
				const SSAVariableReadStatement &stmt = (const SSAVariableReadStatement &) (Statement);
				return stmt.Target()->GetName();
			}
		};

		class SSAVariableWriteStatementWalker : public SSAGenCWalker
		{
		public:

			SSAVariableWriteStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAVariableWriteStatement &stmt = (const SSAVariableWriteStatement &) (Statement);
				output << stmt.Target()->GetName() << " =  " << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
				return true;
			}
		};

		class SSACallStatementWalker : public SSAGenCWalker
		{
		public:

			SSACallStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSACallStatement &stmt = (const SSACallStatement&) (Statement);

				if (stmt.HasValue()) output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ";
				bool is_helper = stmt.Target()->GetPrototype().GetIRSignature().HasAttribute(gensim::genc::ActionAttribute::Helper);

				if(is_helper) {
					output << "helper_" << stmt.GetISA() << "_" ;
				} else {
					UNEXPECTED;
				}

				output << stmt.Target()->GetPrototype().GetIRSignature().GetName();

				if(is_helper) {
					output << "<trace>";
				}

				output << "(";

				bool first = true;

				if(is_helper) {
					first = false;
					output << "thread";
				}
				for (int i = 0; i < stmt.ArgCount(); ++i) {
					if (!first) output << ",";
					first = false;

					const SSAStatement *arg_stmt = dynamic_cast<const SSAStatement*> (stmt.Arg(i));
					const SSASymbol *sym = dynamic_cast<const SSASymbol*> (stmt.Arg(i));
					if (arg_stmt != nullptr) {
						output << Factory.GetOrCreate(arg_stmt)->GetFixedValue();
					} else if (sym != nullptr) {
						output << sym->GetName();
					} else {
						throw std::logic_error("Unknown argument type");
					}
				}

				output << ");";
				return true;
			}
		};

		class SSAUnaryArithmeticStatementWalker : public SSAGenCWalker
		{
		public:

			SSAUnaryArithmeticStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAUnaryArithmeticStatement &stmt = (const SSAUnaryArithmeticStatement&) (Statement);
				switch (stmt.Type) {
						using namespace gensim::genc::SSAUnaryOperator;
					case OP_NEGATE:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = !" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
						break;
					case OP_COMPLEMENT:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ~" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
						break;
					case OP_NEGATIVE:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = -" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
						break;
				}
				return true;
			}
		};

		class SSAVectorExtractElementStatementWalker : public SSAGenCWalker
		{
		public:

			SSAVectorExtractElementStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}

			std::string GetFixedValue() const
			{
				const SSAVectorExtractElementStatement &stmt = (const SSAVectorExtractElementStatement&) (Statement);

				return Factory.GetOrCreate(stmt.Base())->GetFixedValue() + "[" + Factory.GetOrCreate(stmt.Index())->GetFixedValue() + "]";
			}
		};

		class SSAVectorInsertElementStatementWalker : public SSAGenCWalker
		{
		public:

			SSAVectorInsertElementStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAVectorInsertElementStatement &stmt = (const SSAVectorInsertElementStatement&) (Statement);

				output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << Factory.GetOrCreate(stmt.Base())->GetFixedValue() << ";";
				output << stmt.GetName() + "[" + Factory.GetOrCreate(stmt.Index())->GetFixedValue() + "] = " << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ";";
				return true;
			}

		};

		class SSAVectorShuffleStatementWalker : public SSAGenCWalker
		{
		public:
			SSAVectorShuffleStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
			{
				const SSAVectorShuffleStatement &stmt = (const SSAVectorShuffleStatement&)(Statement);

				output << stmt.GetType().GetCType() << " " << stmt.GetName () << " = wutils::VectorShuffle(" <<
				       Factory.GetOrCreate(stmt.LHS())->GetFixedValue() << ", " <<
				       Factory.GetOrCreate(stmt.RHS())->GetFixedValue() << ", " <<
				       Factory.GetOrCreate(stmt.Indices())->GetFixedValue() <<
				       ");";
				return true;
			}

		};

	}

#define STMT_IS(x) std::type_index(typeid(genc::ssa::x)) == std::type_index(typeid(*stmt))
#define HANDLE(x) \
	if (STMT_IS(x)) return new GenCInterpreter::x##Walker(*stmt, *this)

	genc::ssa::SSANodeWalker *gensim::GenCInterpreterNodeFactory::Create(const genc::ssa::SSAStatement *stmt)
	{
		assert(stmt != NULL);

		HANDLE(SSABinaryArithmeticStatement);
		HANDLE(SSACastStatement);
		HANDLE(SSAConstantStatement);
		HANDLE(SSADeviceReadStatement);
		HANDLE(SSAIfStatement);
		HANDLE(SSAIntrinsicStatement);
		HANDLE(SSAJumpStatement);
		HANDLE(SSAMemoryReadStatement);
		HANDLE(SSAMemoryWriteStatement);
		HANDLE(SSAReadStructMemberStatement);
		HANDLE(SSARegisterStatement);
		HANDLE(SSAReturnStatement);
		HANDLE(SSASelectStatement);
		HANDLE(SSASwitchStatement);
		HANDLE(SSAVariableReadStatement);
		HANDLE(SSAVariableWriteStatement);
		HANDLE(SSAUnaryArithmeticStatement);
		HANDLE(SSAVectorExtractElementStatement);
		HANDLE(SSAVectorInsertElementStatement);
		HANDLE(SSAVectorShuffleStatement);
		HANDLE(SSACallStatement);
		assert(false && "Unrecognized statement type");
		return NULL;
	}
#undef STMT_IS
}

