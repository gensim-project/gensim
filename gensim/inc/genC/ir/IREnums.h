/*
 * File:   IREnums.h
 * Author: s0803652
 *
 * Created on August 10, 2012, 3:12 PM
 */

#ifndef IRENUMS_H
#define IRENUMS_H

#include <stdexcept>

namespace gensim
{
	namespace genc
	{

		namespace IRPlainOldDataType
		{
			enum IRPlainOldDataType {
				VOID = 0,
				INT1 = 1,
				INT8 = 2,
				INT16 = 3,
				INT32 = 4,
				INT64 = 5,
				INT128 = 6,
				FLOAT = 7,
				DOUBLE = 8,
				LONG_DOUBLE = 9
			};
		}

		namespace SSAUnaryOperator
		{
			enum ESSAUnaryOperator {
				OP_NEGATE,
				OP_COMPLEMENT,
				OP_NEGATIVE
			};

			inline std::string PrettyPrint(ESSAUnaryOperator op)
			{
				switch(op) {
					case OP_NEGATE:
						return "!";
					case OP_COMPLEMENT:
						return "~";
					case OP_NEGATIVE:
						return "-";
					default:
						throw std::logic_error("");
				}
			}
		}

		namespace IRUnaryOperator
		{
			enum EIRUnaryOperator {
				Dereference,
				Positive,
				Negative,
				Complement,
				Negate,
				Preincrement,
				Predecrement,
				Postincrement,
				Postdecrement,
				Member,
				Ptr,
				Index,
				TakeAddress,
				BitSequence
			};
		}

		namespace BinaryOperator
		{

			// CAREFUL! The order of these matters!
			enum EBinaryOperator {
				Add,
				Subtract,
				Multiply,
				Divide,
				Modulo,
				ShiftLeft,
				ShiftRight,
				RotateLeft,
				RotateRight,
				Bitwise_Or,
				Bitwise_And,
				Bitwise_XOR,
				SignedShiftRight,

				END_OF_NORMAL_OPERATORS,

				Logical_Or,
				Logical_And,

				END_OF_LOGICAL_OPERATORS,

				Equality,
				Inequality,
				LessThan,
				GreaterThan,
				LessThanEqual,
				GreaterThanEqual,

				END_OF_COMPARISON_OPERATORS,

				START_OF_ASSIGNMENT_OPERATORS,
				Set,
				AddAndSet,
				SubAndSet,
				MulAndSet,
				DivAndSet,
				ModAndSet,
				LSAndSet,
				RSAndSet,
				OrAndSet,
				AndAndSet,
				XOrAndSet,
				END_OF_ASSIGNMENT_OPERATORS
			};

			static EBinaryOperator SetOpToNonSetOp(EBinaryOperator op)
			{
				switch(op) {
					case AddAndSet:
						return Add;
					case SubAndSet:
						return Subtract;
					case MulAndSet:
						return Multiply;
					case DivAndSet:
						return Divide;
					case ModAndSet:
						return Modulo;
					case LSAndSet:
						return ShiftLeft;
					case RSAndSet:
						return ShiftRight;
					case OrAndSet:
						return Bitwise_Or;
					case AndAndSet:
						return Bitwise_And;
					case XOrAndSet:
						return Bitwise_XOR;

					default:
						throw std::logic_error("Unsupported SetOp");
				}
			}

			std::string PrettyPrintOperator(BinaryOperator::EBinaryOperator);
			inline BinaryOperator::EBinaryOperator ReverseOperator(BinaryOperator::EBinaryOperator op)
			{
				switch (op) {
					case Equality:
					case Inequality:
						return op;
					case LessThan:
						return GreaterThanEqual;
					case LessThanEqual:
						return GreaterThan;
					case GreaterThan:
						return LessThanEqual;
					case GreaterThanEqual:
						return LessThan;
					default:
						assert(!"Not a reversible operator");
				}
			}
			inline bool IsComparative(BinaryOperator::EBinaryOperator op)
			{
				switch (op) {
					case Equality:
					case Inequality:
					case LessThan:
					case LessThanEqual:
					case GreaterThan:
					case GreaterThanEqual:
						return true;
					default:
						return false;
				}
			}
			inline bool IsAssignment(BinaryOperator::EBinaryOperator op)
			{
				return (op > START_OF_ASSIGNMENT_OPERATORS) && (op < END_OF_ASSIGNMENT_OPERATORS);
			}
		}
	}
}

#endif /* IRENUMS_H */
