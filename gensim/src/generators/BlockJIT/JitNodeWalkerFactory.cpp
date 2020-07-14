/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <typeinfo>

#include "generators/BlockJIT/JitGenerator.h"

#include "genC/Parser.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IR.h"
#include "genC/ssa/SSAWalkerFactory.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAFormAction.h"
#include "isa/ISADescription.h"

using namespace gensim;
using namespace gensim::generator;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

namespace gensim
{
	namespace generator
	{
		namespace jit
		{

			void CreateBlock(util::cppformatstream &output, std::string block_id_name)
			{
				output << "IRBlockId block = " << block_id_name << ";\n";

				output << "dynamic_block_queue.push(" << block_id_name << ");\n";

				//output << "if(dynamic_blocks.count(" << block_id_name << ")) block = dynamic_blocks[" << block_id_name << "];";
				//output << "else { block = lir.create_block(); dynamic_blocks[" << block_id_name << "] = block; dynamic_block_queue.push(" << block_id_name << "); }";

			}

			static std::string operand_for_node(const SSANodeWalker& node)
			{
				if (node.Statement.IsFixed()) {
					auto type = node.Statement.GetType();
					if(type.VectorWidth > 1) {
						int total_size = type.SizeInBytes() * 8;
						return "IROperand::const" + std::to_string(total_size) + "(" + node.GetFixedValue() + ".toData())";
					}

					if (node.Statement.GetType() == IRTypes::UInt8) {
						return "IROperand::const8(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::UInt16) {
						return "IROperand::const16(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::UInt32) {
						return "IROperand::const32(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::UInt64) {
						return "IROperand::const64(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int8) {
						return "IROperand::const8(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int16) {
						return "IROperand::const16(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int32) {
						return "IROperand::const32(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int64) {
						return "IROperand::const64(" + node.GetFixedValue() + ")";
					} else if(node.Statement.GetType() == IRTypes::Float) {
						return "IROperand::const_float(" + node.GetFixedValue() + ")";
					} else if(node.Statement.GetType() == IRTypes::Double) {
						return "IROperand::const_double(" + node.GetFixedValue() + ")";
					} else if(node.Statement.GetType() == IRTypes::Int128) {
						return "IROperand::const128(" + node.GetFixedValue() + ")";
					} else if(node.Statement.GetType() == IRTypes::UInt128) {
						return "IROperand::const128(" + node.GetFixedValue() + ")";
					} else if(node.Statement.GetType() == IRTypes::LongDouble) {
						return "IROperand::const_long_double(" + node.GetFixedValue() + ")";
					} else {
						throw std::logic_error("Unsupported node type: " + node.Statement.GetType().GetCType());
						UNEXPECTED;
					}
				} else {
					return node.GetDynamicValue();
				}
			}

			static std::string operand_for_stmt(const SSAStatement& stmt)
			{
				if (stmt.IsFixed()) {
					switch (stmt.GetType().SizeInBytes()) {
						case 1:
							return "IROperand::const8(CV_" + stmt.GetName() + ")";
						case 2:
							return "IROperand::const16(CV_" + stmt.GetName() + ")";
						case 4:
							return "IROperand::const32(CV_" + stmt.GetName() + ")";
						case 8:
							return "IROperand::const64(CV_" + stmt.GetName() + ")";
						default:
							assert(false && "Unsupported statement size");
							UNEXPECTED;
					}

				} else {
					return "IROperand::vreg(" + stmt.GetName() + ", " + std::to_string(stmt.GetType().SizeInBytes()) + ")";
				}
			}

			static std::string operand_for_symbol(const SSASymbol& sym)
			{
				return "IROperand::vreg(ir_idx_" + sym.GetName() + ", " + std::to_string(sym.GetType().SizeInBytes()) + ")";
			}

			class BlockJitNodeWalker : public genc::ssa::SSANodeWalker
			{
			public:
				BlockJitNodeWalker(const SSAStatement &stmt, SSAWalkerFactory &_fact) : SSANodeWalker(stmt, _fact)
				{
				}

				std::string GetDynamicValue() const override
				{
					std::stringstream str;
					str << "IROperand::vreg(" << Statement.GetName() << ", " << Statement.GetType().SizeInBytes() << ")";
					return str.str();
				}

			};

			class SSAAllocRegisterStatementWalker : public BlockJitNodeWalker
			{
			public:
				SSAAllocRegisterStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}
			};

			class SSABinaryArithmeticStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSABinaryArithmeticStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSABinaryArithmeticStatement &stmt = static_cast<const SSABinaryArithmeticStatement &> (this->Statement);
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ((" << stmt.GetType().GetCType() << ")";

					SSANodeWalker *LHSNode = Factory.GetOrCreate(stmt.LHS());
					SSANodeWalker *RHSNode = Factory.GetOrCreate(stmt.RHS());

					// Modulo needs to handled differently for floating point operands
					if (stmt.Type == BinaryOperator::Modulo && (stmt.LHS()->GetType().IsFloating() || stmt.RHS()->GetType().IsFloating())) {
						output << "fmod(" << LHSNode->GetFixedValue() << ", " << RHSNode->GetFixedValue() << "));";
						return true;
					} else if (stmt.Type == BinaryOperator::RotateRight) {
						output << "(__ror32(" << LHSNode->GetFixedValue() << ", " << RHSNode->GetFixedValue() << ")));";
						return true;
					} else if (stmt.Type == BinaryOperator::RotateLeft) {
						output << "(__rol(" << LHSNode->GetFixedValue() << ", " << RHSNode->GetFixedValue() << ")));";
						return true;
					}

					output << "(" << LHSNode->GetFixedValue();

					switch (stmt.Type) {
						// Shift
						case BinaryOperator::ShiftLeft:
							output << " << ";
							break;
						case BinaryOperator::ShiftRight:
						case BinaryOperator::SignedShiftRight:
							output << " >> ";
							break;

						// Equality
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

						// Arithmetic
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
						case BinaryOperator::Modulo:
							output << " % ";
							break;

						// Logical
						case BinaryOperator::Logical_And:
							output << " && ";
							break;
						case BinaryOperator::Logical_Or:
							output << " || ";
							break;

						// Bitwise
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
							assert(false && "Unrecognised binary operator");
							UNEXPECTED;
					}
					output << RHSNode->GetFixedValue() << "));";
					return true;
				}

				bool EmitDynamicCodeFloat(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSABinaryArithmeticStatement &Statement = static_cast<const SSABinaryArithmeticStatement &> (this->Statement);

					SSANodeWalker *LHSNode = Factory.GetOrCreate(Statement.LHS());
					SSANodeWalker *RHSNode = Factory.GetOrCreate(Statement.RHS());

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";

					switch(Statement.Type) {
						case BinaryOperator::Multiply:
							output << "builder.fmul(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::Divide:
							output << "builder.fdiv(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::Add:
							output << "builder.fadd(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::Subtract:
							output << "builder.fsub(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::LessThan:
							output << "builder.fcmp_lt(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::LessThanEqual:
							output << "builder.fcmp_lte(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::GreaterThan:
							output << "builder.fcmp_gt(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::GreaterThanEqual:
							output << "builder.fcmp_gte(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::Equality:
							output << "builder.fcmp_eq(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case BinaryOperator::Inequality:
							output << "builder.fcmp_ne(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");";
							break;
						default:
							output << "assert(false && \"Unimplemented\");";
					}

					return true;
				}

				bool EmitVectorOp(util::cppformatstream &output, const std::string &op) const
				{
					const SSABinaryArithmeticStatement &Statement = static_cast<const SSABinaryArithmeticStatement &> (this->Statement);

					SSANodeWalker *LHSNode = Factory.GetOrCreate(Statement.LHS());
					SSANodeWalker *RHSNode = Factory.GetOrCreate(Statement.RHS());

					const SSAStatement *LHS = Statement.LHS();
					const SSAStatement *RHS = Statement.RHS();

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
					if(LHS->GetType().IsFloating()) {
						output << "builder.v" << op << "f(IROperand::const32(" << (uint32_t)LHS->GetType().VectorWidth << "), " << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_node(*this) << ");";
					} else {
						output << "builder.v" << op << "i(IROperand::const32(" << (uint32_t)LHS->GetType().VectorWidth << "), " << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_node(*this) << ");";
					}

					return true;
				}

				bool EmitDynamicCodeVector(util::cppformatstream &output) const
				{
					const SSABinaryArithmeticStatement &Statement = static_cast<const SSABinaryArithmeticStatement &> (this->Statement);

					SSANodeWalker *LHSNode = Factory.GetOrCreate(Statement.LHS());
					SSANodeWalker *RHSNode = Factory.GetOrCreate(Statement.RHS());

					const SSAStatement *LHS = Statement.LHS();
					const SSAStatement *RHS = Statement.RHS();

					assert(LHS->GetType().IsFloating() == RHS->GetType().IsFloating());
					assert(LHS->GetType().VectorWidth == RHS->GetType().VectorWidth);
					assert(LHS->GetType().VectorWidth > 1);

					switch(Statement.Type) {
						case BinaryOperator::Add:
							return EmitVectorOp(output, "add");
						case BinaryOperator::Subtract:
							return EmitVectorOp(output, "sub");
						case BinaryOperator::Multiply:
							return EmitVectorOp(output, "mul");
						case BinaryOperator::LessThan:
							return EmitVectorOp(output, "cmp_lt");
						case BinaryOperator::GreaterThan:
							return EmitVectorOp(output, "cmpgt");
						case BinaryOperator::GreaterThanEqual:
							return EmitVectorOp(output, "cmpgte");
						case BinaryOperator::Equality:
							return EmitVectorOp(output, "cmpeq");

						case BinaryOperator::Bitwise_Or:
							return EmitVectorOp(output, "or");
						case BinaryOperator::Bitwise_And:
							return EmitVectorOp(output, "and");
						case BinaryOperator::Bitwise_XOR:
							return EmitVectorOp(output, "xor");
							break;
						default:
							throw std::logic_error("Unimplemented vector operator " + BinaryOperator::PrettyPrintOperator(Statement.Type));
							UNEXPECTED;

					}
					return false;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSABinaryArithmeticStatement &Statement = static_cast<const SSABinaryArithmeticStatement &> (this->Statement);

					SSANodeWalker *LHSNode = Factory.GetOrCreate(Statement.LHS());
					SSANodeWalker *RHSNode = Factory.GetOrCreate(Statement.RHS());

					if(LHSNode->Statement.GetType().IsFloating()) assert(RHSNode->Statement.GetType().IsFloating());
					if(!LHSNode->Statement.GetType().IsFloating()) assert(!RHSNode->Statement.GetType().IsFloating());

					if(LHSNode->Statement.GetType().VectorWidth > 1) {
						return EmitDynamicCodeVector(output);
					}

					if(LHSNode->Statement.GetType().IsFloating()) {
						if(LHSNode->Statement.GetType() == IRTypes::Float) return EmitDynamicCodeFloat(output, end_label, fully_fixed);
						if(LHSNode->Statement.GetType() == IRTypes::Double) return EmitDynamicCodeFloat(output, end_label, fully_fixed);
					}

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";

					switch (Statement.Type) {
						// Shift
						case BinaryOperator::ShiftLeft:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.shl(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::SignedShiftRight:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.sar(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::ShiftRight:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.shr(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::RotateRight:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.ror(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::RotateLeft:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.rol(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;

						// Equality
						case BinaryOperator::Equality:
							output << "{"
							       "builder.cmpeq(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n"
							       "}";
							break;
						case BinaryOperator::Inequality:
							output << "{"
							       "builder.cmpne(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n"
							       "}";
							break;

						case BinaryOperator::LessThan:
						case BinaryOperator::LessThanEqual:
						case BinaryOperator::GreaterThan:
						case BinaryOperator::GreaterThanEqual: {
							std::string comparison = "";
							if(LHSNode->Statement.GetType().Signed) {
								switch(Statement.Type) {
									case BinaryOperator::LessThan:
										comparison = "cmpslt";
										break;
									case BinaryOperator::LessThanEqual:
										comparison = "cmpslte";
										break;
									case BinaryOperator::GreaterThan:
										comparison = "cmpsgt";
										break;
									case BinaryOperator::GreaterThanEqual:
										comparison = "cmpsgte";
										break;

									default:
										UNEXPECTED;
								}
							} else {
								switch(Statement.Type) {
									case BinaryOperator::LessThan:
										comparison = "cmplt";
										break;
									case BinaryOperator::LessThanEqual:
										comparison = "cmplte";
										break;
									case BinaryOperator::GreaterThan:
										comparison = "cmpgt";
										break;
									case BinaryOperator::GreaterThanEqual:
										comparison = "cmpgte";
										break;

									default:
										UNEXPECTED;
								}
							}

							output << "{"
							       "builder." << comparison << "(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n"
							       "}";
							break;
						}
						// Arithmetic
						case BinaryOperator::Add:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.add(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::Subtract:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.sub(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::Multiply:

							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							if(LHSNode->Statement.GetType().Signed || RHSNode->Statement.GetType().Signed) {
								output << "builder.imul(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							} else {
								output << "builder.mul(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							}
							break;
						case BinaryOperator::Divide:

							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							if(Statement.LHS()->GetType().Signed)
								output << "builder.sdiv(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							else
								output << "builder.udiv(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::Modulo:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.mod(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;

						// Logical
						case BinaryOperator::Logical_And:
						case BinaryOperator::Logical_Or:
							assert(false && "Unexpected instruction (we should have got rid of these during optimisation!)");
							break;

						// Bitwise
						case BinaryOperator::Bitwise_XOR:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.bitwise_xor(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::Bitwise_And:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.bitwise_and(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;
						case BinaryOperator::Bitwise_Or:
							output << "builder.mov(" << operand_for_node(*LHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							output << "builder.bitwise_or(" << operand_for_node(*RHSNode) << ", " << operand_for_stmt(Statement) << ");\n";
							break;

						default:
							assert(false && "Unrecognised binary operator");
					}

					return true;
				}
			};

			class SSAConstantStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAConstantStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = " << GetFixedValue() << ";";
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAConstantStatement& stmt = (const SSAConstantStatement&) (this->Statement);
					if(stmt.GetType().IsFloating()) {
						UNIMPLEMENTED;
					}

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");";
					output << "builder.mov(IROperand::const32(" << stmt.Constant.Int() << "), " << operand_for_stmt(stmt) << ");";
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAConstantStatement &stmt = static_cast<const SSAConstantStatement &> (this->Statement);
					std::stringstream str;
					if(stmt.GetType().IsFloating()) {
						switch(stmt.Constant.Type()) {
							case genc::IRConstant::Type_Float_Single: {
								// bitcast the floating point constant so that it can be precisely encoded
								float f = stmt.Constant.Flt();
								uint32_t bf = *(uint32_t*)(&f);
								// now, emit code to bitcast it back
								str << "[]{uint32_t bf = " << bf << "; return *(float*)&bf;}()";
								break;
							}
							case genc::IRConstant::Type_Float_Double: {
								// bitcast the floating point constant so that it can be precisely encoded
								double f = stmt.Constant.Dbl();
								uint64_t bf = *(uint64_t*)(&f);
								// now, emit code to bitcast it back
								str << "[]{uint64_t bf = " << bf << "; return *(double*)&bf;}()";
								break;
							}
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
									str << std::to_string(stmt.Constant.GetVector().GetElement(i).Int());
								}
								str << "})";
								break;
							default:
								UNIMPLEMENTED;
						}
					}
					return str.str();
				}

				std::string GetDynamicValue() const
				{
					const SSAConstantStatement &stmt = static_cast<const SSAConstantStatement &> (this->Statement);
					const auto stmttype = stmt.GetType();
					if(stmttype.IsFloating()) {
						if(stmttype.SizeInBytes() == 4) {
							return "IROperand::const_float(" + std::to_string(stmt.Constant.Flt()) + ")";
						} else if(stmttype.SizeInBytes() == 8) {
							return "IROperand::const_double(" + std::to_string(stmt.Constant.Dbl()) + ")";
						} else {
							UNREACHABLE;
						}
					} else {
						return "IROperand::const" + std::to_string(Statement.GetType().SizeInBytes()*8) + "(" + std::to_string(stmt.Constant.Int()) + "ull)";
					}
				}
			};

			class SSACastStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSACastStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}

				bool EmitCastToFloat(util::cppformatstream &output) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement&> (this->Statement);

					const SSAStatement *from = Statement.Expr();
					const SSAStatement *to = &Statement;

					SSANodeWalker *fromnode = Factory.GetOrCreate(from);

					assert(!from->GetType().IsFloating());
					assert(to->GetType().IsFloating());

					if(from->GetType().Signed)
						output << "builder.fcvt_si_to_float(" << operand_for_node(*fromnode) << ", " << operand_for_stmt(*to) << ");";
					else
						output << "builder.fcvt_ui_to_float(" << operand_for_node(*fromnode) << ", " << operand_for_stmt(*to) << ");";

					return true;
				}

				bool EmitCastFromFloat(util::cppformatstream &output) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement&> (this->Statement);

					const SSAStatement *from = Statement.Expr();
					const SSAStatement *to = &Statement;

					SSANodeWalker *fromnode = Factory.GetOrCreate(from);

					assert(from->GetType().IsFloating());
					assert(!to->GetType().IsFloating());

					if(to->GetType().Signed) {
						switch(Statement.GetOption()) {
							case SSACastStatement::Option_RoundDefault:
								output << "builder.fcvt_float_to_si(" << operand_for_node(*fromnode) << ", " << operand_for_stmt(*to) << ");";
								break;
							case SSACastStatement::Option_RoundTowardZero:
								output << "builder.fcvtt_float_to_si(" << operand_for_node(*fromnode) << ", " << operand_for_stmt(*to) << ");";
								break;
							default:
								UNIMPLEMENTED;
						}
					} else {
						switch(Statement.GetOption()) {
							case SSACastStatement::Option_RoundDefault:
								output << "builder.fcvt_float_to_ui(" << operand_for_node(*fromnode) << ", " << operand_for_stmt(*to) << ");";
								break;
							case SSACastStatement::Option_RoundTowardZero:
								output << "builder.fcvtt_float_to_ui(" << operand_for_node(*fromnode) << ", " << operand_for_stmt(*to) << ");";
								break;
							default:
								UNIMPLEMENTED;
						}
					}

					return true;
				}

				bool EmitCastBetweenFloat(util::cppformatstream &output) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement&> (this->Statement);

					const SSAStatement *from = Statement.Expr();
					const SSAStatement *to = &Statement;

					SSANodeWalker *fromnode = Factory.GetOrCreate(from);

					assert(from->GetType().IsFloating());
					assert(to->GetType().IsFloating());

					// at the moment we only support two types of float cast:
					// 1. float to double
					// 2. double to float

					if(from->GetType().SizeInBytes() == 4 && to->GetType().SizeInBytes() == 8) {
						output << "builder.fcvt_single_to_double(" << operand_for_node(*fromnode) << ", " << operand_for_stmt(*to) << ");";
					} else if(from->GetType().SizeInBytes() == 8 && to->GetType().SizeInBytes() == 4) {
						output << "builder.fcvt_double_to_single(" << operand_for_node(*fromnode) << ", " << operand_for_stmt(*to) << ");";
					} else {
						assert(false && "Unsupported float cast!");
					}

					return true;
				}

				bool EmitFloatCastCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement&> (this->Statement);

					// Three different conditions here:
					// 1. Cast TO floating point
					// 2. Cast FROM floating point
					// 3. Cast BETWEEN floating point

					if(Statement.GetType().IsFloating() && !Statement.Expr()->GetType().IsFloating()) {
						return EmitCastToFloat(output);
					} else if(!Statement.GetType().IsFloating() && Statement.Expr()->GetType().IsFloating()) {
						return EmitCastFromFloat(output);
					} else if(Statement.GetType().SizeInBytes() != Statement.Expr()->GetType().SizeInBytes()) {
						return EmitCastBetweenFloat(output);
					}

					throw std::logic_error("Unknown cast type");
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement&> (this->Statement);

					SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");";

					if(Statement.GetCastType() != SSACastStatement::Cast_Reinterpret) {
						if(Statement.GetType() == Statement.Expr()->GetType()) {
							// actually just a mov
							output << "builder.mov(" << operand_for_node(*expr) << ", " << operand_for_stmt(Statement) << ");";
							return true;
						}

						if(Statement.GetType().IsFloating() || Statement.Expr()->GetType().IsFloating()) {
							return EmitFloatCastCode(output, end_label, fully_fixed);
						}

						//If we're truncating, we can do an and (theoretically we can do just a mov but we don't correctly size operands yet)
						if (Statement.GetType().SizeInBytes() < Statement.Expr()->GetType().SizeInBytes()) {
							output << "builder.trunc(" << operand_for_node(*expr) << ", " << operand_for_stmt(Statement) << ");";
						} else if (Statement.GetType().SizeInBytes() == Statement.Expr()->GetType().SizeInBytes()) {
							output << "builder.mov(" << operand_for_node(*expr) << ", " << operand_for_stmt(Statement) << ");";
						} else if (Statement.GetType().SizeInBytes() > Statement.Expr()->GetType().SizeInBytes()) {
							//Otherwise we need to sign extend
							if (Statement.GetType().Signed)
								output << "builder.sx(" << operand_for_node(*expr) << ", " << operand_for_stmt(Statement) << ");";
							else
								output << "builder.zx(" << operand_for_node(*expr) << ", " << operand_for_stmt(Statement) << ");";
						}
					} else {
						// for the blockjit, a reinterpret is the same as a mov since things are usually kept in integer registers
						output << "builder.mov(" << operand_for_node(*expr) << ", " << operand_for_stmt(Statement) << ");";
					}

					return true;
				}

				std::string GetFixedValue() const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement &> (this->Statement);
					std::stringstream str;

					if(Statement.GetCastType() == SSACastStatement::Cast_Reinterpret) {
						IRType start = Statement.Expr()->GetType();
						const IRType &end = Statement.GetType();

						// there are only 4 valid casts here
						if(start == IRTypes::UInt32 && end == IRTypes::Float) {
							str << "bitcast_u32_float";
						} else if(start == IRTypes::UInt64 && end == IRTypes::Double) {
							str << "bitcast_u64_double";
						} else if(start == IRTypes::Float && end == IRTypes::UInt32) {
							str << "bitcast_float_u32";
						} else if(start == IRTypes::Double && end == IRTypes::UInt64) {
							str << "bitcast_double_u64";
						} else {
							assert(false);
							throw std::logic_error("Something bad happened");
						}

						str << "(" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ")";
					} else {
						str << "((" << Statement.GetType().GetCType() << ")" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ")";
					}

					return str.str();
				}

				std::string GetDynamicValue() const override
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement &> (this->Statement);

					if(Statement.IsFixed()) {
						switch (Statement.GetType().SizeInBytes()) {
							case 1:
								return "IROperand::const8(" + GetFixedValue() + ")";
							case 2:
								return "IROperand::const16(" + GetFixedValue() + ")";
							case 4:
								return "IROperand::const32(" + GetFixedValue() + ")";
							case 8:
								return "IROperand::const64(" + GetFixedValue() + ")";
							default:
								assert(false && "Unsupported statement size");
								UNEXPECTED;
						}
					} else {
						return "IROperand::vreg(" + Statement.GetName() + ", " + std::to_string(Statement.GetType().SizeInBytes()) + ")";
					}
				}

			};

			class SSAFreeRegisterStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAFreeRegisterStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}
			};

			class SSAIfStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAIfStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIfStatement &stmt = static_cast<const SSAIfStatement &> (this->Statement);

					output << "if (" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ") {\n";
					if (stmt.TrueTarget()->IsFixed() == BLOCK_ALWAYS_CONST)
						output << "goto block_" << stmt.TrueTarget()->GetName() << ";\n";
					else {
						CreateBlock(output, "block_idx_" + stmt.TrueTarget()->GetName());
						output << "builder.jump(IROperand::block(block));";
						if (end_label != "")
							output << "goto " << end_label << ";";
					}
					output << "} else {\n";
					if (stmt.FalseTarget()->IsFixed() == BLOCK_ALWAYS_CONST)
						output << "goto block_" << stmt.FalseTarget()->GetName() << ";\n";
					else {
						CreateBlock(output, "block_idx_" + stmt.FalseTarget()->GetName());
						output << "builder.jump(IROperand::block(block));";
						if (end_label != "")
							output << "goto " << end_label << ";";
					}
					output << "}\n";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIfStatement &Statement = static_cast<const SSAIfStatement&> (this->Statement);

					const SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());

					if (Statement.Expr()->IsFixed()) {
						//We can predicate which jump target to generate based on the result of the expression
						output << "if(" << expr->GetFixedValue() << ") {";
						CreateBlock(output, "block_idx_" + Statement.TrueTarget()->GetName());
						output << "builder.jump(IROperand::block(block));";
						output << "}";
						output << "else {";
						CreateBlock(output, "block_idx_" + Statement.TrueTarget()->GetName());
						output << "builder.jump(IROperand::block(block));";
						output << "}";
						if (end_label != "")
							output << "goto " << end_label << ";";
					} else {
						output << "{";
						output << "IRBlockId true_target;";
						output << "{";
						CreateBlock(output, "block_idx_" + Statement.TrueTarget()->GetName());
						output << "true_target = block;";
						output << "}";
						output << "IRBlockId false_target;";
						output << "{";
						CreateBlock(output, "block_idx_" + Statement.FalseTarget()->GetName());
						output << "false_target = block;";
						output << "}";

						output << "builder.branch("
						       << operand_for_node(*expr) << ", "
						       << "IROperand::block(true_target),"
						       << "IROperand::block(false_target)"
						       << ");";

						output << "}";
						if (end_label != "")
							output << "goto " << end_label << ";";
					}

					return true;
				}
			};

			class SSAIntrinsicStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAIntrinsicStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &> (this->Statement);

					const SSANodeWalker *arg0 = nullptr;
					if (Statement.ArgCount() > 0) arg0 = Factory.GetOrCreate(Statement.Args(0));

					switch (Statement.GetID()) {
						case IntrinsicID::ReadPC:
							assert(false && "ReadPC cannot be fixed");
							break;
						case IntrinsicID::PopCount32:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __builtin_popcount(" << arg0->GetFixedValue() << ");";
							break;
						case IntrinsicID::CLZ32:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __builtin_clz(" << arg0->GetFixedValue() << ");";
							break;
						case IntrinsicID::GetCpuMode:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = GetIsaMode();";
							break;

						case IntrinsicID::GetFeature:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = GetFeatureLevel(" << arg0->GetFixedValue() <<");";
							break;

						case IntrinsicID::SetCpuMode:
						case IntrinsicID::TakeException:
						case IntrinsicID::Trap:
							break;

						case IntrinsicID::UpdateZNFlags32:
						case IntrinsicID::UpdateZNFlags64:
						case IntrinsicID::ADC8_Flags:
						case IntrinsicID::ADC16_Flags:
						case IntrinsicID::ADC32_Flags:
						case IntrinsicID::ADC64_Flags:
						case IntrinsicID::ADC8:
						case IntrinsicID::ADC16:
						case IntrinsicID::ADC32:
						case IntrinsicID::ADC64:
							return EmitDynamicCode(output, end_label, fully_fixed);

						case IntrinsicID::Abs32:
						case IntrinsicID::Abs64:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = abs(" << arg0->GetFixedValue() << ");";
							break;

						default:
							throw std::logic_error("Unrecognised intrinsic: " + Statement.GetSignature().GetName());
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &> (this->Statement);

					const SSANodeWalker *arg0 = NULL;
					if (Statement.ArgCount() > 0) arg0 = Factory.GetOrCreate(Statement.Args(0));

					const SSANodeWalker *arg1 = NULL;
					if (Statement.ArgCount() > 1) arg1 = Factory.GetOrCreate(Statement.Args(1));

					const SSANodeWalker *arg2 = NULL;
					if (Statement.ArgCount() > 2) arg2 = Factory.GetOrCreate(Statement.Args(2));

					switch (Statement.GetID()) {
						case IntrinsicID::PopCount32:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.popcnt(" << operand_for_node(*arg0) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case IntrinsicID::BSwap32:
						case IntrinsicID::BSwap64:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.bswap(" << operand_for_node(*arg0) << ", " << operand_for_stmt(Statement) << ");";
							break;
						case IntrinsicID::CLZ32:
						case IntrinsicID::CLZ64:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.clz(" << operand_for_node(*arg0) << ", " << operand_for_stmt(Statement) << ");";
							break;

						case IntrinsicID::SetCpuMode:
							output << "SetIsaMode(" << operand_for_node(*arg0) << ", builder);";
							break;
						case IntrinsicID::Trap:
							output << "builder.trap();";
							break;
						case IntrinsicID::PendInterrupt:
//							output << "UNIMPLEMENTED; // pendirq\n";
							output << "builder.call(IROperand::const32(0), IROperand::func((void*)cpuPendInterrupt));";
							break;
						case IntrinsicID::PopInterrupt:
							// XXX TODO FIXME
							output << "builder.trap();";
							break;
						case IntrinsicID::PushInterrupt:
							// XXX TODO FIXME
							output << "builder.trap();";
							break;
						case IntrinsicID::HaltCpu:
							// XXX TODO FIXME
							output << "builder.trap();";
							break;
						case IntrinsicID::ProbeDevice:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.probe_device(" << operand_for_node(*arg0) << ", IROperand::vreg(" << Statement.GetName() << "));";
							break;
						case IntrinsicID::WriteDevice32:
							output << "builder.write_device(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ", " << operand_for_node(*arg2) << ");";
							break;
						case IntrinsicID::ReadPC:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.ldpc(IROperand::vreg(" << Statement.GetName() << ", " << Statement.GetType().SizeInBytes() << "));";
							break;
						case IntrinsicID::WritePC: {
							auto pc_descriptor = Statement.Parent->Parent->GetAction()->Context.Arch.GetRegFile().GetTaggedRegSlot("PC");

							output << "builder.streg(" << operand_for_node(*arg0) << ", IROperand::const32(" << pc_descriptor->GetRegFileOffset() << "));";

							break;
						}

						case IntrinsicID::ADC8_Flags:
						case IntrinsicID::ADC16_Flags:
						case IntrinsicID::ADC32_Flags:
						case IntrinsicID::ADC64_Flags:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.adc_with_flags(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ", " << operand_for_node(*arg2) << ");";
							break;
						case IntrinsicID::SBC8_Flags:
						case IntrinsicID::SBC16_Flags:
						case IntrinsicID::SBC32_Flags:
						case IntrinsicID::SBC64_Flags:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.sbc_with_flags(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ", " << operand_for_node(*arg2) << ");";
							break;
						case IntrinsicID::ADC8:
						case IntrinsicID::ADC16:
						case IntrinsicID::ADC32:
						case IntrinsicID::ADC64:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.mov(" << operand_for_node(*arg0) << ", " << operand_for_node(*this) << ");";
							output << "builder.add(" << operand_for_node(*arg1) << ", " << operand_for_node(*this) << ");";
							output << "builder.add(" << operand_for_node(*arg2) << ", " << operand_for_node(*this) << ");";
							break;

						case IntrinsicID::TakeException:
							output << "builder.take_exception(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ");";
							break;

						case IntrinsicID::EnterKernelMode:
							output << "UNIMPLEMENTED; // enterkernelmode\n";
//							output << "builder.call(IROperand::func((void*)cpuEnterKernelMode));";
							break;
						case IntrinsicID::EnterUserMode:
							output << "UNIMPLEMENTED; // enterusermode\n";
//							output << "builder.call(IROperand::func((void*)cpuEnterUserMode));";
							break;

						case IntrinsicID::UpdateZNFlags32:
						case IntrinsicID::UpdateZNFlags64:
							output << "builder.updatezn(" << operand_for_node(*arg0) << ");";
							break;


						case IntrinsicID::FloatAbs:
						case IntrinsicID::DoubleAbs:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.fabs(" << operand_for_node(*arg0) << ", " << operand_for_node(*this) << ");";
							break;

						case IntrinsicID::FPGetRounding:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.fctrl_get_round(" << operand_for_node(*this) << ");";
							break;

						case IntrinsicID::FPSetRounding:
							output << "builder.fctrl_set_round(" << operand_for_node(*arg0) << ");";
							break;

						case IntrinsicID::FPGetFlush:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.fctrl_get_flush(" << operand_for_node(*this) << ");";
							break;

						case IntrinsicID::FPSetFlush:
							output << "builder.fctrl_set_flush(" << operand_for_node(*arg0) << ");";
							break;

						case IntrinsicID::SetFeature:
							output << "SetFeatureLevel(" << arg0->GetFixedValue() << ", " << arg1->GetFixedValue() << ", builder);";
							if(!Statement.Parent->IsFixed() != BLOCK_ALWAYS_CONST) output << "InvalidateFeatures();";
							break;

						case IntrinsicID::FloatSqrt:
						case IntrinsicID::DoubleSqrt:
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";
							output << "builder.fsqrt(" << operand_for_node(*arg0) << ", " << operand_for_node(*this) << ");";
							break;

						default:
							fprintf(stderr, "error: unimplemented intrinsic:\n");

							std::ostringstream s;
							Statement.PrettyPrint(s);
							std::cerr << s.str() << std::endl;

							if(Statement.HasValue()) output << "IRRegId " << Statement.GetName() << ";";
							output << "assert(false && \"Unimplemented intrinsic " << s.str() << "\\n\");";
							break;
//						assert(false && "Unimplemented intrinsic");
					}

					return true;
				}

				std::string GetDynamicValue() const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &> (this->Statement);

					switch (Statement.GetID()) {
//					case SSAIntrinsicStatement::SSAIntrinsic_ReadPc:
//						return Statement.GetName();
						case IntrinsicID::GetCpuMode:
							return "IROperand::const32(GetIsaMode())";

						default:
							return BlockJitNodeWalker::GetDynamicValue();
					}
				}

				/*std::string GetFixedValue() const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &> (this->Statement);

					switch (Statement.Type) {
					case SSAIntrinsicStatement::SSAIntrinsic_ReadPc:
						return "insn.pc";
					default:
						return genc::ssa::SSANodeWalker::GetFixedValue();
					}
				}*/
			};

			class SSAJumpStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAJumpStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAJumpStatement &stmt = static_cast<const SSAJumpStatement &> (this->Statement);

					if (fully_fixed) {
						if (stmt.Target()->IsFixed() != BLOCK_ALWAYS_CONST) {
							output << "{";
							CreateBlock(output, "block_idx_" + stmt.Target()->GetName());
							output << "builder.jump(IROperand::block(block));";
							output << "}";
							if (end_label != "")
								output << "goto " << end_label << ";";
						} else {
							output << "goto block_" << stmt.Target()->GetName() << ";\n";
						}
					} else
						EmitDynamicCode(output, end_label, fully_fixed);

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAJumpStatement &Statement = static_cast<const SSAJumpStatement &> (this->Statement);
					output << "{";
					CreateBlock(output, "block_idx_" + Statement.Target()->GetName());
					output << "builder.jump(IROperand::block(block)); }";
					if (end_label != "")
						output << "goto " << end_label << ";";
					return true;
				}
			};

			class SSAMemoryReadStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAMemoryReadStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAMemoryReadStatement &Statement = static_cast<const SSAMemoryReadStatement &> (this->Statement);

					SSANodeWalker *address = Factory.GetOrCreate(Statement.Addr());

					output << "builder.ldmem(IROperand::const32(" << Statement.GetInterface()->GetID() << ")," << operand_for_node(*address) << ", IROperand::const32(0), " << operand_for_symbol(*Statement.Target()) << ");\n";

					output << "if(trace) {";
					output << "builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceOnlyMemRead" << (uint32_t)(8*Statement.Width) << "), " << operand_for_node(*address) << ", " << operand_for_symbol(*Statement.Target()) << ");";
					output << "}";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return EmitFixedCode(output, end_label, fully_fixed);
				}
			};

			class SSAMemoryWriteStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAMemoryWriteStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return EmitDynamicCode(output, end_label, fully_fixed);
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAMemoryWriteStatement &Statement = static_cast<const SSAMemoryWriteStatement&> (this->Statement);
					SSANodeWalker *address = Factory.GetOrCreate(Statement.Addr());
					SSANodeWalker *value = Factory.GetOrCreate(Statement.Value());

//					if (Statement.User) {
//						output << "builder.stmem_user(" << operand_for_node(*value) << ", " << operand_for_node(*address) << ");\n";
//					} else {
					output << "builder.stmem(IROperand::const32(" << Statement.GetInterface()->GetID() << "), " << operand_for_node(*value) << ", IROperand::const32(0), " << operand_for_node(*address) << ");\n";
					output << "if(trace)";
					output << "builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceOnlyMemWrite" << (uint32_t)(8*Statement.Width) << "), " << operand_for_node(*address) << ", " << operand_for_node(*value) << ");";
//					}

					return true;
				}
			};

			class SSAReadStructMemberStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAReadStructMemberStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAReadStructMemberStatement &Statement = static_cast<const SSAReadStructMemberStatement &> (this->Statement);
					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");";

					output << "builder.mov(IROperand::const";

					switch (Statement.GetType().SizeInBytes()) {
						case 1:
							output << "8";
							break;
						case 2:
							output << "16";
							break;
						case 4:
							output << "32";
							break;
						case 8:
							output << "64";
							break;
						default:
							assert(false);
					}

					output << "(insn" << Statement.FormatMembers() << "), " << operand_for_stmt(Statement) << "));";
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAReadStructMemberStatement &stmt = static_cast<const SSAReadStructMemberStatement &> (this->Statement);
					std::stringstream str;

					if (stmt.MemberNames.at(0) == "IsPredicated") {
						str << "insn.GetIsPredicated()";
					} else if(stmt.MemberNames.at(0) == "PredicateInfo") {
						str << "insn.GetPredicateInfo()";
					} else {
						str << "insn" << stmt.FormatMembers();
					}

					return str.str();
				}
			};

			class SSARegisterStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSARegisterStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitBankedRegisterWrite(util::cppformatstream &output, const SSARegisterStatement &write) const
				{
					SSANodeWalker *RegNum = Factory.GetOrCreate(write.RegNum());
					SSANodeWalker *Value = Factory.GetOrCreate(write.Value());

					const auto &bank = Statement.Parent->Parent->GetContext().GetArchDescription().GetRegFile().GetBankByIdx(write.Bank);
					uint32_t offset = bank.GetRegFileOffset();
					uint32_t register_width = bank.GetRegisterWidth();
					uint32_t register_stride = bank.GetRegisterStride();

					if (write.RegNum()->IsFixed()) {
						std::string value_string;

						if(Value->Statement.GetType().SizeInBytes() < register_width) {
							output << "IRRegId tmp = builder.alloc_reg(" << register_width  << ");";
							output << "builder.zx(" << operand_for_node(*Value) << ", IROperand::vreg(tmp, " << register_width << "));";
							std::stringstream str;
							str << "IROperand::vreg(tmp, " << (uint32_t)register_width << ")";
							value_string = str.str();
						} else {
							value_string = operand_for_node(*Value);
						}

						output << "builder.streg("
						       << value_string << ", "
						       <<"IROperand::const32((uint32_t)(" << offset << " + (" << register_stride << " * " << RegNum->GetFixedValue() << ")))"
						       << ");\n";

						if(register_width <= 8) {
							output << "if(trace) {";
							output << "IRRegId tmp = builder.alloc_reg(8);\n";
							if(Value->Statement.GetType().SizeInBytes() < 8) {
								output << "builder.zx(" << operand_for_node(*Value) << ", IROperand::vreg(tmp, 8));";
							} else {
								output << "builder.mov(" << operand_for_node(*Value) << ", IROperand::vreg(tmp, 8));";
							}

							output << "builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegBankWrite), IROperand::const8(" << (uint32_t)write.Bank << "), IROperand::const8(" << RegNum->GetFixedValue() << "), IROperand::const32(" << register_width << "), IROperand::vreg(tmp,8));";
							output << "}";
						}
						return true;
					} else {
						output << "{";
						output << "IRRegId tmp = builder.alloc_reg(8);\n";

						output << "builder.mov(" << operand_for_node(*RegNum) << ", IROperand::vreg(tmp, 8));";
						output << "builder.imul(IROperand::const32((uint32_t)" << register_stride << "), IROperand::vreg(tmp, 8));";
						output << "builder.add(IROperand::const32(" << offset << "), IROperand::vreg(tmp, 8));";

						output << "builder.streg(" << operand_for_node(*Value) << ", IROperand::vreg(tmp, 8));\n";
						if(register_width <= 8) {
							output << "if(trace) builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegBankWrite), IROperand::const8(" << (uint32_t)write.Bank << "), " << operand_for_node(*RegNum) << ", IROperand::const32(" << register_width << "), " << operand_for_node(*Value) << ");";
						}
						output << "}";
					}

					return true;
				}

				bool EmitBankedRegisterRead(util::cppformatstream &output, const SSARegisterStatement &write) const
				{
					SSANodeWalker *RegNum = Factory.GetOrCreate(write.RegNum());

					const auto &bank = Statement.Parent->Parent->GetContext().GetArchDescription().GetRegFile().GetBankByIdx(write.Bank);
					uint32_t offset = bank.GetRegFileOffset();
					uint32_t register_width = bank.GetRegisterWidth();
					uint32_t register_stride = bank.GetRegisterStride();

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << register_width << ");\n";

					if (write.RegNum()->IsFixed()) {
						output << "builder.ldreg(IROperand::const32((uint32_t)(" << offset << " + (" << register_stride << " * " << RegNum->GetFixedValue() << "))), " << operand_for_stmt(Statement) << ");\n";
						output << "if(trace) builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegBankRead), IROperand::const8(" << (uint32_t)write.Bank << "), IROperand::const8(" << RegNum->GetFixedValue() << "), IROperand::const32(" << register_width << "), " << operand_for_stmt(Statement) << ");";
						return true;
					} else {
						output << "{";
						output << "IRRegId tmp = builder.alloc_reg(4);";

						output << "builder.mov(" << operand_for_node(*RegNum) << ", IROperand::vreg(tmp, 4));";
						output << "builder.imul(IROperand::const32((uint32_t)" << register_stride << "), IROperand::vreg(tmp, 4));";
						output << "builder.add(IROperand::const32(" << offset << "), IROperand::vreg(tmp, 4));";

						output << "builder.ldreg(IROperand::vreg(tmp, 4), " << operand_for_stmt(Statement) << ");";

						output <<
						       "if(trace) {"
						       "  builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegBankRead), IROperand::const8(" << (uint32_t)write.Bank << ")," << operand_for_node(*RegNum) << ", IROperand::const32(" << register_width << "), " << operand_for_stmt(Statement) << ");"
						       "}";

						output << "}";
					}

					return true;
				}

				bool EmitRegisterWrite(util::cppformatstream &output, const SSARegisterStatement &write) const
				{
					SSANodeWalker *Value = Factory.GetOrCreate(write.Value());

					const auto &reg = Statement.Parent->Parent->GetContext().GetArchDescription().GetRegFile().GetSlotByIdx(write.Bank);
					uint32_t offset = reg.GetRegFileOffset();
					uint32_t register_width = reg.GetWidth();

					if (Value->Statement.GetType().SizeInBytes() > register_width) {
						output << "{";
						output << "IRRegId tmp = builder.alloc_reg(" << register_width << ");";
						output << "builder.trunc(" << operand_for_node(*Value) << ", IROperand::vreg(tmp, " << register_width << "));";
						output << "builder.streg(IROperand::vreg(tmp, " << register_width << "), IROperand::const32(" << offset << "));";
						output << "if(trace) builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegWrite), IROperand::const8(" << (uint32_t)write.Bank << "), IROperand::vreg(tmp, " << register_width << "));";
						output << "}";
					} else if (Value->Statement.GetType().SizeInBytes() < register_width) {
						assert(false);
					} else {
						output << "builder.streg(" << operand_for_node(*Value) << ", IROperand::const32(" << offset << "));";

						output << "if(trace) {";
						if(register_width < 8) {
							output << "  IRRegId tmp = builder.alloc_reg(8);";
							output << "  builder.zx(" << operand_for_node(*Value) << ", IROperand::vreg(tmp, 8));";
							output << "  builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegWrite), IROperand::const8(" << (uint32_t)write.Bank << "), IROperand::vreg(tmp, 8));";
						} else {
							output << "  builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegWrite), IROperand::const8(" << (uint32_t)write.Bank << ")," << operand_for_node(*Value) << ");";
						}
						output << "}";

					}

					return true;
				}

				bool EmitRegisterRead(util::cppformatstream &output, const SSARegisterStatement &write) const
				{
					const auto &reg = Statement.Parent->Parent->GetContext().GetArchDescription().GetRegFile().GetSlotByIdx(write.Bank);
					uint32_t offset = reg.GetRegFileOffset();
					uint32_t register_width = reg.GetWidth();

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << register_width << ");\n";
					output << "builder.ldreg(IROperand::const32(" << offset << "), " << operand_for_stmt(Statement) << ");";

					output << "if(trace) {";
					if(register_width < 8) {
						output << "  IRRegId tmp = builder.alloc_reg(8);";
						output << "  builder.zx(IROperand::vreg(" << Statement.GetName() << ", " << register_width << "), IROperand::vreg(tmp, 8));";
						output << "  builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegRead), IROperand::const8(" << (uint32_t)write.Bank << "), IROperand::vreg(tmp, 8));";
					} else {
						output << "  builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceRegRead), IROperand::const8(" << (uint32_t)write.Bank << "), IROperand::vreg(" << Statement.GetName() << ", 8));";
					}

					output << "}";

					return true;
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					EmitDynamicCode(output, end_label, fully_fixed);

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSARegisterStatement &Statement = static_cast<const SSARegisterStatement &> (this->Statement);

					if (Statement.IsBanked) {
						if (Statement.IsRead) {
							EmitBankedRegisterRead(output, Statement);
							return true;
						} else {
							EmitBankedRegisterWrite(output, Statement);
							return true;
						}
					} else {
						if (Statement.IsRead) {
							EmitRegisterRead(output, Statement);
							return true;
						} else {
							EmitRegisterWrite(output, Statement);
							return true;
						}
					}

					return true;
				}
			};

			class SSAReturnStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAReturnStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAReturnStatement &Statement = static_cast<const SSAReturnStatement &> (this->Statement);
					if (fully_fixed) {
						if (Statement.Value()) {
							output << "builder.mov(" << operand_for_node(*Factory.GetOrCreate(Statement.Value())) << ", IROperand::vreg(__result, 1));";
						}
						if (end_label != "")
							output << "goto " << end_label << ";\n";
					} else {
						EmitDynamicCode(output, end_label, fully_fixed);
					}
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAReturnStatement &Statement = static_cast<const SSAReturnStatement&> (this->Statement);

					const IRAction &action = *Statement.Parent->Parent->GetAction();
					if (dynamic_cast<const IRHelperAction*> (&action)) {
						if (Statement.Value()) {
							output << "builder.mov(" << operand_for_node(*Factory.GetOrCreate(Statement.Value())) << ", IROperand::vreg(__result, 1));";
						}
						if (!fully_fixed) {
							assert(false);
						}
						if (end_label != "")
							output << "goto " << end_label << ";";
					} else {
						output << "builder.jump(IROperand::block(__exit_block));";
						if (end_label != "")
							output << "goto " << end_label << ";";
					}
					return true;
				}
			};

			class SSASelectStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSASelectStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASelectStatement &Statement = static_cast<const SSASelectStatement &> (this->Statement);

					SSANodeWalker *Cond = Factory.GetOrCreate(Statement.Cond());
					SSANodeWalker *True = Factory.GetOrCreate(Statement.TrueVal());
					SSANodeWalker *False = Factory.GetOrCreate(Statement.FalseVal());

					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = ((" << Statement.GetType().GetCType() << ")";
					output << "(" << Cond->GetFixedValue() << " ? (" << True->GetFixedValue() << ") : (" << False->GetFixedValue() << ")));";
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASelectStatement &Statement = static_cast<const SSASelectStatement &> (this->Statement);

					const SSANodeWalker *cond = Factory.GetOrCreate(Statement.Cond());
					const SSANodeWalker *if_true = Factory.GetOrCreate(Statement.TrueVal());
					const SSANodeWalker *if_false = Factory.GetOrCreate(Statement.FalseVal());

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";

					if (Statement.Cond()->IsFixed()) {
						output << "if(" << cond->GetFixedValue() << ") {";
						output << "  builder.mov(" << operand_for_node(*if_true) << ", " << operand_for_stmt(Statement) << ");";
						output << "} else {";
						output << "  builder.mov(" << operand_for_node(*if_false) << ", " << operand_for_stmt(Statement) << ");";
						output << "}";
					} else {
						output << "builder.mov(" << operand_for_node(*if_false) << ", " << operand_for_stmt(Statement) << ");";
						output << "builder.cmov(" << operand_for_node(*cond) << ", " << operand_for_node(*if_true) << ", " << operand_for_stmt(Statement) << ");";
					}

					return true;
				}
			};

			class SSASwitchStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSASwitchStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASwitchStatement &Statement = static_cast<const SSASwitchStatement &> (this->Statement);

					SSANodeWalker *Expr = Factory.GetOrCreate(Statement.Expr());

					output << "switch (" << Expr->GetFixedValue() << ") {";
					for (auto v : Statement.GetValues()) {
						SSANodeWalker *c = Factory.GetOrCreate(v.first);
						SSABlock* target = v.second;
						output << "case " << c->GetFixedValue() << ":\n";

						if (target->IsFixed() == BLOCK_ALWAYS_CONST)
							output << "goto block_" << target->GetName() << ";\n";
						else {
							output << "{";
							CreateBlock(output, "block_idx_" + target->GetName());
							output << "builder.jump(IROperand::block(block));";
							output << "}";
						}
						output << "break;";

					}
					output << "default:\n";

					const SSABlock *target = Statement.Default();
					if (target->IsFixed() == BLOCK_ALWAYS_CONST)
						output << "goto block_" << target->GetName() << ";\n";
					else {
						output << "{";
						CreateBlock(output, "block_idx_" + target->GetName());
						output << "builder.jump(IROperand::block(block));";
						output << "}";
					}

					output << "}";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					assert(false);
					return true;
				}
			};

			class SSAUnaryArithmeticStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAUnaryArithmeticStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					auto &stmt = (genc::ssa::SSAUnaryArithmeticStatement&)Statement;

					SSANodeWalker *ExprNode = Factory.GetOrCreate(stmt.Expr());

					switch(stmt.Type) {
							using namespace gensim::genc::SSAUnaryOperator;
						case OP_COMPLEMENT:
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ~(" << ExprNode->GetFixedValue() << ");";
							break;
						case OP_NEGATE:
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = !(" << ExprNode->GetFixedValue() << ");";
							break;
						case OP_NEGATIVE:
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = -(" << ExprNode->GetFixedValue() << ");";
							break;
						default:
							assert(false);
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					auto &stmt = (genc::ssa::SSAUnaryArithmeticStatement&)Statement;

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");\n";

					SSANodeWalker *ExprNode = Factory.GetOrCreate(stmt.Expr());

					switch(stmt.Type) {
							using namespace gensim::genc::SSAUnaryOperator;
						case OP_COMPLEMENT:
							assert(!stmt.GetType().IsFloating());
							//output << "IRRegId " << stmt.GetName() << " = builder.alloc_reg(" << stmt.GetType().Size() << ");";
							output << "builder.mov(" << operand_for_node(*Factory.GetOrCreate(stmt.Expr())) << ", " << operand_for_stmt(stmt) << ");";

							switch (stmt.GetType().SizeInBytes()) {
								case 1:
									output << "builder.bitwise_xor(IROperand::const8(0xff), " << operand_for_stmt(stmt) << ");";
									break;
								case 2:
									output << "builder.bitwise_xor(IROperand::const16(0xffff), " << operand_for_stmt(stmt) << ");";
									break;
								case 4:
									output << "builder.bitwise_xor(IROperand::const32(0xffffffff), " << operand_for_stmt(stmt) << ");";
									break;
								case 8:
									output << "builder.bitwise_xor(IROperand::const64(0xffffffffffffffffull), " << operand_for_stmt(stmt) << ");";
									break;
								default:
									assert(false);
							}


							//output << "builder.bitwise_not(" << operand_for_stmt(stmt) << "));";
							break;

						case OP_NEGATE:
							assert(!stmt.GetType().IsFloating());

							//output << "IRRegId " << stmt.GetName() << " = ctx.alloc_reg(" << stmt.GetType().Size() << ");";

							if (stmt.GetType().SizeInBytes() == 1) {
								output << "builder.cmpeq(" << operand_for_node(*Factory.GetOrCreate(stmt.Expr())) << ", IROperand::const8(0), " << operand_for_stmt(stmt) << ");";
							} else {
								output << "{";
								output << "IRRegId tmp = builder.alloc_reg(1);";

								// LHS RHS OUT
								output << "builder.cmpeq(" << operand_for_node(*Factory.GetOrCreate(stmt.Expr())) << ", IROperand::const";
								output << (uint32_t)(stmt.GetType().SizeInBytes() * 8) << "(0), IROperand::vreg(tmp, 1));";

								output << "builder.zx(IROperand::vreg(tmp, 1), " << operand_for_stmt(stmt) << ");";
								output << "}";
							}

							break;

						case OP_NEGATIVE:
							// only implemented for FP values at the moment
							assert(stmt.GetType().IsFloating());

							if(stmt.GetType().IsFloating()) {
								output << "builder.mov(" << operand_for_node(*ExprNode) << ", " << operand_for_node(*this) << ");";

								switch(stmt.GetType().SizeInBytes()) {
									case 4:
										output << "builder.bitwise_xor(IROperand::const32(0x80000000UL), " << operand_for_node(*this) << ");";
										break;
									case 8:
										output << "builder.bitwise_xor(IROperand::const64(0x8000000000000000ULL), " << operand_for_node(*this) << ");";
										break;
								}
							}
							break;
						default:
							assert(false);
					}


					return true;
				}

			};

			class SSAVariableReadStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAVariableReadStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					//		const SSAVariableReadStatement &Statement = static_cast<const SSAVariableReadStatement &>(this->Statement);
					//		output << "archsim::translate::insnjit::lir::LirVirtualRegister *" << Statement.GetName() << " = lir.create_vreg(" << Statement.GetType().Size() <<");\n";
					//
					//		output << "lir.mov(*" << Statement.GetName() << ", *lir_variables[lir_idx_" << Statement.Target->GetName() << "]);";
					//
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAVariableReadStatement &Statement = static_cast<const SSAVariableReadStatement &> (this->Statement);
					return "CV_" + Statement.Target()->GetName();
				}

				std::string GetDynamicValue() const
				{
					const SSAVariableReadStatement &Statement = static_cast<const SSAVariableReadStatement &> (this->Statement);
					return "IROperand::vreg(ir_idx_" + Statement.Target()->GetName() + ", " + std::to_string(Statement.Target()->GetType().SizeInBytes()) + ")";
				}
			};

			class SSAVariableWriteStatementWalker : public BlockJitNodeWalker
			{
			public:

				SSAVariableWriteStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableWriteStatement &Statement = static_cast<const SSAVariableWriteStatement &> (this->Statement);
					SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());

					output << "CV_" << Statement.Target()->GetName() << " = " << expr->GetFixedValue() << ";";

					if (Statement.Parent->Parent->HasDynamicDominatedReads(&Statement)) {
						output << "builder.mov(" << operand_for_node(*expr) << ", " << operand_for_symbol(*Statement.Target()) << ");";

						//output << "lir.mov(*lir_variables[lir_idx_" << Statement.GetTarget()->GetName() << "], CV_" << Statement.GetTarget()->GetName() << ");";
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableWriteStatement &Statement = static_cast<const SSAVariableWriteStatement &> (this->Statement);

					SSANodeWalker *value_node = Factory.GetOrCreate(Statement.Expr());

					if (Statement.Target()->GetType().SizeInBytes() > value_node->Statement.GetType().SizeInBytes()) {
						output << "builder.zx(" << operand_for_node(*value_node) << ", " << operand_for_symbol(*Statement.Target()) << ");";
					} else if (Statement.Target()->GetType().SizeInBytes() < value_node->Statement.GetType().SizeInBytes()) {
						output << "builder.trunc(" << operand_for_node(*value_node) << ", " << operand_for_symbol(*Statement.Target()) << ");";
					} else {
						output << "builder.mov(" << operand_for_node(*value_node) << ", " << operand_for_symbol(*Statement.Target()) << ");";
					}

					//output << "lir.mov(*lir_variables[lir_idx_" << Statement.GetTarget()->GetName() << "], " << maybe_deref(value_node) << ");";
					return true;
				}
			};

			class SSADeviceReadStatementWalker : public BlockJitNodeWalker
			{
			public:
				SSADeviceReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory *_fact) : BlockJitNodeWalker(_statement, *_fact) {}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label, bool fully_fixed) const
				{
					const SSADeviceReadStatement *stmt = dynamic_cast<const SSADeviceReadStatement*>(&Statement);
					assert(stmt);

					SSANodeWalker *arg0 = Factory.GetOrCreate(stmt->Device());
					SSANodeWalker *arg1 = Factory.GetOrCreate(stmt->Address());

					output << "builder.read_device("
					       << operand_for_node(*arg0)
					       << ", "
					       << operand_for_node(*arg1)
					       << ", "
					       << operand_for_symbol(*(stmt->Target())) << ");";

					return true;
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					assert(false);
					return false;
				}
			};


			class SSACallStatementWalker : public BlockJitNodeWalker
			{
			public:
				SSACallStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{

				}

				virtual bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					return true;
				}

				virtual bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					const SSACallStatement &Statement = static_cast<const SSACallStatement &> (this->Statement);

					// IR Call instruction supports up to 5 operands
					assert(Statement.ArgCount() <= 5);

					std::string function_name;
					function_name = "helper_" + Statement.Target()->GetContext().GetIsaDescription().ISAName + "_" + Statement.Target()->GetPrototype().GetIRSignature().GetName() + "<false>";

					if (Statement.Target()->GetPrototype().GetIRSignature().GetName() == "flush") {
						output << "builder.flush();\n";
					} else if (Statement.Target()->GetPrototype().GetIRSignature().GetName() == "flush_itlb") {
						output << "builder.flush_itlb();\n";
					} else if (Statement.Target()->GetPrototype().GetIRSignature().GetName() == "flush_dtlb") {
						output << "builder.flush_dtlb();\n";
					} else if (Statement.Target()->GetPrototype().GetIRSignature().GetName() == "flush_itlb_entry") {
						output << "builder.flush_itlb_entry(" << operand_for_node(*Factory.GetOrCreate(dynamic_cast<const SSAStatement*>(Statement.Arg(0)))) << ");\n";
					} else if (Statement.Target()->GetPrototype().GetIRSignature().GetName() == "flush_dtlb_entry") {
						output << "builder.flush_dtlb_entry(" << operand_for_node(*Factory.GetOrCreate(dynamic_cast<const SSAStatement*>(Statement.Arg(0)))) << ");\n";
					} else {
						std::string output_string;
						if(Statement.HasValue()) {
							output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");";
							output_string = operand_for_node(*this);
						} else {
							output_string = "IROperand::const32(0)";
						}
						output << "builder.call";
						output << "(" << output_string << ", IROperand::func((void *)&" << function_name << ")";

						for(unsigned i = 0; i < Statement.ArgCount(); ++i) {
							auto arg = Statement.Arg(i);
							SSANodeWalker *argWalker = Factory.GetOrCreate(dynamic_cast<const SSAStatement*>(arg));
							output << ", " << operand_for_node(*argWalker);
						}

						output << ");\n";
					}

					return true;
				}
			};

			class SSAVectorExtractElementStatementWalker : public BlockJitNodeWalker
			{
			public:
				SSAVectorExtractElementStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{

				}

				virtual bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					return true;
				}

				virtual bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					const SSAVectorExtractElementStatement &Statement = static_cast<const SSAVectorExtractElementStatement &> (this->Statement);

					// shift and mask

					uint64_t mask = 0;
					switch(Statement.GetType().SizeInBytes()) {
						case 1:
							mask = 0xff;
							break;
						case 2:
							mask = 0xffff;
							break;
						case 4:
							mask = 0xffffffff;
							break;
						case 8:
							mask = 0xffffffffffffffff;
							break;
						default:
							assert(false);
					}

					uint32_t base_size_bytes = Statement.Base()->GetType().SizeInBytes();

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");";
					output << "{";
					output << "IRRegId temp = builder.alloc_reg(" << base_size_bytes << ");";
					output << "builder.mov(" << Factory.GetOrCreate(Statement.Base())->GetDynamicValue() << ", IROperand::vreg(temp, " << base_size_bytes <<  "));";
					if(Statement.Index()->IsFixed()) {
						output << "builder.shr(IROperand::const" << (Statement.Index()->GetType().SizeInBytes()*8) << "(8 * " << Factory.GetOrCreate(Statement.Index())->GetFixedValue() << " * " << Statement.GetType().SizeInBytes() << "), IROperand::vreg(temp, " << base_size_bytes << "));";
					} else {
						assert(false);
					}
					output << "builder.bitwise_and(IROperand::const" << (base_size_bytes*8) << "(" << mask << "ull), IROperand::vreg(temp, " << base_size_bytes << "));";
					output << "builder.trunc(IROperand::vreg(temp, " << base_size_bytes << "), " << operand_for_stmt(Statement) << ");";
					output << "}";
					return true;
				}
			};

			class SSAVectorInsertElementStatementWalker : public BlockJitNodeWalker
			{
			public:
				SSAVectorInsertElementStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{

				}

				virtual bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					return true;
				}

				virtual bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					const SSAVectorInsertElementStatement &Statement = static_cast<const SSAVectorInsertElementStatement &> (this->Statement);

					// mask out old entry
					uint64_t mask = 0;
					switch(Statement.Value()->GetType().SizeInBytes()) {
						case 1:
							mask = 0xff;
							break;
						case 2:
							mask = 0xffff;
							break;
						case 4:
							mask = 0xffffffff;
							break;
						case 8:
							mask = 0xffffffffffffffff;
							break;
						default:
							assert(false);
					}

					auto IndexNode = Factory.GetOrCreate(Statement.Index());
					auto ExprNode = Factory.GetOrCreate(Statement.Value());
					auto BaseNode = Factory.GetOrCreate(Statement.Base());

					output << "IRRegId " << Statement.GetName() << " = builder.alloc_reg(" << Statement.GetType().SizeInBytes() << ");";
					output << "{";
					if(Statement.Index()->IsFixed()) {
						output << "uint64_t mask = ~(" << mask << "ull << (8*" << IndexNode->GetFixedValue() << "*" << Statement.Value()->GetType().SizeInBytes() << "));";
						output << "uint64_t shift = 8*" << Statement.Value()->GetType().SizeInBytes() << " * " << IndexNode->GetFixedValue() << ";";
					} else {
						assert(false);
					}
					uint32_t target_size = Statement.GetType().SizeInBytes();
					output << "IRRegId val  = builder.alloc_reg(" << target_size << ");";
					output << "IRRegId val2  = builder.alloc_reg(" << target_size << ");";
					output << "builder.mov(" << BaseNode->GetDynamicValue() << ", IROperand::vreg(val2, " << target_size << "));";
					output << "builder.bitwise_and(IROperand::const" << target_size*8 << "(mask), IROperand::vreg(val2, " << target_size << "));";

					// shift new entry
					output << "builder.zx(" << ExprNode->GetDynamicValue() << ", IROperand::vreg(val, " << target_size << "));";
					output << "builder.shl(IROperand::const" << target_size*8 << "(shift), IROperand::vreg(val, " << target_size << "));";

					// orr in new entry
					output << "builder.bitwise_or(IROperand::vreg(val, " << target_size << "), IROperand::vreg(val2, " << target_size << "));";
					output << "builder.mov(IROperand::vreg(val2, " << target_size << "), " << operand_for_node(*this) << ");";

					output << "}";

					return true;
				}
			};

			class SSATrapWalker : public BlockJitNodeWalker
			{
			public:
				SSATrapWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : BlockJitNodeWalker(stmt, *_fact)
				{

				}

				virtual bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					return true;
				}

				virtual bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					const SSACallStatement &Statement = static_cast<const SSACallStatement &> (this->Statement);

//					output << "//" << Statement.PrettyPrint() << "\n";
					if(Statement.HasValue()) output << "IRRegId " << Statement.GetName() << ";";
					output << "assert(false && \"trap\");";

					return true;
				}
			};
		}
	}
}

#define STMT_IS(x) (typeid(*stmt) == typeid(genc::ssa::x))
#define HANDLE(x) if (STMT_IS(x)) return new gensim::generator::jit::x##Walker(*stmt, this)

SSANodeWalker *JitNodeWalkerFactory::Create(const SSAStatement *stmt)
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
	HANDLE(SSAUnaryArithmeticStatement);
	HANDLE(SSAVariableReadStatement);
	HANDLE(SSAVariableWriteStatement);
	HANDLE(SSACallStatement);

	HANDLE(SSAVectorExtractElementStatement);
	HANDLE(SSAVectorInsertElementStatement);

	return new gensim::generator::jit::SSATrapWalker(*stmt, this);
}
