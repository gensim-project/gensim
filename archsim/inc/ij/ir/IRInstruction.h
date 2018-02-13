/*
 * File:   IRInstruction.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 15:27
 */

#ifndef IRINSTRUCTION_H
#define	IRINSTRUCTION_H

#include "define.h"

#include <ostream>
#include <vector>

namespace archsim
{
	namespace ij
	{
		class IJIRBlock;

		namespace ir
		{
			class IRValue;

			class IRInstruction
			{
			public:
				enum OperandDirection {
					Input,
					Output,
					Both
				};

				typedef char TypeIDBase;
				typedef TypeIDBase *TypeID;

				IRInstruction(IJIRBlock& parent) : parent(parent) { }
				virtual ~IRInstruction();

				virtual void Dump(std::ostream& stream) const;

				IJIRBlock& GetParent() const
				{
					return parent;
				}

				const std::vector<IRValue *>& GetAllOperands() const
				{
					return operands;
				}

				virtual TypeID GetTypeID() const = 0;

				void EnsureOperandsAllocated();

				template<typename T> bool is() const
				{
					return GetTypeID() == &T::Type;
				}
				template<typename T> T* as() const
				{
					if (is<T>()) return (T*)this;
					else return NULL;
				}
				template<typename T> T& cast() const
				{
					assert(is<T>());
					return (T&)*this;
				}

			protected:
				void AddOperand(IRValue *value, OperandDirection direction)
				{
					operands.push_back(value);
					operand_directions.push_back(direction);
				}

				IRValue *GetOperand(uint8_t index) const
				{
					assert(index < operands.size());
					return operands[index];
				}

				int GetOperandCount() const
				{
					return operands.size();
				}

				void DumpOperands(std::ostream& stream) const;

			private:
				std::vector<IRValue *> operands;
				std::vector<OperandDirection> operand_directions;
				IJIRBlock& parent;
			};

			class IRTerminatorInstruction : public IRInstruction
			{
			public:
				IRTerminatorInstruction(IJIRBlock& parent) : IRInstruction(parent) { }
			};

			class IRRetInstruction : public IRTerminatorInstruction
			{
			public:
				static IRInstruction::TypeIDBase Type;

				IRRetInstruction(IJIRBlock& parent, IRValue *val) : IRTerminatorInstruction(parent)
				{
					if (val) {
						AddOperand(val, IRInstruction::Input);
					}
				}

				void Dump(std::ostream& stream) const override;

				IRInstruction::TypeID GetTypeID() const
				{
					return &IRRetInstruction::Type;
				}

				inline bool HasValue() const
				{
					return GetOperandCount() > 0;
				}
				inline IRValue *GetValue() const
				{
					return GetOperand(0);
				}
			};

			class IRBinaryInstruction : public IRInstruction
			{
			public:
				IRBinaryInstruction(IJIRBlock& parent, IRValue *source, IRValue *destination) : IRInstruction(parent)
				{
					AddOperand(source, IRInstruction::Input);
					AddOperand(destination, IRInstruction::Both);
				}

				IRValue& GetSource() const
				{
					return *GetOperand(0);
				}
				IRValue& GetDestination() const
				{
					return *GetOperand(1);
				}
			};

			class IRUnaryInstruction : public IRInstruction
			{
			public:
				enum Operation {
					Negation,
					Inversion,
				};

				IRUnaryInstruction(IJIRBlock& parent, IRValue *destination, Operation op) : IRInstruction(parent), op(op)
				{
					AddOperand(destination, IRInstruction::Both);
				}

			private:
				Operation op;
			};

			class IRArithmeticInstruction : public IRBinaryInstruction
			{
			public:
				static char Type;

				enum Operation {
					Add,
					Subtract,
					Multiply,
					Divide,
					Modulo
				};

				IRArithmeticInstruction(IJIRBlock& parent, IRValue *source, IRValue *destination, Operation op) : IRBinaryInstruction(parent, source, destination), op(op) { }

				virtual void Dump(std::ostream& stream) const;

				inline Operation GetOperation() const
				{
					return op;
				}

				IRInstruction::TypeID GetTypeID() const
				{
					return &IRArithmeticInstruction::Type;
				}

			private:
				Operation op;
			};

			class IRBitManipInstruction : public IRBinaryInstruction
			{
			public:
				static char Type;

				enum Operation {
					LogicalShiftRight,
					ArithmeticShiftRight,
					LogicalShiftLeft,
					RotateLeft,
					RotateRight,
				};

				IRBitManipInstruction(IJIRBlock& parent, IRValue *source, IRValue *destination, Operation op) : IRBinaryInstruction(parent, source, destination) { }

				IRInstruction::TypeID GetTypeID() const
				{
					return &IRBitManipInstruction::Type;
				}
			};

			class IRBitwiseInstruction : public IRBinaryInstruction
			{
			public:
				static char Type;

				enum Operation {
					BitwiseAnd,
					BitwiseOr,
					BitwiseXor,
				};

				IRBitwiseInstruction(IJIRBlock& parent, IRValue *source, IRValue *destination, Operation op) : IRBinaryInstruction(parent, source, destination) { }

				IRInstruction::TypeID GetTypeID() const
				{
					return &IRBitwiseInstruction::Type;
				}
			};

			class IRCastInstruction : public IRBinaryInstruction
			{
			public:
				static char Type;

				enum Operation {
					ZeroExtend,
					SignExtend,
					Truncate,
				};

				IRCastInstruction(IJIRBlock& parent, IRValue *source, IRValue *destination, Operation op) : IRBinaryInstruction(parent, source, destination) { }

				IRInstruction::TypeID GetTypeID() const
				{
					return &IRCastInstruction::Type;
				}
			};
		}
	}
}

#endif	/* IRINSTRUCTION_H */

