#include "define.h"
#include "genC/Parser.h"
#include "genC/ir/IR.h"
#include "genC/ir/IRAction.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSASymbol.h"
#include "isa/ISADescription.h"

#include "generators/GenCJIT/DynamicTranslationGenerator.h"

#include <typeindex>

#define INSTRUMENT_FLOATS 1

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{

			std::string GetLLVMValue(const IRType &type, std::string val)
			{
				std::string llvm_type = type.GetLLVMType();
				std::ostringstream constant_str;

				switch (type.BaseType.PlainOldDataType) {
					case IRPlainOldDataType::INT1:
					case IRPlainOldDataType::INT8:
					case IRPlainOldDataType::INT16:
					case IRPlainOldDataType::INT32:
					case IRPlainOldDataType::INT64:
						constant_str << "llvm::ConstantInt::get(" << llvm_type << ", " << val << ", " << type.Signed << ")";
						break;
					case IRPlainOldDataType::FLOAT:
					case IRPlainOldDataType::DOUBLE:
						constant_str << "llvm::ConstantFP::get(" << llvm_type << ", " << val << ")";
						break;
					case IRPlainOldDataType::VOID:
						assert(false && "Trying to get value for void");
						break;
					default:
						assert(false && "Unrecognized type");
						break;
				}


				return constant_str.str();
			}

			void CreateBlock(const SSABlock &block, util::cppformatstream &output)
			{
				output << "QueueDynamicBlock(ctx.block.region, dynamic_blocks, dynamic_block_queue, __block_" << block.GetName() << ");";
			}

			void JumpOrInlineBlock(SSAWalkerFactory &factory, const SSABlock &Target, util::cppformatstream &output, std::string end_label, bool fully_fixed, bool dynamic)
			{
				// if the target is never_const, we need to put it on the dynamic block queue list and emit an llvm branch
				if (Target.IsFixed() != BLOCK_ALWAYS_CONST) {
					CreateBlock(Target, output);
					output << "__irBuilder.CreateBr(dynamic_blocks[__block_" << Target.GetName() << "]);";
					output << "goto " << end_label << ";";
					return;
				}
				if (dynamic) {
					output << "dynamic_block_queue.push_back(__block_" << Target.GetName() << ");";
					output << "break;";
					return;
				}
				if (Target.GetPredecessors().size() == 1) {
					for (auto statement_iterator = Target.GetStatements().begin(); statement_iterator != Target.GetStatements().end(); statement_iterator++) {
						SSAStatement &stmt = **statement_iterator;
						output << "// STMT: LINE " << stmt.GetDiag().Line() << "\n";
						output << "// " << stmt.ToString() << "\n";
						if (stmt.IsFixed())
							factory.GetOrCreate(&stmt)->EmitFixedCode(output, end_label, fully_fixed);
						else
							factory.GetOrCreate(&stmt)->EmitDynamicCode(output, end_label, fully_fixed);
					}
				} else {
					output << "goto block_" << Target.GetName() << ";";
				}
			}

			class SSABinaryArithmeticStatementWalker : public SSANodeWalker
			{
			public:
				SSABinaryArithmeticStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : SSANodeWalker(_statement, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSABinaryArithmeticStatement &Statement = static_cast<const SSABinaryArithmeticStatement &>(this->Statement);
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = ((" << Statement.GetType().GetCType() << ")";

					SSANodeWalker *LHSNode = Factory.GetOrCreate(Statement.LHS());
					SSANodeWalker *RHSNode = Factory.GetOrCreate(Statement.RHS());

					// Modulo needs to handled differently for floating point operands
					if (Statement.Type == BinaryOperator::Modulo && (Statement.LHS()->GetType().IsFloating() || Statement.RHS()->GetType().IsFloating())) {
						output << "fmod(" << LHSNode->GetFixedValue() << ", " << RHSNode->GetFixedValue() << "));";
						return true;
					}

					output << "(" << LHSNode->GetFixedValue();

					switch (Statement.Type) {
						case BinaryOperator::ShiftLeft:
							output << " << ";
							break;
						case BinaryOperator::ShiftRight:
							output << " >> ";
							break;

						case BinaryOperator::Equality:
							output << " == ";
							break;
						case BinaryOperator::Inequality:
							output << " != ";
							break;

						case BinaryOperator::LessThan:
							output << " < ";
							break;
						case BinaryOperator::LessThanEqual:
							output << " <= ";
							break;
						case BinaryOperator::GreaterThanEqual:
							output << " >= ";
							break;
						case BinaryOperator::GreaterThan:
							output << " > ";
							break;
						case BinaryOperator::Add:
							output << " + ";
							break;
						case BinaryOperator::Subtract:
							output << " - ";
							break;
						case BinaryOperator::Multiply:
							output << " * ";
							break;
						case BinaryOperator::Divide:
							output << " / ";
							break;
						case BinaryOperator::Modulo: {
							output << " % ";
							break;
						}

						case BinaryOperator::Logical_And:
							output << " && ";
							break;
						case BinaryOperator::Logical_Or:
							output << " || ";
							break;

						case BinaryOperator::Bitwise_XOR:
							output << " ^ ";
							break;
						case BinaryOperator::Bitwise_And:
							output << " & ";
							break;
						case BinaryOperator::Bitwise_Or:
							output << " | ";
							break;
						default:
							assert(false && "Unrecognized binary operator");
					}
					output << RHSNode->GetFixedValue() << "));";
					return true;
				}

				bool EmitDynamicBinaryOp(util::cppformatstream &output) const
				{
					const SSABinaryArithmeticStatement &Statement = static_cast<const SSABinaryArithmeticStatement &>(this->Statement);

					const SSANodeWalker &LHSNode = *Factory.GetOrCreate(Statement.LHS());
					const SSANodeWalker &RHSNode = *Factory.GetOrCreate(Statement.RHS());

					output << "llvm::Value *" << Statement.GetName() << ";";

					switch(Statement.Type) {
						case BinaryOperator::Add:
						case BinaryOperator::Bitwise_Or:
						case BinaryOperator::Bitwise_XOR:
						case BinaryOperator::ShiftLeft:
						case BinaryOperator::ShiftRight:
							if(Statement.LHS()->IsFixed()) {
								output << "if(" << LHSNode.GetFixedValue() << " == 0) " << Statement.GetName() << " = " << RHSNode.GetDynamicValue() << "; else ";
							}
							if(Statement.RHS()->IsFixed()) {
								output << "if(" << RHSNode.GetFixedValue() << " == 0) " << Statement.GetName() << " = " << LHSNode.GetDynamicValue() << "; else ";
							}
							break;
						default:
							break;
					}

					output << Statement.GetName() << " = __irBuilder.CreateBinOp(";
					if (LHSNode.Statement.GetType().IsFloating() || RHSNode.Statement.GetType().IsFloating()) {
						switch (Statement.Type) {
							case BinaryOperator::Add:
								output << "llvm::Instruction::FAdd";
								break;
							case BinaryOperator::Subtract:
								output << "llvm::Instruction::FSub";
								break;
							case BinaryOperator::Multiply:
								output << "llvm::Instruction::FMul";
								break;
							case BinaryOperator::Divide:
								output << "llvm::Instruction::FDiv";
								break;
							case BinaryOperator::Modulo:
								output << "llvm::Instruction::FRem";
								break;
							default:
								assert(false && "Undefined operation");
						}
					} else {

						switch (Statement.Type) {
							case BinaryOperator::Add:
								output << "llvm::Instruction::Add";
								break;
							case BinaryOperator::Subtract:
								output << "llvm::Instruction::Sub";
								break;
							case BinaryOperator::Multiply:
								output << "llvm::Instruction::Mul";
								break;
							case BinaryOperator::Divide:
								output << "llvm::Instruction::UDiv";
								break;
							case BinaryOperator::Modulo:
								output << "llvm::Instruction::URem";
								break;
							case BinaryOperator::ShiftLeft:
								output << "llvm::Instruction::Shl";
								break;
							case BinaryOperator::ShiftRight:
								if (LHSNode.Statement.GetType().Signed)
									output << "llvm::Instruction::AShr";
								else
									output << "llvm::Instruction::LShr";
								break;
							case BinaryOperator::Bitwise_And:
								output << "llvm::Instruction::And";
								break;
							case BinaryOperator::Bitwise_Or:
								output << "llvm::Instruction::Or";
								break;
							case BinaryOperator::Bitwise_XOR:
								output << "llvm::Instruction::Xor";
								break;
							default:
								assert(false && "Undefined operation");
								break;
						}
					}
					output << ", " << LHSNode.GetDynamicValue() << ", " << RHSNode.GetDynamicValue() << ");";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSABinaryArithmeticStatement &Statement = static_cast<const SSABinaryArithmeticStatement &>(this->Statement);

					const SSANodeWalker &LHSNode = *Factory.GetOrCreate(Statement.LHS());
					const SSANodeWalker &RHSNode = *Factory.GetOrCreate(Statement.RHS());

					if (Statement.Type < BinaryOperator::Equality) {

						EmitDynamicBinaryOp(output);

					} else {
						std::stringstream uncast;

						if (LHSNode.Statement.GetType().IsFloating() || RHSNode.Statement.GetType().IsFloating()) {
							switch (Statement.Type) {
								case BinaryOperator::Equality:
									uncast << "__irBuilder.CreateFCmpUEQ";
									break;
								case BinaryOperator::GreaterThan:
									uncast << "__irBuilder.CreateFCmpUGT";
									break;
								case BinaryOperator::GreaterThanEqual:
									uncast << "__irBuilder.CreateFCmpUGE";
									break;
								case BinaryOperator::LessThan:
									uncast << "__irBuilder.CreateFCmpULT";
									break;
								case BinaryOperator::LessThanEqual:
									uncast << "__irBuilder.CreateFCmpULE";
									break;
								case BinaryOperator::Inequality:
									uncast << "__irBuilder.CreateFCmpUNE";
									break;
								default:
									assert(false && "Undefined operation");
							}
						} else {
							if (LHSNode.Statement.GetType().Signed) {
								switch (Statement.Type) {
									case BinaryOperator::Equality:
										uncast << "__irBuilder.CreateICmpEQ";
										break;
									case BinaryOperator::GreaterThan:
										uncast << "__irBuilder.CreateICmpSGT";
										break;
									case BinaryOperator::GreaterThanEqual:
										uncast << "__irBuilder.CreateICmpSGE";
										break;
									case BinaryOperator::LessThan:
										uncast << "__irBuilder.CreateICmpSLT";
										break;
									case BinaryOperator::LessThanEqual:
										uncast << "__irBuilder.CreateICmpSLE";
										break;
									case BinaryOperator::Inequality:
										uncast << "__irBuilder.CreateICmpNE";
										break;
									default:
										assert(false && "Undefined operation");
								}
							} else {
								switch (Statement.Type) {
									case BinaryOperator::Equality:
										uncast << "__irBuilder.CreateICmpEQ";
										break;
									case BinaryOperator::GreaterThan:
										uncast << "__irBuilder.CreateICmpUGT";
										break;
									case BinaryOperator::GreaterThanEqual:
										uncast << "__irBuilder.CreateICmpUGE";
										break;
									case BinaryOperator::LessThan:
										uncast << "__irBuilder.CreateICmpULT";
										break;
									case BinaryOperator::LessThanEqual:
										uncast << "__irBuilder.CreateICmpULE";
										break;
									case BinaryOperator::Inequality:
										uncast << "__irBuilder.CreateICmpNE";
										break;
									default:
										assert(false && "Undefined operation");
								}
							}
						}

						uncast << "(" << LHSNode.GetDynamicValue() << "," << RHSNode.GetDynamicValue() << ")";
						output << "llvm::Value * " << Statement.GetName() << " = __irBuilder.CreateIntCast(" << uncast.str() << ", txln_ctx.types.i8, false);";
					}

					if(INSTRUMENT_FLOATS && Statement.GetType().IsFloating() && Statement.GetType().VectorWidth == 1)
						output << "ctx.block.region.EmitPublishEvent(PubSubType::FloatResult, { ctx.block.region.CreateMaterialise(ctx.tiu.GetOffset()), __irBuilder.CreateFPCast(" << Statement.GetName() << ", ctx.block.region.txln.types.f32) });";

					return true;
				}

				std::string GetDynamicValue() const
				{
					if (Statement.IsFixed())
						return GetLLVMValue(Statement.GetType(), Statement.GetName());
					else
						return Statement.GetName();
				}
			};


			class SSAUnaryArithmeticStatementWalker : public SSANodeWalker
			{
			public:
				SSAUnaryArithmeticStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : SSANodeWalker(_statement, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAUnaryArithmeticStatement &Statement = static_cast<const SSAUnaryArithmeticStatement &>(this->Statement);
					switch(Statement.Type) {
							using namespace gensim::genc::SSAUnaryOperator;
						case OP_NEGATIVE:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = -" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ";";
							break;
						default:
							assert(false);
					}

					return false;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAUnaryArithmeticStatement &Statement = static_cast<const SSAUnaryArithmeticStatement &>(this->Statement);
					switch(Statement.Type) {
							using namespace gensim::genc::SSAUnaryOperator;
						case OP_NEGATIVE:
							// if the inner statement is floating point, then emit fsub -0.0, %stmt
							if((Statement.Expr()->GetType().BaseType.PlainOldDataType == IRPlainOldDataType::FLOAT) || (Statement.Expr()->GetType().BaseType.PlainOldDataType == IRPlainOldDataType::DOUBLE)) {
								output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateFNeg(" << Factory.GetOrCreate(Statement.Expr())->GetDynamicValue() << ");";
							} else {
								output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateNeg(" << Factory.GetOrCreate(Statement.Expr())->GetDynamicValue() << ");";
							}
							break;
						default:
							assert(false);
					}

					return false;
				}
			};

			class SSACastStatementWalker : public SSANodeWalker
			{
			public:
				SSACastStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : SSANodeWalker(_statement, *_fact) {}
				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement &>(this->Statement);

					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = ";
					if(Statement.GetCastType() == SSACastStatement::Cast_Reinterpret) {
						IRType start = Statement.Expr()->GetType();
						const IRType &end = Statement.GetType();

						// there are only 4 valid casts here
						if(start == IRTypes::UInt32 && end == IRTypes::Float) {
							output << "bitcast_u32_float";
						} else if(start == IRTypes::UInt64 && end == IRTypes::Double) {
							output << "bitcast_u64_double";
						} else if(start == IRTypes::Float && end == IRTypes::UInt32) {
							output << "bitcast_float_u32";
						} else if(start == IRTypes::Double && end == IRTypes::UInt64) {
							output << "bitcast_double_u64";
						} else {
							assert(false);
							return false;
						}

						output << "(" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ");";
					} else {
						output << "(" << Statement.GetType().GetCType() << ")" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ";";
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement &>(this->Statement);

					output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCast(";

					// figure out what kind of cast we should do
					switch (Statement.GetCastType()) {
						case SSACastStatement::Cast_Reinterpret:
							output << "llvm::Instruction::BitCast";
							break;
						case SSACastStatement::Cast_SignExtend: {
							// if we are casting from int to float
							if (!Statement.Expr()->GetType().IsFloating() && Statement.GetType().IsFloating()) output << "llvm::Instruction::SIToFP";
							// if we are casting from float to double
							else if (Statement.Expr()->GetType().IsFloating() && Statement.GetType().IsFloating())
								output << "llvm::Instruction::FPExt";
							else
								output << "llvm::Instruction::SExt";
							break;
						}
						case SSACastStatement::Cast_ZeroExtend: {
							// if we are casting from int to float
							if (!Statement.Expr()->GetType().IsFloating() && Statement.GetType().IsFloating()) output << "llvm::Instruction::UIToFP";
							// if we are casting from float to double
							else if (Statement.Expr()->GetType().IsFloating() && Statement.GetType().IsFloating())
								output << "llvm::Instruction::FPExt";
							else
								output << "llvm::Instruction::ZExt";
							break;
						}
						case SSACastStatement::Cast_Truncate: {
							// if we are casting from double to float
							if (Statement.Expr()->GetType().IsFloating() && Statement.GetType().IsFloating()) output << "llvm::Instruction::FPTrunc";
							// if we are casting from float to int
							else if (Statement.Expr()->GetType().IsFloating() && !Statement.GetType().IsFloating()) {
								if (Statement.GetType().Signed)
									output << "llvm::Instruction::FPToSI";
								else
									output << "llvm::Instruction::FPToUI";
							} else
								output << "llvm::Instruction::Trunc";
							break;
						}
						default:
							assert(false && "Unrecognized cast type");
					}

					output << ", " << Factory.GetOrCreate(Statement.Expr())->GetDynamicValue() << ", " << Statement.GetType().GetLLVMType() << ");";
					return true;
				}

				std::string GetDynamicValue() const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement &>(this->Statement);

					if (Statement.Expr()->IsFixed())
						return GetLLVMValue(Statement.GetType(), GetFixedValue());
					else
						return Statement.GetName();
				}
			};

			class SSAConstantStatementWalker : public SSANodeWalker
			{
			public:
				SSAConstantStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : SSANodeWalker(_statement, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					// output << Type.GetCType() << " " << GetName() << "= " << "(" << Type.GetCType() << ")" << Constant << ";";
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					assert(false && "Unimplemented");
					return true;
				}

				std::string GetDynamicValue() const
				{
					const SSAConstantStatement &Statement = static_cast<const SSAConstantStatement &>(this->Statement);
					if(Statement.GetType().IsFloating()) {
						UNIMPLEMENTED;
					}

					std::stringstream s;
					s << Statement.Constant.Int() << "ULL";
					return GetLLVMValue(Statement.GetType(), s.str());
				}

				std::string GetFixedValue() const
				{
					const SSAConstantStatement &Statement = static_cast<const SSAConstantStatement &>(this->Statement);
					if(Statement.GetType().IsFloating()) {
						UNIMPLEMENTED;
					}


					std::stringstream str;
					str << "(" << Statement.GetType().GetCType() << ")" << Statement.Constant.Int() << "ULL";
					return str.str();
				}
			};

			class SSADeviceReadStatementWalker : public SSANodeWalker
			{
			public:
				SSADeviceReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : SSANodeWalker(_statement, *_fact) {}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label, bool fully_fixed) const
				{
					const SSADeviceReadStatement *stmt = dynamic_cast<const SSADeviceReadStatement*>(&Statement);
					assert(stmt);

					SSANodeWalker *arg0 = Factory.GetOrCreate(stmt->Device());
					SSANodeWalker *arg1 = Factory.GetOrCreate(stmt->Address());

					output << "llvm::Value *" << Statement.GetName() << " = 0;";
					output << "{";

					output << "llvm::Value *ptr = llvm_registers[__idx_" << stmt->Target()->GetName() << "];";
					output << Statement.GetName() << " = __irBuilder.CreateCall4(txln_ctx.jit_functions.dev_read_device, ctx.block.region.values.cpu_ctx_val, " << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ", ptr, \"dev_read_result\");";
					output << "}";

					return true;
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					assert(false);
					return false;
				}
			};

			class SSAIntrinsicStatementWalker : public SSANodeWalker
			{
			public:
				SSAIntrinsicStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : SSANodeWalker(_statement, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &>(this->Statement);

					const SSANodeWalker *arg0 = nullptr;
					if (Statement.ArgCount() > 0) {
						arg0 = Factory.GetOrCreate(Statement.Args(0));
					}

					switch (Statement.Type) {
						case SSAIntrinsicStatement::SSAIntrinsic_ReadPc:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = "
							       << "ctx.block.region.CreateMaterialise(ctx.tiu.GetOffset());\n";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_Popcount32:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __builtin_popcount(" << arg0->GetFixedValue() << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_Clz32:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __builtin_clz(" << arg0->GetFixedValue() << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = " << Statement.Parent->Parent->GetAction()->Context.ISA.isa_mode_id << ";";
							break;
						default:
							assert(false && "Unimplemented");
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &>(this->Statement);

					const SSANodeWalker *arg0 = NULL;
					const SSANodeWalker *arg1 = NULL;
					const SSANodeWalker *arg2 = NULL;
					const SSANodeWalker *arg3 = NULL;

					if (Statement.ArgCount() > 0) arg0 = Factory.GetOrCreate(Statement.Args(0));
					if (Statement.ArgCount() > 1) arg1 = Factory.GetOrCreate(Statement.Args(1));
					if (Statement.ArgCount() > 2) arg2 = Factory.GetOrCreate(Statement.Args(2));
					if (Statement.ArgCount() > 3) arg3 = Factory.GetOrCreate(Statement.Args(3));

					output << "//emit intrinsic here " << Statement.Type << "\n";
					// if (HasValue()) output << GetType().GetCType() << " " << GetName() << " = 0;";
					switch (Statement.Type) {
						case SSAIntrinsicStatement::SSAIntrinsic_ReadPc:
							output << "llvm::Value *" << Statement.GetName() << " = " << "ctx.block.region.CreateMaterialise(ctx.tiu.GetOffset());\n";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_Popcount32:
							assert(arg0);
							output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(__module->getFunction(\"llvm.ctpop.i32\"), " << arg0->GetDynamicValue() << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_Clz32:
							assert(arg0);
							output << "std::vector<llvm::Type*> args;"
							       "args.push_back(txln_ctx.types.i32);"
							       "args.push_back(txln_ctx.types.i1);"
							       "llvm::FunctionType *intrinsicType = llvm::FunctionType::get(txln_ctx.types.i32, args, false);"
							       "llvm::Function *intrinsic = (llvm::Function*)ctx.block.region.region_fn->getParent()->getOrInsertFunction(\"llvm.ctlz.i32\", intrinsicType);"
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall2(intrinsic, " << arg0->GetDynamicValue() << ", llvm::ConstantInt::get(txln_ctx.types.i1, 0, false));";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_SetCpuMode:
							assert(arg0);
							output << "{";
							output << "llvm::Value *isa_mode_ptr = __irBuilder.CreateConstGEP2_32(ctx.block.region.values.state_val, 0, gensim::CpuStateEntries::CpuState_isa_mode);";
							output << "ctx.AddAliasAnalysisNode((llvm::Instruction *)isa_mode_ptr, archsim::translate::llvm::TAG_CPU_STATE);";
							output << "__irBuilder.CreateStore(" << arg0->GetDynamicValue() << ", isa_mode_ptr);";
							output << "}";

							break;
						case SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode:
							output << "llvm::Value *" << Statement.GetName() << " = NULL;";

							output << "{";
							output << "llvm::Value *isa_mode_ptr = __irBuilder.CreateConstGEP2_32(cpuState, 0, gensim::CpuStateEntries::CpuState_isa_mode);";
							output << "ctx.AddAliasAnalysisNode((llvm::Instruction *)isa_mode_ptr, archsim::translate::llvm::TAG_CPU_STATE);";
							output << Statement.GetName() << " = __irBuilder.CreateLoad(isa_mode_ptr);";
							output << "}";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_TakeException:
							assert(arg0 && arg1);
							output << "ctx.block.region.EmitTakeException(ctx.tiu.GetOffset(), " << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_ProbeDevice:
							assert(arg0);
							output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall2(txln_ctx.jit_functions.dev_probe_device, ctx.block.region.values.cpu_ctx_val, " << arg0->GetDynamicValue() << ", \"dev_probe_result\");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_WriteDevice:
							assert(arg0 && arg1 && arg2);
							output << "llvm::Value *" << Statement.GetName();
							output << " = __irBuilder.CreateCall4(txln_ctx.jit_functions.dev_write_device, ctx.block.region.values.cpu_ctx_val, ";
							output << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ", " << arg2->GetDynamicValue() << ", \"dev_write_result\");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_PushInterrupt:
							output << "__irBuilder.CreateCall2(txln_ctx.jit_functions.cpu_push_interrupt, ctx.block.region.values.cpu_ctx_val, " << GetLLVMValue(IRTypes::UInt32, "0") << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_PopInterrupt:
							output << "__irBuilder.CreateCall(txln_ctx.jit_functions.cpu_pop_interrupt, ctx.block.region.values.cpu_ctx_val);";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_Trap:
							output << "__irBuilder.CreateCall(txln_ctx.jit_functions.jit_trap);";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_PendIRQ:
							output << "__irBuilder.CreateCall(txln_ctx.jit_functions.cpu_pend_irq, ctx.block.region.values.cpu_ctx_val);";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_HaltCpu:
							output << "__irBuilder.CreateCall(txln_ctx.jit_functions.cpu_halt, ctx.block.region.values.cpu_ctx_val);";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_EnterKernelMode:
							output << "__irBuilder.CreateCall(txln_ctx.jit_functions.cpu_enter_kernel, ctx.block.region.values.cpu_ctx_val);";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_EnterUserMode:
							output << "__irBuilder.CreateCall(txln_ctx.jit_functions.cpu_enter_user, ctx.block.region.values.cpu_ctx_val);";
							break;

						case SSAIntrinsicStatement::SSAIntrinsic_FloatIsQnan:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = NULL;"
							       "{"
							       "	llvm::Value *val = __irBuilder.CreateBitCast(" << arg0->GetDynamicValue() << ", txln_ctx.types.i32);"
							       "	llvm::Value *exp = __irBuilder.CreateAnd(val, 0x7fc00000);"
							       "	val = __irBuilder.CreateICmpEQ(exp, " << GetLLVMValue(IRTypes::UInt32, "0x7fc00000") << ");"
							       "	" << Statement.GetName() << " = __irBuilder.CreateZExtOrBitCast(val, txln_ctx.types.i8);"
							       "}";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_FloatIsSnan:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = NULL;"
							       "{"
							       "	llvm::Value *val = __irBuilder.CreateBitCast(" << arg0->GetDynamicValue() << ", txln_ctx.types.i32);"
							       "	llvm::Value *exp = __irBuilder.CreateAnd(val, 0x7fc00000);"
							       "	val = __irBuilder.CreateICmpEQ(exp, " << GetLLVMValue(IRTypes::UInt32, "0x7f800000") << ");"
							       "	" << Statement.GetName() << " = __irBuilder.CreateZExtOrBitCast(val, txln_ctx.types.i8);"
							       "}";
							break;

						case SSAIntrinsicStatement::SSAIntrinsic_DoubleIsQnan:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = NULL;"
							       "{"
							       "	llvm::Value *val = __irBuilder.CreateBitCast(" << arg0->GetDynamicValue() << ", txln_ctx.types.i64);"
							       "	llvm::Value *exp = __irBuilder.CreateAnd(val, 0x7ffc000000000000ULL);"
							       "	val = __irBuilder.CreateICmpEQ(exp, " << GetLLVMValue(IRTypes::UInt64, "0x7ffc000000000000") << ");"
							       "	" << Statement.GetName() << " = __irBuilder.CreateZExtOrBitCast(val, txln_ctx.types.i8);"
							       "}";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_DoubleIsSnan:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = NULL;"
							       "{"
							       "	llvm::Value *val = __irBuilder.CreateBitCast(" << arg0->GetDynamicValue() << ", txln_ctx.types.i64);"
							       "	llvm::Value *exp = __irBuilder.CreateAnd(val, 0x7ffc000000000000);"
							       "	val = __irBuilder.CreateICmpEQ(exp, " << GetLLVMValue(IRTypes::UInt64, "0x7ff8000000000000") << ");"
							       "	" << Statement.GetName() << " = __irBuilder.CreateZExtOrBitCast(val, txln_ctx.types.i8);"
							       "}";
							break;

						case SSAIntrinsicStatement::SSAIntrinsic_DoubleSqrt:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(txln_ctx.jit_functions.double_sqrt, " << arg0->GetDynamicValue() << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_FloatSqrt:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(txln_ctx.jit_functions.float_sqrt, " << arg0->GetDynamicValue() << ");";
							break;

						case SSAIntrinsicStatement::SSAIntrinsic_DoubleAbs:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(txln_ctx.jit_functions.double_abs, " << arg0->GetDynamicValue() << ");";
							break;
						case SSAIntrinsicStatement::SSAIntrinsic_FloatAbs:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(txln_ctx.jit_functions.float_abs, " << arg0->GetDynamicValue() << ");";
							break;

						case SSAIntrinsicStatement::SSAIntrinsic_AdcWithFlags: {
							auto V = Statement.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetTaggedRegSlot("V");
							auto C = Statement.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetTaggedRegSlot("C");
							auto Z = Statement.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetTaggedRegSlot("Z");
							auto N = Statement.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetTaggedRegSlot("N");
							output << "{";
							output << "llvm::Value *res = __irBuilder.CreateCall3(txln_ctx.jit_functions.genc_adc_flags, " << arg0->GetDynamicValue() << "," << arg1->GetDynamicValue() << ", " << arg2->GetDynamicValue() << ");";
							output << "llvm::Value *c = __irBuilder.CreateLShr(res, 8);"
							       "c = __irBuilder.CreateAnd(c, 1);";
							output << "llvm::Value *v = res;"
							       "v = __irBuilder.CreateAnd(v, 1);";
							output << "llvm::Value *z = __irBuilder.CreateLShr(res, 14);"
							       "z = __irBuilder.CreateAnd(z, 1);";
							output << "llvm::Value *n = __irBuilder.CreateLShr(res, 15);"
							       "n = __irBuilder.CreateAnd(n, 1);";

							output << "EmitRegisterWrite(ctx, " << (uint32_t)C->GetIndex() << ", __irBuilder.CreateIntCast(c, " << C->GetIRType().GetLLVMType() << ", false) , __trace);";
							output << "EmitRegisterWrite(ctx, " << (uint32_t)V->GetIndex() << ", __irBuilder.CreateIntCast(v, " << V->GetIRType().GetLLVMType() << ", false) , __trace);";
							output << "EmitRegisterWrite(ctx, " << (uint32_t)Z->GetIndex() << ", __irBuilder.CreateIntCast(z, " << Z->GetIRType().GetLLVMType() << ", false) , __trace);";
							output << "EmitRegisterWrite(ctx, " << (uint32_t)N->GetIndex() << ", __irBuilder.CreateIntCast(n, " << N->GetIRType().GetLLVMType() << ", false) , __trace);";
							output << "}";
							break;
						}

						case SSAIntrinsicStatement::SSAIntrinsic_UpdateZN32: {
							auto Z = Statement.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetTaggedRegSlot("Z");
							auto N = Statement.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetTaggedRegSlot("N");

							output << "{"
							       "llvm::Value *n = __irBuilder.CreateLShr(" << arg0->GetDynamicValue() << ", 31);"
							       "llvm::Value *z = __irBuilder.CreateICmpEQ(" << arg0->GetDynamicValue() << ", llvm::ConstantInt::get(txln_ctx.types.i32, 0, false));"
							       "EmitRegisterWrite(ctx, " << (uint32_t)Z->GetIndex() << ", __irBuilder.CreateIntCast(z, " << Z->GetIRType().GetLLVMType() << ", false), __trace);"
							       "EmitRegisterWrite(ctx, " << (uint32_t)N->GetIndex() << ", __irBuilder.CreateIntCast(n, " << N->GetIRType().GetLLVMType() << ", false), __trace);"
							       "}";
							break;
						}

						case SSAIntrinsicStatement::SSAIntrinsic_FPSetRounding:
						case SSAIntrinsicStatement::SSAIntrinsic_FPGetRounding:
						case SSAIntrinsicStatement::SSAIntrinsic_FPSetFlush:
						case SSAIntrinsicStatement::SSAIntrinsic_FPGetFlush:
							output << "llvm::Value *" << Statement.GetName() << " = nullptr; assert(false);";
							break;
						default:
							assert(false && "Unimplemented intrinsic");
							break;
					}
					return true;
				}

				std::string GetDynamicValue() const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &>(this->Statement);

					switch (Statement.Type) {
						case SSAIntrinsicStatement::SSAIntrinsic_Clz32:
						case SSAIntrinsicStatement::SSAIntrinsic_Popcount32:
						case SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode:
						case SSAIntrinsicStatement::SSAIntrinsic_ProbeDevice:
						case SSAIntrinsicStatement::SSAIntrinsic_ReadPc:
						case SSAIntrinsicStatement::SSAIntrinsic_WriteDevice:

						case SSAIntrinsicStatement::SSAIntrinsic_FloatIsSnan:
						case SSAIntrinsicStatement::SSAIntrinsic_FloatIsQnan:
						case SSAIntrinsicStatement::SSAIntrinsic_DoubleIsSnan:
						case SSAIntrinsicStatement::SSAIntrinsic_DoubleIsQnan:
						case SSAIntrinsicStatement::SSAIntrinsic_DoubleSqrt:
						case SSAIntrinsicStatement::SSAIntrinsic_FloatSqrt:
						case SSAIntrinsicStatement::SSAIntrinsic_DoubleAbs:
						case SSAIntrinsicStatement::SSAIntrinsic_FloatAbs:

						case SSAIntrinsicStatement::SSAIntrinsic_AdcWithFlags:

						case SSAIntrinsicStatement::SSAIntrinsic_FPGetRounding:
						case SSAIntrinsicStatement::SSAIntrinsic_FPGetFlush:

							return Statement.GetName();

						default:
							assert(false && "Unimplemented intrinsic");
							return "";  // Keep any semantic parsers happy
					}
				}
			};

			class SSAIfStatementWalker : public SSANodeWalker
			{
			public:
				SSAIfStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : SSANodeWalker(_statement, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIfStatement &Statement = static_cast<const SSAIfStatement &>(this->Statement);

					assert(Statement.Expr()->IsFixed());
					output << "if(" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ") {";
					JumpOrInlineBlock(Factory, *Statement.TrueTarget(), output, end_label, fully_fixed, false);
					output << "} else {";
					JumpOrInlineBlock(Factory, *Statement.FalseTarget(), output, end_label, fully_fixed, false);
					output << "}";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIfStatement &Statement = static_cast<const SSAIfStatement &>(this->Statement);

					output << "{";
					output << "llvm::Value *bitcast_expr = __irBuilder.CreateICmp(llvm::CmpInst::ICMP_NE, " << Factory.GetOrCreate(Statement.Expr())->GetDynamicValue() << ", " << GetLLVMValue(Statement.Expr()->GetType(), "0") << ");";
					// add the target blocks to the dynamic block queue
					CreateBlock(*Statement.TrueTarget(), output);
					CreateBlock(*Statement.FalseTarget(), output);
					output << "__irBuilder.CreateCondBr(bitcast_expr, dynamic_blocks[__block_" << Statement.TrueTarget()->GetName() << "], dynamic_blocks[__block_" << Statement.FalseTarget()->GetName() << "]);";
					output << "goto " << end_label << ";";
					output << "}";

					return true;
				}
			};

			class SSAJumpStatementWalker : public SSANodeWalker
			{
			public:
				SSAJumpStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : SSANodeWalker(_statement, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAJumpStatement &Statement = static_cast<const SSAJumpStatement &>(this->Statement);

					if (fully_fixed)
						JumpOrInlineBlock(Factory, *Statement.Target(), output, end_label, fully_fixed, false);
					else {
						assert(Statement.Target()->IsFixed() != BLOCK_ALWAYS_CONST);
						EmitDynamicCode(output, end_label, fully_fixed);
					}
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAJumpStatement &Statement = static_cast<const SSAJumpStatement &>(this->Statement);

					CreateBlock(*Statement.Target(), output);
					output << "__irBuilder.CreateBr(dynamic_blocks[__block_" << Statement.Target()->GetName() << "]);";
					return true;
				}
			};

			class SSAMemoryReadStatementWalker : public SSANodeWalker
			{
			public:
				SSAMemoryReadStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				// TODO: Add tracing to fast_same_page_literal path
				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAMemoryReadStatement &Statement = static_cast<const SSAMemoryReadStatement &>(this->Statement);

					const SSANodeWalker *AddrExpr = Factory.GetOrCreate(Statement.Addr());
					output << "llvm::Value *" << Statement.GetName() << " = NULL;";

					if(Statement.User)
						output << "txln_ctx.mtm.EmitNonPrivilegedRead(ctx, " << (uint32_t)Statement.Width << ", " << Statement.Signed << ", " << Statement.GetName() << ", " << AddrExpr->GetDynamicValue() << ", " << Statement.Target()->GetType().GetLLVMType() << ", llvm_registers[__idx_" << Statement.Target()->GetName() << "]);";
					else
						output << "txln_ctx.mtm.EmitMemoryRead(ctx, " << (uint32_t)Statement.Width << ", " << Statement.Signed << ", " << Statement.GetName() << ", " << AddrExpr->GetDynamicValue() << ", " << Statement.Target()->GetType().GetLLVMType() << ", llvm_registers[__idx_" << Statement.Target()->GetName() << "]);";

					output << "if (__trace) {";
					output << "llvm::Value *val = __irBuilder.CreateLoad(llvm_registers[__idx_" << Statement.Target()->GetName() << "]);";

					switch (Statement.Width) {
						case 1:
							output << "__irBuilder.CreateCall3(txln_ctx.jit_functions.trace_mem_read_8, ctx.block.region.values.cpu_ctx_val, " << AddrExpr->GetDynamicValue() << ", __irBuilder.CreateCast(llvm::Instruction::ZExt, val, txln_ctx.types.i32));";
							break;
						case 2:
							output << "__irBuilder.CreateCall3(txln_ctx.jit_functions.trace_mem_read_16, ctx.block.region.values.cpu_ctx_val, " << AddrExpr->GetDynamicValue() << ", __irBuilder.CreateCast(llvm::Instruction::ZExt, val, txln_ctx.types.i32));";
							break;
						case 4:
							output << "__irBuilder.CreateCall3(txln_ctx.jit_functions.trace_mem_read_32, ctx.block.region.values.cpu_ctx_val, " << AddrExpr->GetDynamicValue() << ", val);";
							break;
						default:
							assert(false);
					}

					output << "}";
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					EmitFixedCode(output, end_label, fully_fixed);
					return true;
				}
			};

			class SSAMemoryWriteStatementWalker : public SSANodeWalker
			{
			public:
				SSAMemoryWriteStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAMemoryWriteStatement &Statement = static_cast<const SSAMemoryWriteStatement &>(this->Statement);

					const SSANodeWalker *AddrExpr = Factory.GetOrCreate(Statement.Addr());
					const SSANodeWalker *ValueExpr = Factory.GetOrCreate(Statement.Value());

					output << "llvm::Value *" << Statement.GetName() << " = NULL;";

					if(Statement.User)
						output << "txln_ctx.mtm.EmitNonPrivilegedWrite(ctx, " << (uint32_t)Statement.Width << ", " << Statement.GetName() << ", " << AddrExpr->GetDynamicValue() << ", " << ValueExpr->GetDynamicValue() << ");";
					else
						output << "txln_ctx.mtm.EmitMemoryWrite(ctx, " << (uint32_t)Statement.Width << ", " << Statement.GetName() << ", " << AddrExpr->GetDynamicValue() << ", " << ValueExpr->GetDynamicValue() << ");";

					output << "if (__trace) {";

					switch (Statement.Width) {
						case 1:
							output << "__irBuilder.CreateCall3(txln_ctx.jit_functions.trace_mem_write_8, ctx.block.region.values.cpu_ctx_val, " << AddrExpr->GetDynamicValue() << ", __irBuilder.CreateCast(llvm::Instruction::ZExt, " << ValueExpr->GetDynamicValue() << ", txln_ctx.types.i32));";
							break;
						case 2:
							output << "__irBuilder.CreateCall3(txln_ctx.jit_functions.trace_mem_write_16, ctx.block.region.values.cpu_ctx_val, " << AddrExpr->GetDynamicValue() << ", __irBuilder.CreateCast(llvm::Instruction::ZExt, " << ValueExpr->GetDynamicValue() << ", txln_ctx.types.i32));";
							break;
						case 4:
							output << "__irBuilder.CreateCall3(txln_ctx.jit_functions.trace_mem_write_32, ctx.block.region.values.cpu_ctx_val, " << AddrExpr->GetDynamicValue() << ", " << ValueExpr->GetDynamicValue() << ");";
							break;
						default:
							assert(false);
					}

					output << "}";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return EmitFixedCode(output, end_label, fully_fixed);
				}
			};

			class SSAReturnStatementWalker : public SSANodeWalker
			{
			public:
				SSAReturnStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAReturnStatement &Statement = static_cast<const SSAReturnStatement &>(this->Statement);
					SSANodeWalker *Value = NULL;
					if (Statement.Value()) Value = Factory.GetOrCreate(Statement.Value());

					if (fully_fixed) {
						if (Value != NULL)
							output << "__result = " << Value->GetDynamicValue() << ";";
						output << "goto free_variables;";
					} else {
						return EmitDynamicCode(output, end_label, fully_fixed);
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAReturnStatement &Statement = static_cast<const SSAReturnStatement &>(this->Statement);
					SSANodeWalker *Value = NULL;
					if (Statement.Value()) Value = Factory.GetOrCreate(Statement.Value());

					if (Value != NULL)
						output << "__result = " << Value->GetDynamicValue() << ";";

					// emit an llvm jump to the end of the instruction
					output << "__irBuilder.CreateBr(__end_of_instruction);";
					output << "goto " << end_label << ";";

					return true;
				}
			};

			class SSAReadStructMemberStatementWalker : public SSANodeWalker
			{
			public:
				SSAReadStructMemberStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					// nothing to do here since struct members can be obtained through values
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					assert(false && "Unimplemented");
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAReadStructMemberStatement &Statement = static_cast<const SSAReadStructMemberStatement &>(this->Statement);

					std::stringstream str;
					if (Statement.Index != -1) str << "((uint32*)(";
					// XXX just assume we only ever access the inst struct
					// str << "CV_" << TargetStruct->GetName() << "->" << MemberName;

					if(Statement.MemberName == "IsPredicated") {
						str << "CV_top_inst->GetIsPredicated()";
					} else if(Statement.MemberName == "PredicateInfo") {
						str << "CV_top_inst->GetPredicateInfo()";
					} else {
						str << "CV_top_inst"
						    << "->" << Statement.MemberName;
					}
					// XXX
					if (Statement.Index != -1) str << "))[" << Statement.Index << "]";
					return str.str();
				}

				std::string GetDynamicValue() const
				{
					const SSAReadStructMemberStatement &Statement = static_cast<const SSAReadStructMemberStatement &>(this->Statement);

					std::stringstream str;
					if (Statement.Index != -1) str << "((uint32*)(";
					// XXX just assume we only ever access the inst struct
					// str << "CV_" << TargetStruct->GetName() << "->" << MemberName;
					str << "CV_top_inst"
					    << "->" << Statement.MemberName;
					// XXX
					if (Statement.Index != -1) str << "))[" << Statement.Index << "]";
					return GetLLVMValue(Statement.GetType(), str.str());
				}
			};

			class SSARegisterStatementWalker : public SSANodeWalker
			{
			public:
				SSARegisterStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSARegisterStatement &Statement = static_cast<const SSARegisterStatement &>(this->Statement);

					const auto &Arch = Statement.Parent->Parent->GetAction()->Context.Arch;

					SSANodeWalker *ValueExpr = nullptr;
					if (Statement.Value()) ValueExpr = Factory.GetOrCreate(Statement.Value());

					SSANodeWalker *RegnumExpr = nullptr;
					if (Statement.RegNum()) RegnumExpr = Factory.GetOrCreate(Statement.RegNum());

					assert(!Statement.IsRead);

					if (Statement.IsBanked) {
						const IRType &value_type = Arch.GetRegFile().GetBankByIdx(Statement.Bank).GetRegisterIRType();
						output << "EmitBankedRegisterWrite(ctx, " << (uint32_t)Statement.Bank << ", " << RegnumExpr->GetDynamicValue() << ", "
						       "__irBuilder.CreateIntCast(" << ValueExpr->GetDynamicValue() << ", " << value_type.GetLLVMType() << ", false)" <<  // cast the written value to the correct type
						       ", __trace);";
					} else {
						const IRType &value_type = Arch.GetRegFile().GetSlotByIdx(Statement.Bank).GetIRType();
						output << "EmitRegisterWrite(ctx, " << (uint32_t)Statement.Bank << ", "
						       "__irBuilder.CreateIntCast(" << ValueExpr->GetDynamicValue() << ", " << value_type.GetLLVMType() << ", false)" <<  // cast the written value to the correct type
						       ", __trace);";
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSARegisterStatement &Statement = static_cast<const SSARegisterStatement &>(this->Statement);
					const auto &Arch = Statement.Parent->Parent->GetAction()->Context.Arch;

					SSANodeWalker *ValueExpr = NULL;
					if (Statement.Value()) ValueExpr = Factory.GetOrCreate(Statement.Value());

					SSANodeWalker *RegnumExpr = NULL;
					if (Statement.RegNum()) RegnumExpr = Factory.GetOrCreate(Statement.RegNum());

					if (!Statement.IsRead) {
						if (Statement.IsBanked) {
							const IRType &value_type = Arch.GetRegFile().GetBankByIdx(Statement.Bank).GetRegisterIRType();
							output << "EmitBankedRegisterWrite(ctx, " << (uint32_t)Statement.Bank << ", " << RegnumExpr->GetDynamicValue() << ", "
							       "__irBuilder.CreateIntCast(" << ValueExpr->GetDynamicValue() << ", " << value_type.GetLLVMType() << ", false)" <<  // cast the written value to the correct type
							       ", __trace);";
						} else {
							const IRType &value_type = Arch.GetRegFile().GetSlotByIdx(Statement.Bank).GetIRType();
							output << "EmitRegisterWrite(ctx, " << (uint32_t)Statement.Bank << ", "
							       "__irBuilder.CreateIntCast(" << ValueExpr->GetDynamicValue() << ", " << value_type.GetLLVMType() << ", false)" <<  // cast the written value to the correct type
							       ", __trace);";
						}
					} else {
						output << "llvm::Value *" << Statement.GetName() << ";";
						if (Statement.IsBanked)
							output << "EmitBankedRegisterRead(ctx, " << Statement.GetName() << ", " << (uint32_t)Statement.Bank << ", " << RegnumExpr->GetDynamicValue() << ", __trace);";
						else
							output << "EmitRegisterRead(ctx, " << Statement.GetName() << ", " << (uint32_t)Statement.Bank << ", __trace);";
					}

					return true;
				}
			};

			class SSASelectStatementWalker : public SSANodeWalker
			{
			public:
				SSASelectStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASelectStatement &Statement = static_cast<const SSASelectStatement &>(this->Statement);

					SSANodeWalker *Cond = Factory.GetOrCreate(Statement.Cond());
					SSANodeWalker *True = Factory.GetOrCreate(Statement.TrueVal());
					SSANodeWalker *False = Factory.GetOrCreate(Statement.FalseVal());

					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = ((" << Statement.GetType().GetCType() << ")";
					output << "(" << Cond->GetFixedValue() << "?" << True->GetFixedValue() << " : " << False->GetFixedValue() << "));";
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASelectStatement &Statement = static_cast<const SSASelectStatement &>(this->Statement);

					SSANodeWalker *Cond = Factory.GetOrCreate(Statement.Cond());
					SSANodeWalker *True = Factory.GetOrCreate(Statement.TrueVal());
					SSANodeWalker *False = Factory.GetOrCreate(Statement.FalseVal());

					if(Cond->Statement.IsFixed()) {
						output << "llvm::Value *" << Statement.GetName() << ";";
						output << "if(" << Cond->GetFixedValue() << ") " << Statement.GetName() << " = " << True->GetDynamicValue() << ";";
						output << "else " << Statement.GetName() << " = " << False->GetDynamicValue() << ";";
					} else {
						output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateSelect(__irBuilder.CreateIntCast(" << Cond->GetDynamicValue() << ", txln_ctx.types.i1, false), " << True->GetDynamicValue() << ", " << False->GetDynamicValue() << ");";
					}

					return true;
				}

				std::string GetDynamicValue() const
				{
					if (Statement.IsFixed())
						return GetLLVMValue(Statement.GetType(), Statement.GetName());
					else
						return Statement.GetName();
				}
			};

			class SSASwitchStatementWalker : public SSANodeWalker
			{
			public:
				SSASwitchStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASwitchStatement &Statement = static_cast<const SSASwitchStatement &>(this->Statement);

					SSANodeWalker *Expr = Factory.GetOrCreate(Statement.Expr());

					output << "switch(" << Expr->GetFixedValue() << ") {";
					// Emit each case statement in turn
					for (auto v : Statement.GetValues()) {
						SSANodeWalker *c = Factory.GetOrCreate(v.first);
						output << "case " << c->GetFixedValue() << ":";
						output << "{";
						JumpOrInlineBlock(Factory, *v.second, output, end_label, fully_fixed, false);
						output << "}";
					}
					output << "default: {";
					JumpOrInlineBlock(Factory, *Statement.Default(), output, end_label, fully_fixed, false);

					output << "}}";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASwitchStatement &Statement = static_cast<const SSASwitchStatement &>(this->Statement);

					SSANodeWalker *Expr = Factory.GetOrCreate(Statement.Expr());

					CreateBlock(*Statement.Default(), output);
					output << "{ llvm::SwitchInst * sw = __irBuilder.CreateSwitch(" << Expr->GetDynamicValue() << ", dynamic_blocks[__block_" << Statement.Default()->GetName() << "]);";
					for (std::map<SSAStatement *, SSABlock *>::const_iterator v_i = Statement.GetValues().begin(); v_i != Statement.GetValues().end(); ++v_i) {
						SSAStatement *v = v_i->first;
						SSABlock *b = v_i->second;
						SSAConstantStatement *c = dynamic_cast<SSAConstantStatement *>(v);
						assert(c);
						CreateBlock(*b, output);
						std::stringstream constantStream;
						constantStream << c->Constant.Int();
						output << "sw->addCase((llvm::ConstantInt*)" << GetLLVMValue(c->GetType(), constantStream.str()) << ", dynamic_blocks[__block_" << b->GetName() << "]);";
					}
					output << "}";
					output << "goto " << end_label << ";\n";

					return true;
				}
			};

			class SSAVariableReadStatementWalker : public SSANodeWalker
			{
			public:
				SSAVariableReadStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					// variables shouldn't be emitted as their own entire statement so nothing to do here
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableReadStatement &Statement = static_cast<const SSAVariableReadStatement &>(this->Statement);

					output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateLoad(llvm_registers[__idx_" << Statement.Target()->GetName() << "]);";
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAVariableReadStatement &Statement = static_cast<const SSAVariableReadStatement &>(this->Statement);
					return "CV_" + Statement.Target()->GetName();
				}

				std::string GetDynamicValue() const
				{
					const SSAVariableReadStatement &Statement = static_cast<const SSAVariableReadStatement &>(this->Statement);

					// If this is a vector, we need to build the LLVM vector which represents it
					if(Statement.GetType().VectorWidth > 1) {
						// start with an undef vector of the correct type
						// TODO: implement this
						return "(assert(false), nullptr)";

					} else {
						if (Statement.IsFixed()) return GetLLVMValue(Statement.Target()->GetType(), "CV_" + Statement.Target()->GetName());
						return Statement.GetName();
					}
				}
			};

			class SSAVariableWriteStatementWalker : public SSANodeWalker
			{
			public:
				SSAVariableWriteStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableWriteStatement &Statement = static_cast<const SSAVariableWriteStatement &>(this->Statement);
					SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());

					output << "CV_" << Statement.Target()->GetName() << " = " << expr->GetFixedValue() << ";";

					// If we should lower this statement
					if (Statement.Parent->Parent->HasDynamicDominatedReads(&Statement)) {
						output << "__irBuilder.CreateStore(" << GetLLVMValue(Statement.Target()->GetType(), "CV_" + Statement.Target()->GetName()) << ", llvm_registers[__idx_" << Statement.Target()->GetName() << "]);";
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableWriteStatement &Statement = static_cast<const SSAVariableWriteStatement &>(this->Statement);
					SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());
					output << "__irBuilder.CreateStore((llvm::Value*)" << expr->GetDynamicValue() << ", llvm_registers[__idx_" << Statement.Target()->GetName() << "]);";

					return true;
				}
			};

			class SSACallStatementWalker : public SSANodeWalker
			{
			public:
				SSACallStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					// calls are never fixed
					assert(false);
					return false;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSACallStatement &Statement = static_cast<const SSACallStatement&>(this->Statement);

#if 0
				case SSAIntrinsicStatement::SSAIntrinsic_Flush:
					output << "__irBuilder.CreateCall(txln_ctx.jit_functions.tm_flush, ctx.block.region.values.cpu_ctx_val);";
					break;
				case SSAIntrinsicStatement::SSAIntrinsic_FlushITlb:
					output << "__irBuilder.CreateCall(txln_ctx.jit_functions.tm_flush_itlb, ctx.block.region.values.cpu_ctx_val);";
					break;
				case SSAIntrinsicStatement::SSAIntrinsic_FlushDTlb:
					output << "__irBuilder.CreateCall(txln_ctx.jit_functions.tm_flush_dtlb, ctx.block.region.values.cpu_ctx_val);";
					break;
				case SSAIntrinsicStatement::SSAIntrinsic_FlushITlbEntry:
					assert(arg0);
					output << "__irBuilder.CreateCall2(txln_ctx.jit_functions.tm_flush_itlb_entry, ctx.block.region.values.cpu_ctx_val, " << arg0->GetDynamicValue() << ");";
					break;
				case SSAIntrinsicStatement::SSAIntrinsic_FlushDTlbEntry:
					assert(arg0);
					output << "__irBuilder.CreateCall2(txln_ctx.jit_functions.tm_flush_dtlb_entry, ctx.block.region.values.cpu_ctx_val, " << arg0->GetDynamicValue() << ");";
					break;
#endif

					if(Statement.HasValue()) output << "::llvm::Value *" << Statement.GetName() << ";";
					output << "{";
					//first, figure out the type of the function we're calling (+1 because of CPU context - thiscall)
					uint32_t num_params = Statement.Target()->GetPrototype().GetIRSignature().GetParams().size() + 1;

					//Figure out the return type of the function
					std::string return_type = "txln_ctx.types.vtype";
					if(Statement.HasValue()) return_type = Statement.GetType().GetLLVMType();

					//Build a parameter type list to get the correct function type
					output << "llvm::Type *params[] { ctx.block.region.values.cpu_ctx_val->getType() ";
					for(auto param : Statement.Target()->GetPrototype().GetIRSignature().GetParams()) {
						assert(!param.GetType().Reference && "Cannot make function calls with references yet!");
						output << ", " << param.GetType().GetLLVMType();
					}
					output << "};";

					output << "llvm::FunctionType *callee_type = llvm::FunctionType::get(" << return_type << ", llvm::ArrayRef<llvm::Type*>(params, " << num_params << "), false);";

					//get the function pointer to cast
					output << "llvm::Value *llvm_fn_ptr = ctx.block.region.txln.llvm_module->getOrInsertFunction(\"txln_shunt_" << Statement.Target()->GetPrototype().GetIRSignature().GetName() << "\", callee_type);";

					//build the args list
					output << "llvm::Value *args[] { ctx.block.region.values.cpu_ctx_val ";
					for(unsigned i = 0; i < Statement.ArgCount(); ++i) {
						auto param = Statement.Arg(i);
						output << ", " << Factory.GetOrCreate(dynamic_cast<const SSAStatement*>(param))->GetDynamicValue();
					}
					output << "};";

					output << "llvm::CallInst *call = __irBuilder.CreateCall(llvm_fn_ptr, llvm::ArrayRef<llvm::Value*>(args, " << num_params << "));";
//		output << "call->setCallingConv(llvm::CallingConv::C);";
					if(Statement.HasValue()) output << Statement.GetName() << " = call;";
					output << "}";

					return true;
				}
			};

			class SSAVectorExtractElementStatementWalker : public SSANodeWalker
			{
			public:
				SSAVectorExtractElementStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					// calls are never fixed
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << ";";
					output << "assert(false);";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVectorExtractElementStatement *stmt = dynamic_cast<const SSAVectorExtractElementStatement*>(&Statement);
					const SSAStatement *base = stmt->Base();
					const SSAStatement *index = stmt->Index();

					output << "llvm::Value* " << Statement.GetName() << " = __irBuilder.CreateExtractElement(" << Factory.GetOrCreate(base)->GetDynamicValue() << ", " << Factory.GetOrCreate(index)->GetDynamicValue() << ");";

					return true;
				}
			};


			class SSAVectorInsertElementStatementWalker : public SSANodeWalker
			{
			public:
				SSAVectorInsertElementStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					// calls are never fixed
					output << "assert(false);";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVectorInsertElementStatement *stmt = dynamic_cast<const SSAVectorInsertElementStatement*>(&Statement);
					const SSAStatement *base = stmt->Base();
					const SSAStatement *index = stmt->Index();
					const SSAStatement *value = stmt->Value();

					output <<
					       "llvm::Value *" << Statement.GetName() << ";"
					       "" << Statement.GetName() << " = __irBuilder.CreateInsertElement(" << Factory.GetOrCreate(base)->GetDynamicValue() << ", " << Factory.GetOrCreate(value)->GetDynamicValue() << ", " << Factory.GetOrCreate(index)->GetDynamicValue() << ");";

					return true;
				}
			};

		}
	}

#define STMT_IS(x) std::type_index(typeid(genc::ssa::x)) == std::type_index(typeid(*stmt))
#define HANDLE(x) \
	if (STMT_IS(x)) return new gensim::genc::ssa::x##Walker(*stmt, this)
	genc::ssa::SSANodeWalker *generator::DynamicTranslationNodeWalkerFactory::Create(const genc::ssa::SSAStatement *stmt)
	{
		assert(stmt != NULL);

		HANDLE(SSABinaryArithmeticStatement);
		HANDLE(SSAUnaryArithmeticStatement);
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
		HANDLE(SSAVectorExtractElementStatement);
		HANDLE(SSAVectorInsertElementStatement);
		HANDLE(SSACallStatement);
		assert(false && "Unrecognized statement type");
		return NULL;
	}
#undef STMT_IS
}
