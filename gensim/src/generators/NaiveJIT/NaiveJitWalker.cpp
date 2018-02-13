/*
 * InterpreterNodeWalker.cpp
 *
 *  Created on: 24 Mar 2015
 *      Author: harry
 */

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/NaiveJIT/NaiveJitWalker.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAWalkerFactory.h"
#include "genC/ir/IRAction.h"
#include "genC/Parser.h"

#include <typeinfo>
#include <typeindex>

namespace gensim
{
	namespace generator
	{
		namespace naivejit
		{
			using namespace genc::ssa;

			class SSAGenCWalker : public SSANodeWalker
			{
			public:
				SSAGenCWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSANodeWalker(_statement, _factory) {}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return EmitFixedCode(output, end_label, fully_fixed);
				}
				std::string GetDynamicValue() const
				{
					return GetFixedValue();
				}

			};

			class SSAAllocRegisterStatementWalker : public SSAGenCWalker
			{
			public:
				SSAAllocRegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}
				// Don't need to do anything here
				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}
			};

			class SSABinaryArithmeticStatementWalker : public SSAGenCWalker
			{
			public:
				SSABinaryArithmeticStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

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
					const SSABinaryArithmeticStatement &stmt = (const SSABinaryArithmeticStatement &)Statement;
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = (" << Factory.GetOrCreate(stmt.LHS())->GetFixedValue() << genc::BinaryOperator::PrettyPrintOperator(stmt.Type) << Factory.GetOrCreate(stmt.RHS())->GetFixedValue() << ");";
					return true;
				}
			};

			class SSACastStatementWalker : public SSAGenCWalker
			{
			public:
				SSACastStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				std::string GetFixedValue() const
				{
					const SSACastStatement &stmt = (const SSACastStatement &)Statement;

					if(stmt.GetType().Reference) return Factory.GetOrCreate(stmt.Expr())->GetFixedValue();

					return "((" + stmt.GetType().GetCType() + ")" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + ")";
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
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
				SSAConstantStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAConstantStatement &stmt = (const SSAConstantStatement &)Statement;
					return true;
				}

				virtual std::string GetFixedValue() const
				{
					const SSAConstantStatement &stmt = (const SSAConstantStatement &)Statement;
					if(stmt.GetType().IsFloating()) {
						UNIMPLEMENTED;
					}

					std::stringstream str;
					str << "(" << stmt.GetType().GetCType() << ")(" << stmt.Constant.Int() << "ULL)";
					return str.str();
				}
			};

			class SSAFreeRegisterStatementWalker : public SSAGenCWalker
			{
			public:
				SSAFreeRegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				// Don't need to do anything here
				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}
			};

			class SSAIfStatementWalker : public SSAGenCWalker
			{
			public:
				SSAIfStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIfStatement &stmt = (const SSAIfStatement &)(Statement);

					output << "if(" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ") "
					       //						"  puts(\"C I T " << stmt.GetName() << " " << stmt.GetISA() << " " << stmt.GetLineNumber() << "\");"
					       "  goto __block_" << stmt.TrueTarget()->GetName() << ";"
					       "else "
					       //						"  puts(\"C I F " << stmt.GetName() << " " << stmt.GetISA() << " " << stmt.GetLineNumber() << "\");"
					       "  goto __block_" << stmt.FalseTarget()->GetName() << ";";
					return true;
				}
			};

			class SSAIntrinsicStatementWalker : public SSAGenCWalker
			{
			public:
				SSAIntrinsicStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIntrinsicStatement &stmt = (const SSAIntrinsicStatement &)(Statement);

					switch (stmt.Type) {
						case SSAIntrinsicStatement::SSAIntrinsic_Clz32:
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_clz(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_TakeException:
							output << "cpuTakeException((gensim::Processor*)cpu->cpu_ctx, " << (Factory.GetOrCreate(stmt.Args(0))->GetFixedValue()) << "," << (Factory.GetOrCreate(stmt.Args(1))->GetFixedValue()) << ");";

							break;
						case SSAIntrinsicStatement::SSAIntrinsic_HaltCpu:
							output << "halt_cpu();";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_Popcount32:
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_popcount(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_SetCpuMode:
							//XXX todo;
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode:
						case SSAIntrinsicStatement::SSAIntrinsic_ReadPc:
							// nothing here
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_Trap:
							output << "asm(\"int3\");";
							break;

//			output << "set_cpu_mode(" << Factory.GetOrCreate(stmt.Args[0])->GetFixedValue() << ");";
//			break;
						case SSAIntrinsicStatement::SSAIntrinsic_WriteDevice:

						case SSAIntrinsicStatement::SSAIntrinsic_DoubleAbs:
						case SSAIntrinsicStatement::SSAIntrinsic_DoubleIsQnan:
						case SSAIntrinsicStatement::SSAIntrinsic_DoubleIsSnan:
						case SSAIntrinsicStatement::SSAIntrinsic_DoubleSqrt:
						case SSAIntrinsicStatement::SSAIntrinsic_FloatAbs:
						case SSAIntrinsicStatement::SSAIntrinsic_FloatIsQnan:
						case SSAIntrinsicStatement::SSAIntrinsic_FloatIsSnan:
						case SSAIntrinsicStatement::SSAIntrinsic_FloatSqrt:
							if(stmt.HasValue()) {
								output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = 0;";
							}
							output << "asm(\"int3\");";
							break;

						default:
							assert(false && "Unrecognized intrinsic");
					}
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAIntrinsicStatement &stmt = (const SSAIntrinsicStatement &)(Statement);
					std::ostringstream str;
					switch (stmt.Type) {
						case SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode: {
							str << "0";
							return str.str();
						}
						case SSAIntrinsicStatement::SSAIntrinsic_ReadPc: {
							str << "__pc";
							return str.str();
						}
						default:
							return stmt.GetName();
					}
				}
			};

			class SSAJumpStatementWalker : public SSAGenCWalker
			{
			public:
				SSAJumpStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAJumpStatement &stmt = (const SSAJumpStatement &)(Statement);

					output << "goto __block_" << stmt.Target()->GetName() << ";";
					return true;
				}
			};

			class SSAMemoryReadStatementWalker : public SSAGenCWalker
			{
			public:
				SSAMemoryReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAMemoryReadStatement &stmt = (const SSAMemoryReadStatement &)(Statement);
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";"
					       "{"
					       "uint" << (stmt.Width*8) << "_t v;"
					       << stmt.GetName() << " = " << "fn_mem_read_" << (stmt.Width * 8) << (stmt.Signed ? "s" : "") << "((gensim::Processor*)(cpu->cpu_ctx), " << Factory.GetOrCreate(stmt.Addr())->GetFixedValue() << ", &v);"
					       << stmt.Target()->GetName() << " = v;"
					       "}";
					return true;
				}
			};

			class SSAMemoryWriteStatementWalker : public SSAGenCWalker
			{
			public:
				SSAMemoryWriteStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAMemoryWriteStatement &stmt = (const SSAMemoryWriteStatement &)(Statement);
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << "fn_mem_write_" << (stmt.Width * 8) << "((gensim::Processor*)(cpu->cpu_ctx), " << Factory.GetOrCreate(stmt.Addr())->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ");";
					return true;
				}
			};

			class SSAReadStructMemberStatementWalker : public SSAGenCWalker
			{
			public:
				SSAReadStructMemberStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAReadStructMemberStatement &stmt = (const SSAReadStructMemberStatement &)(Statement);
					std::stringstream str;

					if(stmt.MemberName == "IsPredicated")
						str << "inst.GetIsPredicated()";
					else {
						if (Statement.Parent->Parent->GetAction()->Context.ISA.IsFieldOrthogonal(stmt.MemberName))
							str << "inst.GetField_" << Statement.Parent->Parent->GetAction()->Context.ISA.ISAName << "_" << stmt.MemberName << "()";
						else
							str << "__insn_" << stmt.MemberName;
					}
					return str.str();
				}
			};

			class SSARegisterStatementWalker : public SSAGenCWalker
			{
			public:
				SSARegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSARegisterStatement &stmt = (const SSARegisterStatement &)(Statement);

					if (stmt.IsBanked) {
						auto &regbank = stmt.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetBankByIdx(stmt.Bank);

						if(regbank.GetElementCount() != 1) {
							// TODO
							if(stmt.IsRead)
								output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";
							output << "assert(false);\n";
							return true;
						}

						uint32_t regbank_offset = regbank.GetRegFileOffset();
						std::ostringstream reg_offset_str;
						reg_offset_str << "(" << regbank.GetRegisterStride() << "*" << Factory.GetOrCreate(stmt.RegNum())->GetFixedValue() << ")";

						std::ostringstream access_str;
						access_str << "*(" << regbank.GetRegisterIRType().GetCType() << "*)(((uint8_t*)regs)+" << regbank_offset << " + " << reg_offset_str.str() << ")";

						if (stmt.IsRead) {
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << access_str.str() << ";";
						} else {
							output << access_str.str() << " = " << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ";";
						}
					} else {
						auto &regslot = stmt.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetSlotByIdx(stmt.Bank);

						uint32_t regbank_offset = regslot.GetRegFileOffset();

						std::ostringstream access_str;
						access_str << "*(" << regslot.GetIRType().GetCType() << "*)(((uint8_t*)regs)+" << regbank_offset << ")";

						if (stmt.IsRead) {
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << access_str.str() << ";";
						} else {
							output << access_str.str() << " = " << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ";";
						}
					}
					return true;
				}
			};

			class SSAReturnStatementWalker : public SSAGenCWalker
			{
			public:
				SSAReturnStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAReturnStatement &stmt = (const SSAReturnStatement&)Statement;
					if(stmt.Value()) output << "return " << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ";";
					else
						output << "goto " << end_label << ";";
					return true;
				}
			};

			class SSASelectStatementWalker : public SSAGenCWalker
			{
			public:
				SSASelectStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASelectStatement &stmt = (const SSASelectStatement &)(Statement);

					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << Factory.GetOrCreate(stmt.Cond())->GetFixedValue() << " ? (" << Factory.GetOrCreate(stmt.TrueVal())->GetFixedValue() << ") : (" << Factory.GetOrCreate(stmt.FalseVal())->GetFixedValue() << ");";

					return true;
				}
			};

			class SSASwitchStatementWalker : public SSAGenCWalker
			{
			public:
				SSASwitchStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASwitchStatement &stmt = (const SSASwitchStatement &)(Statement);

					int i = 0;
					output << "switch(" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ") {";
					for (SSASwitchStatement::ValueMap::const_iterator ci = stmt.GetValues().begin(); ci != stmt.GetValues().end(); ++ci) {
						output << "case (" << Factory.GetOrCreate(ci->first)->GetFixedValue() << "): "
						       "  goto __block_" << ci->second->GetName() << ";";
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
				SSAVariableReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableReadStatement &stmt = (const SSAVariableReadStatement&)(Statement);
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << stmt.Target()->GetName() << ";";
					return true;
				}
			};

			class SSAVariableWriteStatementWalker : public SSAGenCWalker
			{
			public:
				SSAVariableWriteStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableWriteStatement &stmt = (const SSAVariableWriteStatement &)(Statement);
					output << stmt.Target()->GetName() << " =  " << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
					return true;
				}
			};

			class SSACallStatementWalker : public SSAGenCWalker
			{
			public:
				SSACallStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSACallStatementWalker &stmt = (const SSACallStatementWalker &)(Statement);
					output << "assert(false);";
					return true;
				}
			};

			class SSAVectorExtractElementStatementWalker : public SSAGenCWalker
			{
			public:
				SSAVectorExtractElementStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAStatement &stmt = (const SSAStatement&)(Statement);
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = 0;";
					output << "assert(false);";
					return true;
				}
			};

			class SSAVectorInsertElementStatementWalker : public SSAGenCWalker
			{
			public:
				SSAVectorInsertElementStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAStatement &stmt = (const SSAStatement &)(Statement);
					output << "assert(false);";
					return true;
				}
			};

			class SSADeviceReadStatementWalker : public SSAGenCWalker
			{
			public:
				SSADeviceReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAStatement &stmt = (const SSAStatement &)(Statement);
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = 0;";
					output << "assert(false);";
					return true;
				}
			};

			class SSAUnaryArithmeticStatementWalker : public SSAGenCWalker
			{
			public:
				SSAUnaryArithmeticStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAStatement &stmt = (const SSAStatement &)(Statement);
					if(stmt.HasValue()) {
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = 0;";
					}
					output << "assert(false);";
					return true;
				}
			};

#define STMT_IS(x) std::type_index(typeid(gensim::genc::ssa::x)) == std::type_index(typeid(*stmt))
#define HANDLE(x) \
	if (STMT_IS(x)) return new x##Walker(*stmt, *this)
			genc::ssa::SSANodeWalker *NaiveJitNodeFactory::Create(const genc::ssa::SSAStatement *stmt)
			{
				assert(stmt != NULL);

				HANDLE(SSAUnaryArithmeticStatement);
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
				HANDLE(SSAVectorInsertElementStatement);
				HANDLE(SSAVectorExtractElementStatement);
				HANDLE(SSACallStatement);
				assert(false && "Unrecognized statement type");
				return NULL;
			}
#undef STMT_IS

		}
	}
}




