/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "JITv2NodeWalker.h"
#include "isa/ISADescription.h"
#include "genC/Enums.h"
#include "genC/Parser.h"
#include "genC/ir/IR.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IREnums.h"
#include "genC/ir/IRRegisterExpression.h"
#include "genC/ir/IRScope.h"
#include "genC/ir/IRScope.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSABuilder.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAWalkerFactory.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/analysis/RegisterAllocationAnalysis.h"

#include <cassert>
#include <typeinfo>

using namespace gensim::generator::captive;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

namespace gensim
{
	namespace generator
	{
		namespace captive_v2
		{

			void CreateBlock(util::cppformatstream &output, std::string block_id_name)
			{
				output << "auto block = " << block_id_name << ";\n";
				output << "dynamic_block_queue.push(" << block_id_name << ");\n";
			}

			static std::string type_for_statement(const SSAStatement& stmt)
			{
				return "emitter.context().types()." + stmt.GetType().GetUnifiedType() + "()";
			}

			static std::string type_for_symbol(const SSASymbol& sym)
			{
				return "emitter.context().types()." + sym.GetType().GetUnifiedType() + "()";
			}

			static std::string operand_for_symbol(const SSASymbol& sym)
			{
				return "DV_" + sym.GetName();
			}

			static std::string operand_for_node(const SSANodeWalker& node)
			{
				if (node.Statement.IsFixed()) {
					if (node.Statement.GetType() == IRTypes::UInt8) {
						return "emitter.const_u8(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::UInt16) {
						return "emitter.const_u16(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::UInt32) {
						return "emitter.const_u32(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::UInt64) {
						return "emitter.const_u64(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::UInt128) {
						return "emitter.const_u128(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int8) {
						return "emitter.const_s8(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int16) {
						return "emitter.const_s16(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int32) {
						return "emitter.const_s32(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int64) {
						return "emitter.const_s64(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Int128) {
						return "emitter.const_s128(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Float) {
						return "emitter.const_f32(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType() == IRTypes::Double) {
						return "emitter.const_f64(" + node.GetFixedValue() + ")";
					} else if (node.Statement.GetType().VectorWidth > 1) {
						auto t = node.Statement.GetType();
						if (t.IsFloating()) {
							return "emitter.constant_vector_splat_f(" + node.GetFixedValue() + "[0], " + type_for_statement(node.Statement) + ")";
						} else {
							return "emitter.constant_vector_splat(" + node.GetFixedValue() + "[0], " + type_for_statement(node.Statement) + ")";
						}
						//return "emitter.const_vector(" + node.GetFixedValue() + ", " + type_for_statement(node.Statement) + ")";
					} else {
						fprintf(stderr, "node type: %s @ %s:%d\n", node.Statement.GetType().GetUnifiedType().c_str(), node.Statement.GetDiag().Filename().c_str(), node.Statement.GetDiag().Line());
						assert(false && "Unsupported node type");
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
							if (stmt.GetType().Signed) {
								return "emitter.const_s8(CV_" + stmt.GetName() + ")";
							} else {
								return "emitter.const_u8(CV_" + stmt.GetName() + ")";
							}
						case 2:
							if (stmt.GetType().Signed) {
								return "emitter.const_s16(CV_" + stmt.GetName() + ")";
							} else {
								return "emitter.const_u16(CV_" + stmt.GetName() + ")";
							}
						case 4:
							if (stmt.GetType().Signed) {
								return "emitter.const_s32(CV_" + stmt.GetName() + ")";
							} else {
								return "emitter.const_u32(CV_" + stmt.GetName() + ")";
							}
						case 8:
							if (stmt.GetType().Signed) {
								return "emitter.const_s64(CV_" + stmt.GetName() + ")";
							} else {
								return "emitter.const_u64(CV_" + stmt.GetName() + ")";
							}
						default:
							assert(false && "Unsupported statement size");
							UNEXPECTED;
					}

				} else {
					return stmt.GetName();
				}
			}

			class SSABinaryArithmeticStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSABinaryArithmeticStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
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
					}

					if (stmt.Type == BinaryOperator::RotateRight) {
						output << "(__ror(" << LHSNode->GetFixedValue() << ", " << RHSNode->GetFixedValue() << ")));";
					} else if (stmt.Type == BinaryOperator::RotateLeft) {
						output << "(__rol(" << LHSNode->GetFixedValue() << ", " << RHSNode->GetFixedValue() << ")));";
					} else {
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
								throw std::logic_error("Unsupported binary operator '" + std::to_string(stmt.Type) + "'");

						}

						output << RHSNode->GetFixedValue() << "));";
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSABinaryArithmeticStatement &Statement = static_cast<const SSABinaryArithmeticStatement &> (this->Statement);

					SSANodeWalker *LHSNode = Factory.GetOrCreate(Statement.LHS());
					SSANodeWalker *RHSNode = Factory.GetOrCreate(Statement.RHS());

					output << "auto " << Statement.GetName() << " = ";

					switch (Statement.Type) {
						// Shift
						case BinaryOperator::ShiftLeft:
							output << "emitter.shl(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::RotateRight:
							output << "emitter.ror(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::RotateLeft:
							output << "emitter.rol(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::ShiftRight:
							output << "emitter.shr(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::SignedShiftRight:
							output << "emitter.sar(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;

						// Equality
						case BinaryOperator::Equality:
							output << "emitter.cmp_eq(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::Inequality:
							output << "emitter.cmp_ne(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::LessThan:
							output << "emitter.cmp_lt(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::LessThanEqual:
							output << "emitter.cmp_le(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::GreaterThanEqual:
							output << "emitter.cmp_ge(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::GreaterThan:
							output << "emitter.cmp_gt(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;

						// Arithmetic
						case BinaryOperator::Add:
							output << "emitter.add(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";

							break;
						case BinaryOperator::Subtract:
							output << "emitter.sub(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::Multiply:
							output << "emitter.mul(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::Divide:
							output << "emitter.div(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::Modulo:
							output << "emitter.mod(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;

						// Logical
						case BinaryOperator::Logical_And:
						case BinaryOperator::Logical_Or:
							assert(false && "Unexpected instruction (we should have got rid of these during optimisation!)");
							break;

						// Bitwise
						case BinaryOperator::Bitwise_XOR:
							output << "emitter.bitwise_xor(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::Bitwise_And:
							output << "emitter.bitwise_and(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;
						case BinaryOperator::Bitwise_Or:
							output << "emitter.bitwise_or(" << operand_for_node(*LHSNode) << ", " << operand_for_node(*RHSNode) << ");";
							break;

						default:
							throw std::logic_error("Unsupported binary operator '" + std::to_string(Statement.Type) + "'");
					}

#ifdef REGISTER_ALLOCATION_HINTS
					if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
						output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
					}
#endif
					return true;
				}
			};

			class SSAConstantStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAConstantStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAConstantStatement &stmt = static_cast<const SSAConstantStatement &> (this->Statement);

					switch (stmt.Constant.Type()) {
						case IRConstant::Type_Float_Double: {
							double value = stmt.Constant.Dbl();

							output << "double " << stmt.GetName() << ";";

							output << "{";
							output << "uint64_t __tmp = " << *(uint64_t *)&value << ";";
							output << stmt.GetName() << " = *(double *)&__tmp;";
							output << "}";

							break;
						}

						case IRConstant::Type_Float_Single: {
							float value = stmt.Constant.Flt();

							output << "float " << stmt.GetName() << ";";

							output << "{";
							output << "uint32_t __tmp = " << *(uint32_t *)&value << ";";
							output << stmt.GetName() << " = *(float *)&__tmp;";
							output << "}";

							break;
						}

						case IRConstant::Type_Integer:
						case IRConstant::Type_Vector:
							break;

						/*case IRConstant::Type_Vector: {
							IRConstant splat_value = stmt.Constant.GetVector().GetElement(0);

							output << "auto " << stmt.GetName() << " = wutils::Vector<" << stmt.GetType().GetElementType().GetCType() << ", " << (uint32_t)stmt.GetType().VectorWidth << ">(";
							if (splat_value.Type() == IRConstant::Type_Integer) {
								output << splat_value.Int();
							} else if (splat_value.Type() == IRConstant::Type_Float_Single) {
								output << splat_value.Flt();
							} else if (splat_value.Type() == IRConstant::Type_Float_Double) {
								output << splat_value.Dbl();
							} else {
								throw std::logic_error("Unsupported vector splat element type");
							}

							output << ");";

							/*output << "auto " << stmt.GetName() << " = ";

							if (splat_value.Type() == IRConstant::Type_Integer) {
								output << "emitter.constant_vector_splat(" << splat_value.Int() << ", " << type_for_statement(stmt) << ");";
							} else if (splat_value.Type() == IRConstant::Type_Float_Single) {
								output << "emitter.constant_vector_splat_f((dbt_f32)" << splat_value.Flt() << ", " << type_for_statement(stmt) << ");";
							} else if (splat_value.Type() == IRConstant::Type_Float_Double) {
								output << "emitter.constant_vector_splat_f((dbt_f64)" << splat_value.Dbl() << ", " << type_for_statement(stmt) << ");";
							} else {
								throw std::logic_error("Unsupported vector splat element type");
							}* /

							break;
						}*/

						default:
							throw std::logic_error("Unsupported conversion constant type: " + std::to_string(stmt.Constant.Type()));
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAConstantStatement& stmt = (const SSAConstantStatement&) (this->Statement);

					switch (stmt.Constant.Type()) {
						case IRConstant::Type_Integer:
							output << "auto " << Statement.GetName() << " = emitter.const_" << (Statement.GetType().Signed ? "s" : "u") << (Statement.GetType().SizeInBytes() * 8) << "(" << stmt.Constant.Int() << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;
						default:
							throw std::logic_error("Unsupported constant type: " + std::to_string(stmt.Constant.Type()));
					}

					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAConstantStatement &stmt = static_cast<const SSAConstantStatement &> (this->Statement);

					std::stringstream str;

					switch (stmt.Constant.Type()) {
						case IRConstant::Type_Integer:
							str << "(" << stmt.GetType().GetCType() << ")" << stmt.Constant.Int() << "ULL";
							return str.str();
						case IRConstant::Type_Vector: {
							const auto& vector = stmt.Constant.GetVector();
							const auto& splat_elem = vector.GetElement(0);

							switch (splat_elem.Type()) {
								case IRConstant::Type_Integer:
									return stmt.GetType().GetCType() + "(" + std::to_string(vector.GetElement(0).Int()) + ")";
								case IRConstant::Type_Float_Single:
									return stmt.GetType().GetCType() + "(" + std::to_string(vector.GetElement(0).Flt()) + ")";
								case IRConstant::Type_Float_Double:
									return stmt.GetType().GetCType() + "(" + std::to_string(vector.GetElement(0).Dbl()) + ")";
								default:
									throw std::logic_error("Unsupported vector element type");
							}



							/*str << "{";

							for (int i = 0; i < vector.Width(); i++) {
								if (i > 0) str << ", ";

								const auto& value = vector.GetElement(i);

								if (value.Type() == IRConstant::Type_Integer) {
									str << value.Int();
								} else if (value.Type() == IRConstant::Type_Float_Single) {
									str << value.Flt();
								} else if (value.Type() == IRConstant::Type_Float_Double) {
									str << value.Dbl();
								} else {
									throw std::logic_error("Unsupported constant vector element type");
								}
							}

							str << "}";
							return str.str();*/
						}
						default:
							break;
					}

					return stmt.GetName();
				}
			};

			class SSACastStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSACastStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement &> (this->Statement);

					if (Statement.GetType().VectorWidth > 1) {
						SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());

						output << "auto " << Statement.GetName() << " = " << Statement.GetType().GetCType() << "(" << expr->GetFixedValue() << ");";
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement&> (this->Statement);

					SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());

					output << "auto " << Statement.GetName() << " = ";

					if (Statement.GetType().IsFloating() || Statement.Expr()->GetType().IsFloating()) {
						if (Statement.GetType() == Statement.Expr()->GetType()) {
							output << operand_for_node(*expr) << ";";
						} else {
							output << "emitter.convert(" << operand_for_node(*expr) << ", " << type_for_statement(Statement) << ");";
						}
					} else {
						if (Statement.GetCastType() == SSACastStatement::Cast_VectorSplat) {
							output << "emitter.splat(" << operand_for_node(*expr) << ", " << type_for_statement(Statement) << ");";
						} else if (Statement.GetCastType() == SSACastStatement::Cast_Reinterpret) {
							output << "emitter.reinterpret(" << operand_for_node(*expr) << ", " << type_for_statement(Statement) << ");";
						} else {
							// TODO: Make sure that the specified cast type matches what the sizes are talking about below
							if (Statement.GetType().SizeInBytes() < Statement.Expr()->GetType().SizeInBytes()) {
								output << "emitter.truncate(" << operand_for_node(*expr) << ", " << type_for_statement(Statement) << ");";
							} else if (Statement.GetType().SizeInBytes() == Statement.Expr()->GetType().SizeInBytes()) {
								if (Statement.GetType().Signed != Statement.Expr()->GetType().Signed) {
									// Class a sign-change as a re-interpretation
									output << "emitter.reinterpret(" << operand_for_node(*expr) << ", " << type_for_statement(Statement) << ");";
								} else {
									// HMMMMMMM this probably should be a copy
									output << "(captive::arch::dbt::el::Value *)" << operand_for_node(*expr) << ";";
								}
							} else if (Statement.GetType().SizeInBytes() > Statement.Expr()->GetType().SizeInBytes()) {
								//Otherwise we need to sign extend
								if (Statement.GetType().Signed)
									output << "emitter.sx(" << operand_for_node(*expr) << ", " << type_for_statement(Statement) << ");";
								else
									output << "emitter.zx(" << operand_for_node(*expr) << ", " << type_for_statement(Statement) << ");";
							}
						}
					}


#ifdef REGISTER_ALLOCATION_HINTS
					if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
						output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
					}
#endif
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSACastStatement &Statement = static_cast<const SSACastStatement &> (this->Statement);

					// HACK HACK HACK
					if (Statement.GetType().VectorWidth > 1) {
						//return Factory.GetOrCreate(Statement.Expr())->GetFixedValue();
						if (auto constant = dynamic_cast<const genc::ssa::SSAConstantStatement *>(Statement.Expr())) {
							// If it's a cast of a constant to a vector, then it's a splat
							auto t = Statement.GetType();
							if (t.IsFloating()) {
								return "emitter.constant_vector_splat_f(" + Factory.GetOrCreate(constant)->GetFixedValue() + ", " + type_for_statement(Statement) + ")";
							} else {
								return "emitter.constant_vector_splat(" + Factory.GetOrCreate(constant)->GetFixedValue() + ", " + type_for_statement(Statement) + ")";
							}
						}

						return Statement.GetName();
					}

					std::stringstream str;

					str << "((" << Statement.GetType().GetCType() << ")" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ")";
					return str.str();
				}
			};

			class SSAIfStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAIfStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIfStatement &stmt = static_cast<const SSAIfStatement &> (this->Statement);

					output << "if (" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ") {\n";
					if (stmt.TrueTarget()->IsFixed() == BLOCK_ALWAYS_CONST)
						output << "goto fixed_block_" << stmt.TrueTarget()->GetName() << ";\n";
					else {
						CreateBlock(output, "block_" + stmt.TrueTarget()->GetName());
						output << "emitter.jump(block);";
						if (end_label != "")
							output << "goto " << end_label << ";";
					}
					output << "} else {\n";
					if (stmt.FalseTarget()->IsFixed() == BLOCK_ALWAYS_CONST)
						output << "goto fixed_block_" << stmt.FalseTarget()->GetName() << ";\n";
					else {
						CreateBlock(output, "block_" + stmt.FalseTarget()->GetName());
						output << "emitter.jump(block);";
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
						output << "if (" << expr->GetFixedValue() << ") {";
						CreateBlock(output, "block_" + Statement.TrueTarget()->GetName());
						output << "emitter.jump(block);";
						output << "} else {";
						CreateBlock(output, "block_" + Statement.FalseTarget()->GetName());
						output << "emitter.jump(block);";
						output << "}";
						if (end_label != "")
							output << "goto " << end_label << ";";
					} else {
						output << "{";
						output << "captive::arch::dbt::el::Block *true_target;";
						output << "{";
						CreateBlock(output, "block_" + Statement.TrueTarget()->GetName());
						output << "true_target = block;";
						output << "}";
						output << "captive::arch::dbt::el::Block *false_target;";
						output << "{";
						CreateBlock(output, "block_" + Statement.FalseTarget()->GetName());
						output << "false_target = block;";
						output << "}";

						output << "emitter.branch("
						       << operand_for_node(*expr) << ", "
						       << "true_target, false_target);";

						output << "}";
						if (end_label != "")
							output << "goto " << end_label << ";";
					}

					return true;
				}
			};

			class SSAIntrinsicStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAIntrinsicStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &> (this->Statement);

					const SSANodeWalker *arg0 = nullptr;
					if (Statement.ArgCount() > 0) arg0 = Factory.GetOrCreate(Statement.Args(0));

					switch (Statement.GetID()) {
						case IntrinsicID::PopCount32:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __builtin_popcount(" << arg0->GetFixedValue() << ");";
							break;
						case IntrinsicID::CLZ32:
						case IntrinsicID::CLZ64:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __builtin_clz(" << arg0->GetFixedValue() << ");";
							break;

						case IntrinsicID::GetFeature:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = __get_feature(" << arg0->GetFixedValue() << ");";
							break;

						default:
							throw std::logic_error("Unimplemented fixed intrinsic: " + Statement.GetSignature().GetName());
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
						case IntrinsicID::CLZ32:
						case IntrinsicID::CLZ64:
							output << "auto " << Statement.GetName() << " = emitter.clz(" << operand_for_node(*arg0) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						case IntrinsicID::Trap:
							output << "emitter.raise(emitter.const_u8(0));";
							break;
						case IntrinsicID::TakeException:
							output << "emitter.raise(emitter.const_u8(0));";
							break;
						case IntrinsicID::PopInterrupt:
							output << "emitter.raise(emitter.const_u8(0));";
							break;
						case IntrinsicID::PushInterrupt:
							output << "emitter.raise(emitter.const_u8(0));";
							break;
						case IntrinsicID::HaltCpu:
							output << "emitter.raise(emitter.const_u8(0));";
							break;

						case IntrinsicID::WriteDevice32:
						case IntrinsicID::WriteDevice64:
							output << "if (TRACE) {";
							output << "emitter.trace(captive::arch::dbt::el::TraceEvent::STORE_DEVICE,"
							       << operand_for_node(*arg0)
							       << ", "
							       << operand_for_node(*arg1)
							       << ", "
							       << operand_for_node(*arg2) << ");";
							output << "}";

							output << "emitter.store_device(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ", " << operand_for_node(*arg2) << ");";
							break;

						case IntrinsicID::SetFeature:
							output << "emitter.set_feature(" << arg0->GetFixedValue() << ", " << operand_for_node(*arg1) << ");";
							break;

						case IntrinsicID::ReadPC:
							output << "auto " << Statement.GetName() << " = emitter.load_pc();";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						case IntrinsicID::WritePC:
							output << "emitter.store_pc(" << operand_for_node(*arg0) << ");";
							break;

						case IntrinsicID::ADC8_Flags:
						case IntrinsicID::ADC16_Flags:
						case IntrinsicID::ADC32_Flags:
						case IntrinsicID::ADC64_Flags:
							output << "auto " << Statement.GetName() << " = emitter.adcf(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ", " << operand_for_node(*arg2) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						case IntrinsicID::ADC8:
						case IntrinsicID::ADC16:
						case IntrinsicID::ADC32:
						case IntrinsicID::ADC64:
							output << "auto " << Statement.GetName() << " = emitter.adc(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ", " << operand_for_node(*arg2) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						case IntrinsicID::SBC8_Flags:
						case IntrinsicID::SBC16_Flags:
						case IntrinsicID::SBC32_Flags:
						case IntrinsicID::SBC64_Flags:
							output << "auto " << Statement.GetName() << " = emitter.sbcf(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ", " << operand_for_node(*arg2) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						case IntrinsicID::SBC8:
						case IntrinsicID::SBC16:
						case IntrinsicID::SBC32:
						case IntrinsicID::SBC64:
							output << "auto " << Statement.GetName() << " = emitter.sbc(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ", " << operand_for_node(*arg2) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						case IntrinsicID::SMULH:
							output << "auto " << Statement.GetName() << " = emitter.smulh(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						case IntrinsicID::UMULH:
							output << "auto " << Statement.GetName() << " = emitter.umulh(" << operand_for_node(*arg0) << ", " << operand_for_node(*arg1) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						case IntrinsicID::UpdateZNFlags32:
						case IntrinsicID::UpdateZNFlags64:
							output << "emitter.set_zn(" << operand_for_node(*arg0) << ");";
							break;

						case IntrinsicID::BSwap32:
						case IntrinsicID::BSwap64:
							output << "auto " << Statement.GetName() << " = emitter.bswap(" << operand_for_node(*arg0) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
							if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif
							break;

						default:
							if (Statement.HasValue()) {
								output << "auto " << Statement.GetName() << " = ";
							}

							output << "emitter.call(__captive_" << Statement.GetSignature().GetName();

							for (int i = 0; i < Statement.ArgCount(); i++) {
								const SSANodeWalker *arg = nullptr;
								arg = Factory.GetOrCreate(Statement.Args(i));
								output << ", " << operand_for_node(*arg);
							}

							output << ");\n";

#ifdef REGISTER_ALLOCATION_HINTS
							if (Statement.HasValue() && ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
								output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
							}
#endif

							break;
					}

					return true;
				}

				std::string GetDynamicValue() const
				{
					const SSAIntrinsicStatement &Statement = static_cast<const SSAIntrinsicStatement &> (this->Statement);

					switch (Statement.GetID()) {
						case IntrinsicID::ReadPC:
							return Statement.GetName();

						default:
							return genc::ssa::SSANodeWalker::GetDynamicValue();
					}
				}
			};

			class SSAJumpStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAJumpStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAJumpStatement &stmt = static_cast<const SSAJumpStatement &> (this->Statement);

					if (fully_fixed) {
						if (stmt.Target()->IsFixed() != BLOCK_ALWAYS_CONST) {
							output << "{";
							CreateBlock(output, "block_" + stmt.Target()->GetName());
							output << "emitter.jump(block);";
							output << "}";
							if (end_label != "")
								output << "goto " << end_label << ";";
						} else {
							output << "goto fixed_block_" << stmt.Target()->GetName() << ";\n";
						}
					} else
						EmitDynamicCode(output, end_label, fully_fixed);

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAJumpStatement &Statement = static_cast<const SSAJumpStatement &> (this->Statement);
					output << "{";
					CreateBlock(output, "block_" + Statement.Target()->GetName());
					output << "emitter.jump(block); }";
					if (end_label != "")
						output << "goto " << end_label << ";";
					return true;
				}
			};

			class SSAMemoryReadStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAMemoryReadStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAMemoryReadStatement &Statement = static_cast<const SSAMemoryReadStatement &> (this->Statement);

					SSANodeWalker *address = Factory.GetOrCreate(Statement.Addr());

					output << "auto " << Statement.GetName() << " = emitter.load_memory(" << operand_for_node(*address) << ", " << type_for_symbol(*Statement.Target()) << ");";

					output << "emitter.store_local(" << operand_for_symbol(*Statement.Target()) << ", " << operand_for_stmt(Statement) << ");";

					output << "if (TRACE) {";
					output << "emitter.trace(captive::arch::dbt::el::TraceEvent::LOAD_MEMORY, " << operand_for_node(*address) << ", " << Statement.GetName() << ", emitter.const_u8(" << (uint32_t) Statement.Width << "));\n";
					output << "}";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return EmitFixedCode(output, end_label, fully_fixed);
				}
			};

			class SSAMemoryWriteStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAMemoryWriteStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
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

					output << "if (TRACE) {";
					output << "emitter.trace(captive::arch::dbt::el::TraceEvent::STORE_MEMORY, " << operand_for_node(*address) << ", " << operand_for_node(*value) << ", emitter.const_u8(" << (uint32_t) Statement.Width << "));\n";
					output << "}";

					output << "emitter.store_memory(" << operand_for_node(*address) << ", " << operand_for_node(*value) << ");\n";

					return true;
				}
			};

			class SSAReadStructMemberStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAReadStructMemberStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAReadStructMemberStatement &Statement = static_cast<const SSAReadStructMemberStatement &> (this->Statement);
					output << "auto " << Statement.GetName() << " = emitter.const_";

					if (Statement.GetType().Signed) {
						output << "s";
					} else {
						output << "u";
					}

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

					output << "(insn" << Statement.FormatMembers() << ");";
#ifdef REGISTER_ALLOCATION_HINTS
					if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
						output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
					}
#endif
					return true;
				}

				std::string GetFixedValue() const
				{
					const SSAReadStructMemberStatement &stmt = static_cast<const SSAReadStructMemberStatement &> (this->Statement);
					std::stringstream str;

					if (stmt.MemberNames.at(0) == "IsPredicated") {
						str << "insn.is_predicated";
					} else {
						str << "insn" << stmt.FormatMembers();
					}

					return str.str();
				}
			};

			class SSARegisterStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSARegisterStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitBankedRegisterWrite(util::cppformatstream &output, const SSARegisterStatement &rd) const
				{
					SSANodeWalker *RegNum = Factory.GetOrCreate(rd.RegNum());
					SSANodeWalker *Value = Factory.GetOrCreate(rd.Value());

					const auto &bank_descriptor = rd.Parent->Parent->Arch->GetRegFile().GetBanks()[rd.Bank];
					uint32_t offset = bank_descriptor->GetRegFileOffset();
					uint32_t stride = bank_descriptor->GetRegisterStride();
					uint32_t register_width = bank_descriptor->GetRegisterWidth();

					if (rd.RegNum()->IsFixed()) {
						output << "if (TRACE) {";
						output << "emitter.trace(captive::arch::dbt::el::TraceEvent::STORE_REGISTER,"
						       << "emitter.const_u32((uint32_t)(" << offset << " + (" << stride << " * " << RegNum->GetFixedValue() << "))),"
						       << operand_for_node(*Value) << ","
						       << "emitter.const_u8(" << register_width << ")"
						       << ");";
						output << "}";

						output << "emitter.store_register("
						       << "emitter.const_u32((uint32_t)(" << offset << " + (" << stride << " * " << RegNum->GetFixedValue() << "))),"
						       << operand_for_node(*Value) << ");\n";
						return true;
					} else {
						output << "{";
						output << "auto mulp = emitter.mul(emitter.const_u32(" << stride << "), " << operand_for_node(*RegNum) << ");";
						output << "auto add = emitter.add(emitter.const_u32(" << offset << "), mulp);";
						output << "emitter.store_register(add, " << operand_for_node(*Value) << ");\n";
						output << "}";
					}

					return true;
				}

				bool EmitBankedRegisterRead(util::cppformatstream &output, const SSARegisterStatement &rd) const
				{
					SSANodeWalker *RegNum = Factory.GetOrCreate(rd.RegNum());

					const auto &bank_descriptor = rd.Parent->Parent->Arch->GetRegFile().GetBanks()[rd.Bank];
					uint32_t offset = bank_descriptor->GetRegFileOffset();
					uint32_t stride = bank_descriptor->GetRegisterStride();
					uint32_t register_width = bank_descriptor->GetRegisterWidth();

					if (rd.RegNum()->IsFixed()) {
						output << "auto " << Statement.GetName() << " = emitter.load_register(emitter.const_u32((uint32_t)(" << offset << " + (" << stride << " * " << RegNum->GetFixedValue() << "))), " << type_for_statement(Statement) << ");\n";

						output << "if (TRACE) {";
						output << "emitter.trace(captive::arch::dbt::el::TraceEvent::LOAD_REGISTER, "
						       << "emitter.const_u32((uint32_t)(" << offset << " + (" << stride << " * " << RegNum->GetFixedValue() << "))),"
						       << operand_for_stmt(Statement) << ","
						       << "emitter.const_u8(" << register_width << ")"
						       << ");";
						output << "}";
					} else {
						output << "captive::arch::dbt::el::Value *" << Statement.GetName() << ";";

						output << "{";
						output << "auto mulp = emitter.mul(emitter.const_u32(" << stride << "), " << operand_for_node(*RegNum) << ");";
						output << "auto add = emitter.add(emitter.const_u32(" << offset << "), mulp);";
						output << Statement.GetName() << " = emitter.load_register(add, " << type_for_statement(Statement) << ");\n";
						output << "}";
					}

#ifdef REGISTER_ALLOCATION_HINTS
					if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
						output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
					}
#endif

					return true;
				}

				bool EmitRegisterWrite(util::cppformatstream &output, const SSARegisterStatement &wr) const
				{
					SSANodeWalker *Value = Factory.GetOrCreate(wr.Value());

					const auto &slot = wr.Parent->Parent->Arch->GetRegFile().GetSlots()[wr.Bank];
					uint32_t register_width = slot->GetWidth();
					uint32_t offset = slot->GetRegFileOffset();

					output << "if (TRACE) {";
					output << "emitter.trace(captive::arch::dbt::el::TraceEvent::STORE_REGISTER, emitter.const_u32(" << offset << "), " << operand_for_node(*Value) << ", emitter.const_u8(" << register_width << "));";
					output << "}";

					if (Value->Statement.GetType().SizeInBytes() > register_width) {
						output << "{";
						output << "IRRegId tmp = ctx.alloc_reg(" << slot->GetIRType().GetCaptiveType() << ");";
						output << "ctx.add_instruction(IRInstruction::trunc(" << operand_for_node(*Value) << ", IROperand::vreg(tmp, " << slot->GetIRType().GetCaptiveType() << ")));";
						output << "ctx.add_instruction(IRInstruction::streg(IROperand::vreg(tmp, " << slot->GetIRType().GetCaptiveType() << "), IROperand::const32u(" << offset << ")));";
						output << "}";

						output << "emitter.store_register(emitter.const_u32(" << offset << "), emitter.trunc(" << operand_for_node(*Value) << ", emitter.context().types()." << slot->GetIRType().GetUnifiedType() << "()));";
					} else if (Value->Statement.GetType().SizeInBytes() < register_width) {
						assert(false);
					} else {
						output << "emitter.store_register(emitter.const_u32(" << offset << "), " << operand_for_node(*Value) << ");";
					}

					return true;
				}

				bool EmitRegisterRead(util::cppformatstream &output, const SSARegisterStatement &rd) const
				{
					const auto &slot = rd.Parent->Parent->Arch->GetRegFile().GetSlots()[rd.Bank];
					uint32_t register_width = slot->GetWidth();
					uint32_t offset = slot->GetRegFileOffset();

					output << "auto " << Statement.GetName() << " = ";
					output << "emitter.load_register(emitter.const_u32(" << offset << "), " << type_for_statement(Statement) << ");";

#ifdef REGISTER_ALLOCATION_HINTS
					if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
						output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
					}
#endif

					output << "if (TRACE) {";
					output << "emitter.trace(captive::arch::dbt::el::TraceEvent::LOAD_REGISTER, emitter.const_u32(" << offset << "), " << operand_for_stmt(Statement) << ", emitter.const_u8(" << register_width << "));";
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

			class SSAReturnStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAReturnStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAReturnStatement &Statement = static_cast<const SSAReturnStatement &> (this->Statement);
					if (fully_fixed) {
						if (Statement.Value()) {
							output << "emitter.store_local(__result, " << operand_for_node(*Factory.GetOrCreate(Statement.Value())) << ");";
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

					const IRAction *action = Statement.Parent->Parent->GetAction();
					if (dynamic_cast<const IRHelperAction*> (action)) {
						if (Statement.Value()) {
							output << "emitter.store_local(__result, " << operand_for_node(*Factory.GetOrCreate(Statement.Value())) << ");";
						}
						if (!fully_fixed) {
							assert(false);
						}
						if (end_label != "")
							output << "goto " << end_label << ";";
					} else {
						output << "emitter.jump(__exit_block);";
						if (end_label != "")
							output << "goto " << end_label << ";";
					}
					return true;
				}
			};

			class SSARaiseStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSARaiseStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSARaiseStatement &Statement = static_cast<const SSARaiseStatement &> (this->Statement);
					output << "emitter.raise(emitter.const_u8(0));";
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					return true;
				}
			};

			class SSASelectStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSASelectStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
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

					output << "captive::arch::dbt::el::Value *" << Statement.GetName() << ";";

					if (Statement.Cond()->IsFixed()) {
						output << Statement.GetName() << " = (" << cond->GetFixedValue() << ") ? (" << Statement.TrueVal()->GetName() << ") : (" << Statement.FalseVal()->GetName() << ");";
					} else {

						output << Statement.GetName() << " = emitter.select(" << operand_for_node(*cond) << ", " << operand_for_node(*if_true) << ", " << operand_for_node(*if_false) << ");";

						//output << Statement.GetName() << " = (" << cond->GetFixedValue() << ") ? (" << Statement.TrueVal()->GetName() << ") : (" << Statement.FalseVal()->GetName() << ");";
					}

					return true;
				}
			};

			class SSASwitchStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSASwitchStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
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

						if (target->IsFixed() == BLOCK_ALWAYS_CONST) {
							output << "goto fixed_block_" << target->GetName() << ";\n";
						} else {
							output << "{";
							CreateBlock(output, "block_" + target->GetName());
							output << "emitter.jump(block);";
							output << "}";
						}
						output << "break;";

					}
					output << "default:\n";

					const SSABlock *target = Statement.Default();
					if (target->IsFixed() == BLOCK_ALWAYS_CONST)
						output << "goto fixed_block_" << target->GetName() << ";\n";
					else {
						output << "{";
						CreateBlock(output, "block_" + target->GetName());
						output << "emitter.jump(block);";
						output << "}";
					}

					output << "}";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSASwitchStatement &stmt = static_cast<const SSASwitchStatement &> (this->Statement);

					return false;
				}
			};

			class SSAVariableReadStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAVariableReadStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableReadStatement &Statement = static_cast<const SSAVariableReadStatement &> (this->Statement);
					//return "CV_" + Statement.Target()->GetName();

					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = CV_" << Statement.Target()->GetName() << ";";

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableReadStatement &Statement = static_cast<const SSAVariableReadStatement &> (this->Statement);
					output << "auto " << Statement.GetName() << " = emitter.load_local(DV_" << Statement.Target()->GetName() << ", " << type_for_symbol(*Statement.Target()) << ");";
#ifdef REGISTER_ALLOCATION_HINTS
					if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
						output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
					}
#endif
					return true;
				}
			};

			class SSAVariableWriteStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAVariableWriteStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{
				}

				bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableWriteStatement &Statement = static_cast<const SSAVariableWriteStatement &> (this->Statement);
					SSANodeWalker *expr = Factory.GetOrCreate(Statement.Expr());

					output << "CV_" << Statement.Target()->GetName() << " = " << expr->GetFixedValue() << ";";

					if (Statement.Parent->Parent->HasDynamicDominatedReads(&Statement)) {
						output << "emitter.store_local(" << operand_for_symbol(*Statement.Target()) << ", " << operand_for_node(*expr) << ");";

						/*emitter.const_"
						<< (Statement.Target()->GetType().IsFloating() ? "f" : (Statement.Target()->GetType().Signed ? "s" : "u"))
						<< (uint32_t) (Statement.Target()->GetType().Size() * 8)
						<< "(CV_" << Statement.Target()->GetName() << "));";*/
					}

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
				{
					const SSAVariableWriteStatement &Statement = static_cast<const SSAVariableWriteStatement &> (this->Statement);

					SSANodeWalker *value_node = Factory.GetOrCreate(Statement.Expr());

					if (Statement.Target()->GetType().SizeInBytes() > value_node->Statement.GetType().SizeInBytes()) {
						output << "emitter.store_local(" << operand_for_symbol(*Statement.Target()) << ", emitter.zx(" << operand_for_node(*value_node) << ", " << type_for_symbol(*Statement.Target()) << "));";
					} else if (Statement.Target()->GetType().SizeInBytes() < value_node->Statement.GetType().SizeInBytes()) {
						output << "emitter.store_local(" << operand_for_symbol(*Statement.Target()) << ", emitter.truncate(" << operand_for_node(*value_node) << ", " << type_for_symbol(*Statement.Target()) << "));";
					} else {
						output << "emitter.store_local(" << operand_for_symbol(*Statement.Target()) << ", " << operand_for_node(*value_node) << ");";
					}

					//output << "lir.mov(*lir_variables[lir_idx_" << Statement.GetTarget()->GetName() << "], " << maybe_deref(value_node) << ");";
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
					return "IROperand::vreg(ir_idx_" + Statement.Target()->GetName() + ", " + Statement.Target()->GetType().GetCaptiveType() + ")";
				}
			};

			class SSACallStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSACallStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{

				}

				virtual bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					return true;
				}

				virtual bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const
				{
					const SSACallStatement &Statement = static_cast<const SSACallStatement &> (this->Statement);

					//assert(Statement.GetArgs().size() < 4);

					if (Statement.Target()->GetPrototype().GetIRSignature().HasReturnValue()) {
						output << "auto " << Statement.GetName() << " = ";
					}

					auto callTargetName = Statement.Target()->GetPrototype().GetIRSignature().GetName();

					output << "emitter.call((void *)&__captive_" << callTargetName;

					for (unsigned argIndex = 0; argIndex < Statement.ArgCount(); argIndex++) {
						auto arg = dynamic_cast<const SSAStatement *> (Statement.Arg(argIndex));
						assert(arg);

						SSANodeWalker *argWalker = Factory.GetOrCreate(arg);
						output << ", " << operand_for_node(*argWalker);
					}

					output << ");\n";

					return true;
				}
			};

			class SSABitDepositStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSABitDepositStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{

				}

				bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					/*internal helper uint64 bit_deposit(uint64 src, uint64 bits, uint8 to, uint8 len)
					{
						uint64 mask = (((uint64)1 << len) - 1) << to;
						return ((uint64)src & ~mask) | ((uint64)bits << to);
					}*/

					const SSABitDepositStatement& stmt = static_cast<const SSABitDepositStatement &> (this->Statement);
					SSANodeWalker *expr = Factory.GetOrCreate(stmt.Expr());
					SSANodeWalker *from = Factory.GetOrCreate(stmt.BitFrom());
					SSANodeWalker *to = Factory.GetOrCreate(stmt.BitTo());
					SSANodeWalker *value = Factory.GetOrCreate(stmt.Value());

					output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";
					output << "{";
					output << "uint64_t mask = (((uint64_t)1 << (" << to->GetFixedValue() << " - (" << from->GetFixedValue() << "))) - 1) << (" << to->GetFixedValue() << ");";
					output << "uint64_t value = ((uint64_t)(" << value->GetFixedValue() << ")) << (" << to->GetFixedValue() << ");";
					output << stmt.GetName() << " = ((uint64_t)" << expr->GetFixedValue() << " & ~mask) | (value & mask);";
					output << "}";

					/*if (Statement.Parent->Parent->HasDynamicDominatedReads(&Statement)) {
						output << "ctx.add_instruction(IRInstruction::mov(IROperand::const"
							   << (uint32_t) (Statement.Target()->GetType().Size() * 8)
							   << "(CV_" << Statement.Target()->GetName() << "), "
							   << operand_for_symbol(*Statement.Target()) << "));";
					}*/

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					output << "// BIT DEPOSIT";
					return false;
				}
			};

			class SSABitExtractStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSABitExtractStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{

				}

				bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					/*internal helper uint64 bit_extract(uint64 src, uint8 from, uint8 len) {
						if (len >= 64) {
							return (src >> from);
						} else {
							return (src >> from) & (((uint64)1 << len) - 1);
						}
					}*/

					const SSABitExtractStatement& stmt = static_cast<const SSABitExtractStatement &> (this->Statement);
					SSANodeWalker *expr = Factory.GetOrCreate(stmt.Expr());
					SSANodeWalker *from = Factory.GetOrCreate(stmt.BitFrom());
					SSANodeWalker *to = Factory.GetOrCreate(stmt.BitTo());

					output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";
					output << "{";
					output << "uint64_t mask = (((uint64_t)1 << (" << to->GetFixedValue() << " - (" << from->GetFixedValue() << "))) - 1) << (" << to->GetFixedValue() << ");";
					output << stmt.GetName() << " = ((uint64_t)" << expr->GetFixedValue() << " >> " << from->GetFixedValue() << ") & mask;";
					output << "}";

					/*if (Statement.Parent->Parent->HasDynamicDominatedReads(&Statement)) {
						output << "ctx.add_instruction(IRInstruction::mov(IROperand::const"
							   << (uint32_t) (Statement.Target()->GetType().Size() * 8)
							   << "(CV_" << Statement.Target()->GetName() << "), "
							   << operand_for_symbol(*Statement.Target()) << "));";
					}*/

					return true;
				}

				bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					output << "// BIT EXTRACT";
					return false;
				}
			};

			class SSAVectorInsertElementStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAVectorInsertElementStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{

				}

				bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					const SSAVectorInsertElementStatement& stmt = static_cast<const SSAVectorInsertElementStatement &> (this->Statement);

					SSANodeWalker *base = Factory.GetOrCreate(stmt.Base());
					SSANodeWalker *index = Factory.GetOrCreate(stmt.Index());
					SSANodeWalker *value = Factory.GetOrCreate(stmt.Value());

					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << base->GetFixedValue() << ";";

					output << stmt.GetName() << "[" << index->GetFixedValue() << "] = " << value->GetFixedValue() << ";";
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					const SSAVectorInsertElementStatement& stmt = static_cast<const SSAVectorInsertElementStatement &> (this->Statement);

					SSANodeWalker *base = Factory.GetOrCreate(stmt.Base());
					SSANodeWalker *index = Factory.GetOrCreate(stmt.Index());
					SSANodeWalker *value = Factory.GetOrCreate(stmt.Value());

					output << "auto " << Statement.GetName() << " = emitter.vector_insert(" << operand_for_node(*base) << ", " << operand_for_node(*index) << ", " << operand_for_node(*value) << ");";
					return true;
				}
			};

			class SSAVectorExtractElementStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAVectorExtractElementStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{

				}

				bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					const SSAVectorExtractElementStatement& stmt = static_cast<const SSAVectorExtractElementStatement &> (this->Statement);

					SSANodeWalker *base = Factory.GetOrCreate(stmt.Base());
					SSANodeWalker *index = Factory.GetOrCreate(stmt.Index());

					output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << base->GetFixedValue() << "[" << index->GetFixedValue() << "];";
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					const SSAVectorExtractElementStatement *stmt = dynamic_cast<const SSAVectorExtractElementStatement*> (&Statement);

					const SSANodeWalker *base = Factory.GetOrCreate(stmt->Base());
					const SSANodeWalker *index = Factory.GetOrCreate(stmt->Index());

					output << "auto " << Statement.GetName() << " = emitter.vector_extract(" << operand_for_node(*base) << ", " << operand_for_node(*index) << ");";
					return true;
				}
			};

			class SSADeviceReadStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSADeviceReadStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{

				}

				bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					assert(false);
					return true;
				}

				bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					const SSADeviceReadStatement *stmt = dynamic_cast<const SSADeviceReadStatement*> (&Statement);

					const SSANodeWalker *device_id = Factory.GetOrCreate(stmt->Device());
					const SSANodeWalker *addr = Factory.GetOrCreate(stmt->Address());

					const SSASymbol *target = stmt->Target();

					output << "{";
					output << "auto tmp = emitter.load_device("
					       << operand_for_node(*device_id)
					       << ", "
					       << operand_for_node(*addr)
					       << ", "
					       << type_for_symbol(*target)
					       << ");";

					output << "emitter.store_local(" << operand_for_symbol(*target) << ", tmp);";

					output << "if (TRACE) {";
					output << "emitter.trace(captive::arch::dbt::el::TraceEvent::LOAD_DEVICE,"
					       << operand_for_node(*device_id)
					       << ", "
					       << operand_for_node(*addr)
					       << ", tmp);";
					output << "}";
					output << "}";

					return true;
				}
			};

			class SSAUnaryArithmeticStatementWalker : public genc::ssa::SSANodeWalker
			{
			public:

				SSAUnaryArithmeticStatementWalker(const SSAStatement &stmt, SSAWalkerFactory *_fact) : SSANodeWalker(stmt, *_fact)
				{

				}

				bool EmitFixedCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					const SSAUnaryArithmeticStatement &Statement = static_cast<const SSAUnaryArithmeticStatement &> (this->Statement);
					switch (Statement.Type) {
						case SSAUnaryOperator::OP_NEGATIVE:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = -" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ";";
							break;
						case SSAUnaryOperator::OP_NEGATE:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = !" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ";";
							break;
						case SSAUnaryOperator::OP_COMPLEMENT:
							output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = ~" << Factory.GetOrCreate(Statement.Expr())->GetFixedValue() << ";";
							break;
						default:
							assert(false);
					}

					return false;
				}

				bool EmitDynamicCode(util::cppformatstream& output, std::string end_label, bool fully_fixed) const override
				{
					const SSAUnaryArithmeticStatement &stmt = static_cast<const SSAUnaryArithmeticStatement &> (this->Statement);

					switch (stmt.Type) {
						case SSAUnaryOperator::OP_NEGATIVE:
							output << "auto " << stmt.GetName() << " = emitter.neg(" << operand_for_stmt(*stmt.Expr()) << ");";
							break;

						case SSAUnaryOperator::OP_COMPLEMENT:
							output << "auto " << stmt.GetName() << " = emitter.bitwise_not(" << operand_for_stmt(*stmt.Expr()) << ");";
							break;

						case SSAUnaryOperator::OP_NEGATE: {
							if (stmt.GetType().SizeInBytes() == 1) {
								output << "auto " << stmt.GetName() << " = emitter.cmp_eq(" << operand_for_node(*Factory.GetOrCreate(stmt.Expr())) << ", emitter.const_u8(0));";
							} else {
								output << "captive::arch::dbt::el::Value *" << stmt.GetName() << ";";
								output << "{";
								output << "auto tmp = emitter.cmp_eq(" << operand_for_node(*Factory.GetOrCreate(stmt.Expr())) << ", emitter.const_u";
								output << (uint32_t) (stmt.GetType().SizeInBytes() * 8) << "(0));";
								output << stmt.GetName() << " = emitter.zx(tmp, " << type_for_statement(stmt) << ");";
								output << "}";
							}

							break;
						}

						default:
							assert(false);
					}

#ifdef REGISTER_ALLOCATION_HINTS
					if (((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().IsStatementAllocated(&Statement)) {
						output << Statement.GetName() << "->allocate(" << ((JITv2NodeWalkerFactory&) Factory).RegisterAllocation().GetRegisterForStatement(&Statement) << ");";
					}
#endif
					return false;
				}
			};
		}
	}
}

#define STMT_IS(x) dynamic_cast<const genc::ssa::x *>(stmt)
#define HANDLE(x) if (STMT_IS(x)) return new gensim::generator::captive_v2::x##Walker(*stmt, this)

SSANodeWalker *JITv2NodeWalkerFactory::Create(const SSAStatement *stmt)
{
	assert(stmt != NULL);

	HANDLE(SSABinaryArithmeticStatement);
	HANDLE(SSABitDepositStatement);
	HANDLE(SSABitExtractStatement);
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
	HANDLE(SSARaiseStatement);
	HANDLE(SSAReturnStatement);
	HANDLE(SSASelectStatement);
	HANDLE(SSASwitchStatement);
	HANDLE(SSAVariableReadStatement);
	HANDLE(SSAVariableWriteStatement);
	HANDLE(SSAVectorExtractElementStatement);
	HANDLE(SSAVectorInsertElementStatement);
	HANDLE(SSACallStatement);

	fprintf(stderr, "ERROR: UNHANDLED STMT TYPE: %s\n", typeid(stmt).name());
	assert(false && "Unrecognized statement type");

	return NULL;
}
