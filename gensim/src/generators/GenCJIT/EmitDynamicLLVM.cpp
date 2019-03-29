/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

#define INSTRUMENT_FLOATS 0

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

				if(type.VectorWidth > 1) {
					// 'val' refers to an archsim vector
					IRType vtype = type.GetElementType();
					vtype.Signed = false;
					constant_str << "llvm::ConstantDataVector::get(ctx.LLVMCtx, std::vector< " << vtype.GetCType() << ">({";

					for(int i = 0; i < type.VectorWidth; ++i) {
						if(i) {
							constant_str << ", ";
						}
						constant_str << val << ".ExtractElement(" << i << ")";
					}

					constant_str << "}))";
				} else {
					switch (type.BaseType.PlainOldDataType) {
						case IRPlainOldDataType::INT1:
						case IRPlainOldDataType::INT8:
						case IRPlainOldDataType::INT16:
						case IRPlainOldDataType::INT32:
						case IRPlainOldDataType::INT64:
						case IRPlainOldDataType::INT128:
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
				}

				return constant_str.str();
			}

			std::string GetConstantBits(const IRConstant &constant)
			{
				GASSERT(constant.Type() != IRConstant::Type_Vector);

				switch(constant.Type()) {
					case IRConstant::Type_Integer:
						return std::to_string(constant.Int()) + "ULL";
					case IRConstant::Type_Float_Single:
						return "bitcast_u32_float(" + std::to_string(constant.FltBits()) + "ULL)";
					case IRConstant::Type_Float_Double:
						return "bitcast_u64_double(" + std::to_string(constant.DblBits()) + "ULL)";
					default:
						UNIMPLEMENTED;
				}
			}

			std::string GetLLVMValue(const IRType &type, const IRConstant &constant)
			{
				if(type.VectorWidth > 1) {
					std::stringstream str;
					IRType vtype = type.GetElementType();
					vtype.Signed = false;
					str << "llvm::ConstantDataVector::get(ctx.LLVMCtx, std::vector< " << vtype.GetCType() << ">({";
					for(int i = 0; i < constant.GetVector().Width(); ++i) {
						if(i > 0) {
							str << ", ";
						}
						str << GetConstantBits(constant.GetVector().GetElement(i));
					}
					str << "}))";
					return str.str();
				} else {
					if(type.IsFloating()) {
						return "llvm::ConstantFP::get(" + type.GetLLVMType() + ", " + GetConstantBits(constant) + ")";
					} else {
						return "llvm::ConstantInt::get(" + type.GetLLVMType() + ", " + GetConstantBits(constant) + ")";
					}
				}
			}

			void CreateBlock(const SSABlock &block, util::cppformatstream &output)
			{
//				output << "UNIMPLEMENTED;";
				output << "QueueDynamicBlock(__irBuilder, ctx, dynamic_blocks, dynamic_block_queue, __block_" << block.GetName() << ");";
			}

			void JumpOrInlineBlock(SSAWalkerFactory &factory, const SSABlock &Target, util::cppformatstream &output, std::string end_label, bool fully_fixed, bool dynamic)
			{
				// if the target is never_const, we need to put it on the dynamic block queue list and emit an llvm branch
				if (Target.IsFixed() != BLOCK_ALWAYS_CONST) {
					CreateBlock(Target, output);

					output << "__irBuilder.CreateBr(dynamic_blocks[__block_" << Target.GetName() << "]);";
					output << "ctx.ResetLiveRegisters(__irBuilder);";
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

					if(Statement.Type == BinaryOperator::SignedShiftRight) {
						auto signed_type = Statement.LHS()->GetType();
						signed_type.Signed = true;
						output << "(" << signed_type.GetCType() << ")" << Factory.GetOrCreate(Statement.LHS())->GetFixedValue() << " /* signed */ >> " << Factory.GetOrCreate(Statement.RHS())->GetFixedValue() << ");";
						return true;
					}
					if(Statement.Type == BinaryOperator::RotateRight) {
						output << "__ror" << Statement.LHS()->GetType().SizeInBytes()*8ULL << "(" << Factory.GetOrCreate(Statement.LHS())->GetFixedValue() << ", " << Factory.GetOrCreate(Statement.RHS())->GetFixedValue() << "));";
						return true;
					}
					if(Statement.Type == BinaryOperator::RotateLeft) {
						output << "__rol" << Statement.LHS()->GetType().SizeInBytes()*8ULL << "(" << Factory.GetOrCreate(Statement.LHS())->GetFixedValue() << ", " << Factory.GetOrCreate(Statement.RHS())->GetFixedValue() << "));";
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
//							if(Statement.LHS()->IsFixed()) {
//								output << "if(" << LHSNode.GetFixedValue() << " == 0) " << Statement.GetName() << " = " << RHSNode.GetDynamicValue() << "; else ";
//							}
//							if(Statement.RHS()->IsFixed()) {
//								output << "if(" << RHSNode.GetFixedValue() << " == 0) " << Statement.GetName() << " = " << LHSNode.GetDynamicValue() << "; else ";
//							}
							break;
						default:
							break;
					}

					output << Statement.GetName() << " = ";

					// if doing rotates, need to emit funnel shift intrinsic
					if(Statement.Type == BinaryOperator::RotateLeft || Statement.Type == BinaryOperator::RotateRight) {
						std::string function;
						if(Statement.Type == BinaryOperator::RotateLeft) {
							std::string lhs = "__irBuilder.CreateShl(" + LHSNode.GetDynamicValue() + ", " + RHSNode.GetDynamicValue() + ")";
							std::string mask = GetLLVMValue(LHSNode.Statement.GetType(), IRConstant::Integer((8 * LHSNode.Statement.GetType().SizeInBytes()) - 1));
							std::string masked_count = "__irBuilder.CreateAnd(" + RHSNode.GetDynamicValue() + ", " + mask + ")";
							std::string rhs_shift = "__irBuilder.CreateAnd(__irBuilder.CreateSub(llvm::ConstantInt::get(" + LHSNode.Statement.GetType().GetLLVMType() + ", 0), " + masked_count + "), " + mask + ")";
							std::string rhs = "__irBuilder.CreateLShr(" + LHSNode.GetDynamicValue() + ", " + rhs_shift + ")";

							output << "__irBuilder.CreateOr(" << lhs << ", " << rhs << ");";
						} else {
							std::string lhs = "__irBuilder.CreateLShr(" + LHSNode.GetDynamicValue() + ", " + RHSNode.GetDynamicValue() + ")";
							std::string mask = GetLLVMValue(LHSNode.Statement.GetType(), IRConstant::Integer((8 * LHSNode.Statement.GetType().SizeInBytes()) - 1));
							std::string masked_count = "__irBuilder.CreateAnd(" + RHSNode.GetDynamicValue() + ", " + mask + ")";
							std::string rhs_shift = "__irBuilder.CreateAnd(__irBuilder.CreateSub(llvm::ConstantInt::get(" + LHSNode.Statement.GetType().GetLLVMType() + ", 0), " + masked_count + "), " + mask + ")";
							std::string rhs = "__irBuilder.CreateShl(" + LHSNode.GetDynamicValue() + ", " + rhs_shift + ")";

							output << "__irBuilder.CreateOr(" << lhs << ", " << rhs << ");";
						}

						return true;
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
								if(Statement.LHS()->GetType().Signed) {
									output << "llvm::Instruction::SDiv";
								} else {
									output << "llvm::Instruction::UDiv";
								}
								break;
							case BinaryOperator::Modulo:
								if(Statement.LHS()->GetType().Signed) {
									output << "llvm::Instruction::SRem";
								} else {
									output << "llvm::Instruction::URem";
								}
								break;
								break;
							case BinaryOperator::ShiftLeft:
								output << "llvm::Instruction::Shl";
								break;
							case BinaryOperator::ShiftRight:
								output << "llvm::Instruction::LShr";
								break;
							case BinaryOperator::SignedShiftRight:
								output << "llvm::Instruction::AShr";
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

						// if we had a vector, then we need to 'sign extend' the result. otherwise, zero extend
						std::string is_signed = Statement.GetType().VectorWidth > 1 ? "true" : "false";

						output << "llvm::Value * " << Statement.GetName() << " = __irBuilder.CreateIntCast(" << uncast.str() << ", " << Statement.GetType().GetLLVMType() << ", " << is_signed << ");";


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
						case OP_NEGATE:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = !" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ";";
							break;
						case OP_COMPLEMENT:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = ~" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ";";
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
						// -x
						case OP_NEGATIVE:
							// if the inner statement is floating point, then emit fsub -0.0, %stmt
							if((Statement.Expr()->GetType().BaseType.PlainOldDataType == IRPlainOldDataType::FLOAT) || (Statement.Expr()->GetType().BaseType.PlainOldDataType == IRPlainOldDataType::DOUBLE)) {
								output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateFNeg(" << Factory.GetOrCreate(Statement.Expr())->GetDynamicValue() << ");";
							} else {
								output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateNeg(" << Factory.GetOrCreate(Statement.Expr())->GetDynamicValue() << ");";
							}
							break;
						// ~x
						case OP_COMPLEMENT:
							output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateXor(" << Factory.GetOrCreate(Statement.Expr())->GetDynamicValue() << ", llvm::ConstantInt::get(" << Statement.Expr()->GetType().GetLLVMType() << ", -1, true));";
							break;
						// !x
						case OP_NEGATE:
							output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateICmpEQ(" << Factory.GetOrCreate(Statement.Expr())->GetDynamicValue() << ", llvm::ConstantInt::get(" << Statement.Expr()->GetType().GetLLVMType() << ", 0, true));";
							output << Statement.GetName() << " = __irBuilder.CreateZExt(" << Statement.GetName() << ", ctx.Types.i8);";
							break;
						default:
							assert(false);
					}

					return false;
				}

				std::string GetDynamicValue() const override
				{
					const SSAUnaryArithmeticStatement &Statement = static_cast<const SSAUnaryArithmeticStatement &>(this->Statement);
					if(Statement.IsFixed()) {
						// produce llvm constant value based on the produced fixed value
						return GetLLVMValue(Statement.GetType(), Statement.GetName());
					} else {
						return Statement.GetName();
					}
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
							if(Statement.Expr()->GetType().SizeInBytes() != Statement.GetType().SizeInBytes()) {
								UNIMPLEMENTED;
							}
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
						case SSACastStatement::Cast_Convert: {
							auto fromtype = Statement.Expr()->GetType();
							auto totype = Statement.GetType();

							if(fromtype.IsFloating() && totype.IsFloating()) {
								// fp extend or trunc
								if(fromtype.SizeInBytes() > totype.IsFloating()) {
									// fp trunc
									output << "llvm::Instruction::FPTrunc";
								} else {
									output << "llvm::Instruction::FPExtend";
								}

							} else if(fromtype.IsFloating() && !totype.IsFloating()) {
								// fp to int
								if(totype.Signed) {
									output << "llvm::Instruction::FPToSI";
								} else {
									output << "llvm::Instruction::FPToUI";
								}
							} else if(!fromtype.IsFloating() && totype.IsFloating()) {
								// int to fp
								if(fromtype.Signed) {
									output << "llvm::Instruction::SIToFP";
								} else {
									output << "llvm::Instruction::UIToFP";
								}
							} else {
								// int to int?
								UNIMPLEMENTED;
							}

							break;
						}
						default:
							assert(false && "Unrecognized cast type");
					}

					std::string input = Factory.GetOrCreate(Statement.Expr())->GetDynamicValue();

					output << ", " << input << ", " << Statement.GetType().GetLLVMType() << ");";
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
					return GetLLVMValue(Statement.GetType(), Statement.Constant);
				}

				std::string GetFixedValue() const
				{
					const SSAConstantStatement &Statement = static_cast<const SSAConstantStatement &>(this->Statement);
					if(Statement.GetType().IsFloating()) {
						auto constant = Statement.Constant;
						std::stringstream s;
						if(constant.Type() == IRConstant::Type_Float_Single) {
							s << constant.Flt();
						} else if(constant.Type() == IRConstant::Type_Float_Double) {
							s << constant.Dbl();
						}
						return s.str();
					}

					if(Statement.GetType().VectorWidth > 1) {
						std::stringstream str;
						str << "{";
						for(int i = 0; i < Statement.Constant.GetVector().Width(); ++i) {
							if(i > 0) {
								str << ", ";
							}
							str << Statement.Constant.GetVector().GetElement(i).Int();
						}
						str << "}";
						return str.str();
					} else {
						std::stringstream str;
						str << "(" << Statement.GetType().GetCType() << ")" << Statement.Constant.Int() << "ULL";
						return str.str();
					}
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

					std::string callee;
					std::string val_type;
					switch(stmt->Target()->GetType().SizeInBytes()) {
						case 4:
							callee = "dev_read_device";
							val_type = "ctx.Types.i32";
							break;
						case 8:
							callee = "dev_read_device64";
							val_type = "ctx.Types.i64";
							break;
						default:
							UNIMPLEMENTED;
					}

					output << "llvm::Value *ptr = __irBuilder.CreateAlloca(" << val_type << ");";
					output << Statement.GetName() << " = __irBuilder.CreateCall(ctx.Functions." << callee << ", {ctx.GetThreadPtr(__irBuilder), " << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ", ptr}, \"dev_read_result\");";
					output << "ctx.StoreRegister(__irBuilder, __idx_" << stmt->Target()->GetName() << ", __irBuilder.CreateLoad(ptr));";
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

					switch (Statement.GetDescriptor().GetID()) {
						case IntrinsicID::ReadPC: {
							auto pc_desc = Statement.Parent->Parent->Arch->GetRegFile().GetTaggedRegSlot("PC");
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = " << GetLLVMValue(pc_desc->GetIRType(), "phys_pc.Get()") << ";";// EmitRegisterRead(ctx, (void*)&__irBuilder, " << pc_desc->GetWidth() << ", " << pc_desc->GetRegFileOffset() << ");";
							break;
						}
						case IntrinsicID::PopCount32:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __builtin_popcount(" << arg0->GetFixedValue() << ");";
							break;
						case IntrinsicID::CLZ32:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __builtin_clz(" << arg0->GetFixedValue() << ");";
							break;
						case IntrinsicID::GetCpuMode:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = " << Statement.Parent->Parent->GetAction()->Context.ISA.isa_mode_id << ";";
							break;
						case IntrinsicID::GetFeature:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << ";";
							output << "UNIMPLEMENTED;";
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

					output << "//emit intrinsic here " << Statement.GetDescriptor().GetName() << "\n";
					// if (HasValue()) output << GetType().GetCType() << " " << GetName() << " = 0;";
					switch (Statement.GetDescriptor().GetID()) {
						case IntrinsicID::ReadPC: {
							auto pc_desc = Statement.Parent->Parent->Arch->GetRegFile().GetTaggedRegSlot("PC");
							output << "llvm::Value *" << Statement.GetName() << " = " << GetLLVMValue(pc_desc->GetIRType(), "phys_pc.Get()") << ";";// EmitRegisterRead(ctx, (void*)&__irBuilder, " << pc_desc->GetWidth() << ", " << pc_desc->GetRegFileOffset() << ");";
							break;
						}
						case IntrinsicID::WritePC: {
							auto pc_desc = Statement.Parent->Parent->Arch->GetRegFile().GetTaggedRegSlot("PC");

							output << "EmitRegisterWrite(__irBuilder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry(\"PC\"), nullptr, " << arg0->GetDynamicValue() << ");";

							break;
						}
						case IntrinsicID::PopCount32:
							assert(arg0);
							output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(ctx.Functions.ctpop_i32, " << arg0->GetDynamicValue() << ");";
							break;
						case IntrinsicID::CLZ32:
							assert(arg0);

							output <<
							       "llvm::Function *intrinsic = (llvm::Function*)ctx.Functions.clz_i32;"
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(intrinsic, {" << arg0->GetDynamicValue() << ", llvm::ConstantInt::get(ctx.Types.i1, 0, false)});";
							break;
						case IntrinsicID::CLZ64:
							assert(arg0);

							output <<
							       "llvm::Function *intrinsic = (llvm::Function*)ctx.Functions.clz_i64;"
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(intrinsic, {" << arg0->GetDynamicValue() << ", llvm::ConstantInt::get(ctx.Types.i1, 0, false)});";
							break;
						case IntrinsicID::CTZ32:
							assert(arg0);

							output <<
							       "llvm::Function *intrinsic = (llvm::Function*)ctx.Functions.ctz_i32;"
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(intrinsic, {" << arg0->GetDynamicValue() << ", llvm::ConstantInt::get(ctx.Types.i1, 0, false)});";
							break;
						case IntrinsicID::CTZ64:
							assert(arg0);

							output <<
							       "llvm::Function *intrinsic = (llvm::Function*)ctx.Functions.ctz_i64;"
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(intrinsic, {" << arg0->GetDynamicValue() << ", llvm::ConstantInt::get(ctx.Types.i1, 0, false)});";
							break;
						case IntrinsicID::SetCpuMode:
							assert(arg0);
							output << "{";
							output << "llvm::Value *isa_mode_ptr = ctx.GetStateBlockPointer(__irBuilder, \"ModeID\");";
							output << "llvm::Value *value = __irBuilder.CreateZExtOrTrunc(" << arg0->GetDynamicValue() << ", isa_mode_ptr->getType()->getPointerElementType());";
							output << "__irBuilder.CreateStore(value, isa_mode_ptr);";
							output << "}";

							break;
						case IntrinsicID::GetCpuMode:
							output << "llvm::Value *" << Statement.GetName() << " = NULL;";

							output << "{";
							output << "llvm::Value *isa_mode_ptr = ctx.GetStateBlockPointer(__irBuilder, \"ModeID\");";
							output << Statement.GetName() << " = __irBuilder.CreateLoad(isa_mode_ptr);";
							output << "}";
							break;
						case IntrinsicID::TakeException:
							assert(arg0 && arg1);
							output << "EmitTakeException(__irBuilder, ctx, " << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ");";
//							output << "ctx.block.region.EmitTakeException(ctx.tiu.GetOffset(), " << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ");";
							break;
						case IntrinsicID::ProbeDevice:
							assert(arg0);
							output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall2(ctx.Functions.dev_probe_device, ctx.GetThreadPtr(__irBuilder), " << arg0->GetDynamicValue() << ", \"dev_probe_result\");";
							break;
						case IntrinsicID::WriteDevice32:
							assert(arg0 && arg1 && arg2);
							output << "llvm::Value *" << Statement.GetName();
							output << " = __irBuilder.CreateCall(ctx.Functions.dev_write_device, {ctx.GetThreadPtr(__irBuilder), ";
							output << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ", " << arg2->GetDynamicValue() << "}, \"dev_write_result\");";
							break;
						case IntrinsicID::WriteDevice64:
							assert(arg0 && arg1 && arg2);
							output << "llvm::Value *" << Statement.GetName();
							output << " = __irBuilder.CreateCall(ctx.Functions.dev_write_device64, {ctx.GetThreadPtr(__irBuilder), ";
							output << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ", " << arg2->GetDynamicValue() << "}, \"dev_write_result\");";
							break;
						case IntrinsicID::PushInterrupt:
							output << "__irBuilder.CreateCall(ctx.Functions.cpuPushInterrupt, {ctx.GetThreadPtr(__irBuilder), " << GetLLVMValue(IRTypes::UInt32, "0") << "});";
							break;
						case IntrinsicID::PopInterrupt:
							output << "__irBuilder.CreateCall(ctx.Functions.cpu_pop_interrupt, ctx.GetThreadPtr(__irBuilder));";
							break;
						case IntrinsicID::Trap:
							output << "__irBuilder.CreateCall(ctx.Functions.jit_trap, {ctx.GetThreadPtr(__irBuilder)});";
							break;
						case IntrinsicID::PendInterrupt:
							output << "__irBuilder.CreateCall(ctx.Functions.cpuPendIRQ, ctx.GetThreadPtr(__irBuilder));";
							break;
						case IntrinsicID::HaltCpu:
							output << "__irBuilder.CreateCall(ctx.Functions.cpu_halt, ctx.GetThreadPtr(__irBuilder));";
							break;
						case IntrinsicID::SetExecutionRing:
							output << "__irBuilder.CreateCall(ctx.Functions.cpuSetRing, {ctx.GetThreadPtr(__irBuilder), " << arg0->GetDynamicValue() << "});";
							break;
						case IntrinsicID::EnterKernelMode:
							output << "__irBuilder.CreateCall(ctx.Functions.cpuEnterKernel, ctx.GetThreadPtr(__irBuilder));";
							break;
						case IntrinsicID::EnterUserMode:
							output << "__irBuilder.CreateCall(ctx.Functions.cpuEnterUser, ctx.GetThreadPtr(__irBuilder));";
							break;

						case IntrinsicID::FloatIsQNaN:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = NULL;"
							       "{"
							       "	llvm::Value *val = __irBuilder.CreateBitCast(" << arg0->GetDynamicValue() << ", ctx.Types.i32);"
							       "	llvm::Value *exp = __irBuilder.CreateAnd(val, 0x7fc00000);"
							       "	val = __irBuilder.CreateICmpEQ(exp, " << GetLLVMValue(IRTypes::UInt32, "0x7fc00000") << ");"
							       "	" << Statement.GetName() << " = __irBuilder.CreateZExtOrBitCast(val, ctx.Types.i8);"
							       "}";
							break;
						case IntrinsicID::FloatIsSNaN:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = NULL;"
							       "{"
							       "	llvm::Value *val = __irBuilder.CreateBitCast(" << arg0->GetDynamicValue() << ", ctx.Types.i32);"
							       "	llvm::Value *exp = __irBuilder.CreateAnd(val, 0x7fc00000);"
							       "	val = __irBuilder.CreateICmpEQ(exp, " << GetLLVMValue(IRTypes::UInt32, "0x7f800000") << ");"
							       "	" << Statement.GetName() << " = __irBuilder.CreateZExtOrBitCast(val, ctx.Types.i8);"
							       "}";
							break;

						case IntrinsicID::DoubleIsQNaN:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = NULL;"
							       "{"
							       "	llvm::Value *val = __irBuilder.CreateBitCast(" << arg0->GetDynamicValue() << ", ctx.Types.i64);"
							       "	llvm::Value *exp = __irBuilder.CreateAnd(val, 0x7ffc000000000000ULL);"
							       "	val = __irBuilder.CreateICmpEQ(exp, " << GetLLVMValue(IRTypes::UInt64, "0x7ffc000000000000") << ");"
							       "	" << Statement.GetName() << " = __irBuilder.CreateZExtOrBitCast(val, ctx.Types.i8);"
							       "}";
							break;
						case IntrinsicID::DoubleIsSNaN:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = NULL;"
							       "{"
							       "	llvm::Value *val = __irBuilder.CreateBitCast(" << arg0->GetDynamicValue() << ", ctx.Types.i64);"
							       "	llvm::Value *exp = __irBuilder.CreateAnd(val, 0x7ffc000000000000);"
							       "	val = __irBuilder.CreateICmpEQ(exp, " << GetLLVMValue(IRTypes::UInt64, "0x7ff8000000000000") << ");"
							       "	" << Statement.GetName() << " = __irBuilder.CreateZExtOrBitCast(val, ctx.Types.i8);"
							       "}";
							break;

						case IntrinsicID::DoubleSqrt:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(ctx.Functions.double_sqrt, " << arg0->GetDynamicValue() << ");";
							break;
						case IntrinsicID::FloatSqrt:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(ctx.Functions.float_sqrt, " << arg0->GetDynamicValue() << ");";
							break;

						case IntrinsicID::DoubleAbs:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(ctx.Functions.double_abs, " << arg0->GetDynamicValue() << ");";
							break;
						case IntrinsicID::FloatAbs:
							assert(arg0);
							output <<
							       "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(ctx.Functions.float_abs, " << arg0->GetDynamicValue() << ");";
							break;

						case IntrinsicID::UpdateZNFlags32: {
							output << "{"
							       "llvm::Value *n = __irBuilder.CreateLShr(" << arg0->GetDynamicValue() << ", 31);"
							       "llvm::Value *z = __irBuilder.CreateICmpEQ(" << arg0->GetDynamicValue() << ", llvm::ConstantInt::get(ctx.Types.i32, 0, false));"
							       "EmitRegisterWrite(__irBuilder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry(\"Z\"), 0, z);"
							       "EmitRegisterWrite(__irBuilder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry(\"N\"), 0, n);"
							       "}";
							break;
						}


						case IntrinsicID::ADC8_Flags:
						case IntrinsicID::ADC16_Flags:
						case IntrinsicID::ADC32_Flags:
						case IntrinsicID::ADC64_Flags:
							output << "EmitAdcWithFlags(__irBuilder, ctx, ";
							switch(Statement.GetDescriptor().GetID()) {
								case IntrinsicID::ADC8_Flags:
									output << "8";
									break;
								case IntrinsicID::ADC16_Flags:
									output << "16";
									break;
								case IntrinsicID::ADC32_Flags:
									output << "32";
									break;
								case IntrinsicID::ADC64_Flags:
									output << "64";
									break;
								default:
									UNEXPECTED;
							}
							output << ", " << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ", " << arg2->GetDynamicValue() << ");";
							break;

						case IntrinsicID::SBC8_Flags:
						case IntrinsicID::SBC16_Flags:
						case IntrinsicID::SBC32_Flags:
						case IntrinsicID::SBC64_Flags:
							output << "EmitSbcWithFlags(__irBuilder, ctx, ";
							switch(Statement.GetDescriptor().GetID()) {
								case IntrinsicID::SBC8_Flags:
									output << "8";
									break;
								case IntrinsicID::SBC16_Flags:
									output << "16";
									break;
								case IntrinsicID::SBC32_Flags:
									output << "32";
									break;
								case IntrinsicID::SBC64_Flags:
									output << "64";
									break;
								default:
									UNEXPECTED;
							}
							output << ", " << arg0->GetDynamicValue() << ", " << arg1->GetDynamicValue() << ", " << arg2->GetDynamicValue() << ");";
							break;

						case IntrinsicID::FPSetRounding:
						case IntrinsicID::FPGetRounding:
						case IntrinsicID::FPSetFlush:
						case IntrinsicID::FPGetFlush:
							output << "llvm::Value *" << Statement.GetName() << " = nullptr;";
							output << "UNIMPLEMENTED;";
							break;

						case IntrinsicID::BSwap32:
							output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(ctx.Functions.bswap_i32, " << arg0->GetDynamicValue() << ");";
							break;
						case IntrinsicID::BSwap64:
							output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateCall(ctx.Functions.bswap_i64, " << arg0->GetDynamicValue() << ");";
							break;


						case IntrinsicID::MemLock:
						case IntrinsicID::MemUnlock:
							// TODO: just do nothing for now
							break;

						case IntrinsicID::SetFeature:
						case IntrinsicID::GetFeature:
						case IntrinsicID::UpdateZNFlags64:
							output << "llvm::Value *" << Statement.GetName() << " = nullptr;";
							output << "UNIMPLEMENTED;";
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

					switch (Statement.GetDescriptor().GetID()) {
						case IntrinsicID::CLZ32:
						case IntrinsicID::CLZ64:
						case IntrinsicID::CTZ32:
						case IntrinsicID::CTZ64:
						case IntrinsicID::PopCount32:
						case IntrinsicID::GetCpuMode:
						case IntrinsicID::ProbeDevice:
						case IntrinsicID::ReadPC:
						case IntrinsicID::WriteDevice32:
						case IntrinsicID::WriteDevice64:

						case IntrinsicID::FloatIsSNaN:
						case IntrinsicID::FloatIsQNaN:
						case IntrinsicID::DoubleIsSNaN:
						case IntrinsicID::DoubleIsQNaN:
						case IntrinsicID::DoubleSqrt:
						case IntrinsicID::FloatSqrt:
						case IntrinsicID::DoubleAbs:
						case IntrinsicID::FloatAbs:

						case IntrinsicID::ADC32_Flags:

						case IntrinsicID::FPGetRounding:
						case IntrinsicID::FPGetFlush:

						case IntrinsicID::BSwap32:
						case IntrinsicID::BSwap64:

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
					output << "ctx.ResetLiveRegisters(__irBuilder);";
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
					output << "ctx.ResetLiveRegisters(__irBuilder);";
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
					output << "llvm::Value *" << Statement.GetName() << " = nullptr;";
					output << "{";
					output << "llvm::Value *data = EmitMemoryRead(__irBuilder, ctx, " << Statement.GetInterface()->GetID() << ", " << (uint32_t)Statement.Width << ", " << AddrExpr->GetDynamicValue() << ");";
					output << "ctx.StoreRegister(__irBuilder, __idx_" << Statement.Target()->GetName() << ", data);";
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

					output << "EmitMemoryWrite(__irBuilder, ctx, " << Statement.GetInterface()->GetID() << ", " << (uint32_t)Statement.Width << ", " << AddrExpr->GetDynamicValue() << ", " << ValueExpr->GetDynamicValue() << ");";

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

					str << Statement.Target()->GetName();

					// slight hack to fix getispredicated
					if(Statement.MemberNames.size() == 1 && Statement.MemberNames.at(0) == "IsPredicated") {
						str << ".GetIsPredicated()";

					} else {
						for(auto i : Statement.MemberNames) {
							str << "." << i;
						}
					}

					return str.str();
				}

				std::string GetDynamicValue() const
				{
					const SSAReadStructMemberStatement &Statement = static_cast<const SSAReadStructMemberStatement &>(this->Statement);

					return GetLLVMValue(Statement.GetType(), GetFixedValue());
				}
			};

			class SSARegisterStatementWalker : public SSANodeWalker
			{
			public:
				SSARegisterStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return EmitDynamicCode(output, end_label, fully_fixed);

//					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSARegisterStatement &Statement = static_cast<const SSARegisterStatement &>(this->Statement);
					const auto &Arch = Statement.Parent->Parent->GetAction()->Context.Arch;

					SSANodeWalker *ValueExpr = NULL;
					if (Statement.Value()) ValueExpr = Factory.GetOrCreate(Statement.Value());

					SSANodeWalker *RegnumExpr = NULL;
					if (Statement.RegNum()) RegnumExpr = Factory.GetOrCreate(Statement.RegNum());

					std::string reg_name, index;
					int size = 0;
					if(Statement.IsBanked) {
						auto &descriptor = Arch.GetRegFile().GetBankByIdx(Statement.Bank);

						reg_name = "\"" + descriptor.ID + "\"";
						index = RegnumExpr->GetDynamicValue();
						size = descriptor.GetRegisterIRType().SizeInBytes();

					} else {
						auto &descriptor = Arch.GetRegFile().GetSlotByIdx(Statement.Bank);

						reg_name = "\"" + descriptor.GetID() + "\"";
						index = "nullptr";
						size = descriptor.GetIRType().SizeInBytes();
					}

					if (!Statement.IsRead) {

						if(Statement.IsBanked) {
							output << "if(archsim::options::Trace) {";
							output << "EmitTraceBankedRegisterWrite(__irBuilder, ctx, " << (uint32_t)Statement.Bank << ", " << RegnumExpr->GetDynamicValue() << ", " << size << ", " << ValueExpr->GetDynamicValue() << ");";
							output << "}";
						} else {
							output << "EmitTraceRegisterWrite(__irBuilder, ctx, " << (uint32_t)Statement.Bank << ", " << ValueExpr->GetDynamicValue() << ");";
						}

						output << "EmitRegisterWrite(__irBuilder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetEntry(" << reg_name << "), " << index << ", " << ValueExpr->GetDynamicValue() << ");";

					} else {
						auto &value_type = Statement.GetType();

						output << "llvm::Value *" << Statement.GetName() << " = EmitRegisterRead(__irBuilder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetEntry(" << reg_name << "), " << index << ");";

						if(Statement.IsBanked) {
							output << "if(archsim::options::Trace) {";
							output << "EmitTraceBankedRegisterRead(__irBuilder, ctx, " << (uint32_t)Statement.Bank << ", " << RegnumExpr->GetDynamicValue() << ", " << value_type.SizeInBytes() << ", " << Statement.GetName() << ");";
							output << "}";

						} else {
							output << "EmitTraceRegisterRead(__irBuilder, ctx, " << (uint32_t)Statement.Bank << ", " << Statement.GetName() << ");";
						}

						output << Statement.GetName() << " = __irBuilder.CreateBitCast(" << Statement.GetName() << ", " << Statement.GetType().GetLLVMType() << ");";
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
						output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateSelect(__irBuilder.CreateICmpNE(" << Cond->GetDynamicValue() << ", llvm::ConstantInt::get(" << Statement.Cond()->GetType().GetLLVMType() << ", 0)), " << True->GetDynamicValue() << ", " << False->GetDynamicValue() << ");";
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
					for(auto v_i : Statement.GetValues()) {
						SSAStatement *v = v_i.first;
						SSABlock *b = v_i.second;
						SSAConstantStatement *c = dynamic_cast<SSAConstantStatement *>(v);
						assert(c);
						CreateBlock(*b, output);
						std::stringstream constantStream;
						constantStream << c->Constant.Int();
						output << "sw->addCase((llvm::ConstantInt*)" << GetLLVMValue(Statement.Expr()->GetType(), constantStream.str()) << ", dynamic_blocks[__block_" << b->GetName() << "]);";
					}
					output << "}";
					output << "ctx.ResetLiveRegisters(__irBuilder);";
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

					output << "llvm::Value *" << Statement.GetName() << " = ctx.LoadRegister(__irBuilder, __idx_" << Statement.Target()->GetName() << ");";
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

					if(Statement.GetType().VectorWidth > 1) {
						if(Statement.IsFixed()) {
							// we need to build the LLVM vector which represents it
							std::string str;

							// start with 'undef' value of correct type
							str = Statement.GetType().GetLLVMType();
							str = "llvm::UndefValue::get(" + str + ")";

							// successively insert elements into vector
							for(int i = 0; i < Statement.GetType().VectorWidth; ++i) {
								str = "__irBuilder.CreateInsertElement(" + str + ", llvm::ConstantInt::get(" + Statement.GetType().GetElementType().GetLLVMType() + ", 0), (uint64_t)" + std::to_string(i) + ")";
							}
							return str;
						} else {
							return Statement.GetName();
						}

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
						output << "ctx.StoreRegister(__irBuilder, __idx_" << Statement.Target()->GetName() << ", " << GetLLVMValue(Statement.Target()->GetType(), "CV_" + Statement.Target()->GetName()) << ");";
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableWriteStatement &Statement = static_cast<const SSAVariableWriteStatement &>(this->Statement);
					SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());
					output << "ctx.StoreRegister(__irBuilder, __idx_" << Statement.Target()->GetName() << ", " << expr->GetDynamicValue() << ");";

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

					if(Statement.HasValue()) output << "::llvm::Value *" << Statement.GetName() << ";";
					output << "{";
					//first, figure out the type of the function we're calling (+1 because of CPU context - thiscall)
					uint32_t num_params = Statement.Target()->GetPrototype().GetIRSignature().GetParams().size() + 1;

					//Figure out the return type of the function
					std::string return_type = "ctx.Types.vtype";
					if(Statement.HasValue()) return_type = Statement.GetType().GetLLVMType();

					//Build a parameter type list to get the correct function type
					output << "llvm::Type *params[] { ctx.GetThreadPtr(__irBuilder)->getType() ";
					for(auto param : Statement.Target()->GetPrototype().GetIRSignature().GetParams()) {
						assert(!param.GetType().Reference && "Cannot make function calls with references yet!");
						output << ", " << param.GetType().GetLLVMType();
					}
					output << "};";

					output << "llvm::FunctionType *callee_type = llvm::FunctionType::get(" << return_type << ", llvm::ArrayRef<llvm::Type*>(params, " << num_params << "), false);";

					//get the function pointer to cast
					output << "void *naked_ptr = (void*)txln_shunt_" << Statement.Target()->GetPrototype().GetIRSignature().GetName() << ";";
					output << "llvm::Value *llvm_fn_ptr = __irBuilder.CreateIntToPtr(llvm::ConstantInt::get(ctx.Types.i64, (intptr_t)naked_ptr), callee_type->getPointerTo());";

					//build the args list
					output << "llvm::Value *args[] { ctx.GetThreadPtr(__irBuilder) ";
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
					const SSAVectorInsertElementStatement *stmt = dynamic_cast<const SSAVectorInsertElementStatement*>(&Statement);
					const SSAStatement *base = stmt->Base();
					const SSAStatement *index = stmt->Index();
					const SSAStatement *value = stmt->Value();

					output << stmt->GetType().GetCType() << " " << stmt->GetName() << " = " << Factory.GetOrCreate(base)->GetFixedValue() << ";";
					output << stmt->GetName() << ".InsertElement(" << Factory.GetOrCreate(index)->GetFixedValue() << ", " << Factory.GetOrCreate(value)->GetFixedValue() << ");";

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

			class SSAVectorShuffleStatementWalker : public SSANodeWalker
			{
			public:
				SSAVectorShuffleStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact) {}

				bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					const SSAVectorShuffleStatement *stmt = dynamic_cast<const SSAVectorShuffleStatement*>(&Statement);
					output << "llvm::Value *" << Statement.GetName() << " = __irBuilder.CreateShuffleVector(" <<
					       Factory.GetOrCreate(stmt->LHS())->GetDynamicValue() << ", " <<
					       Factory.GetOrCreate(stmt->RHS())->GetDynamicValue() << ", " <<
					       Factory.GetOrCreate(stmt->Indices())->GetDynamicValue() <<
					       ");";

					return true;
				}

				bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << ";";
					output << "UNIMPLEMENTED;";
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
		HANDLE(SSAVectorShuffleStatement);
		HANDLE(SSACallStatement);
		assert(false && "Unrecognized statement type");
		return NULL;
	}
#undef STMT_IS
}
