#include "ij/ir/IRInstruction.h"
#include "ij/ir/IRValue.h"

using namespace archsim::ij::ir;

IRInstruction::TypeIDBase IRRetInstruction::Type;
IRInstruction::TypeIDBase IRArithmeticInstruction::Type;

IRInstruction::~IRInstruction()
{

}

void IRInstruction::EnsureOperandsAllocated()
{
	for (auto oper : operands) {
		oper->EnsureAllocated();
	}
}

void IRInstruction::Dump(std::ostream& stream) const
{
	stream << "[unknown instruction]";
	DumpOperands(stream);
}

void IRInstruction::DumpOperands(std::ostream& stream) const
{
	stream << " ";

	bool first = true;
	for (auto op : operands) {
		if (first)
			first = false;
		else
			stream << ", ";
		op->Dump(stream);
	}
}

void IRRetInstruction::Dump(std::ostream& stream) const
{
	stream << "ret";
	DumpOperands(stream);
}

void IRArithmeticInstruction::Dump(std::ostream& stream) const
{
	switch (op) {
		case Add:
			stream << "add";
			break;
		case Subtract:
			stream << "sub";
			break;
		case Multiply:
			stream << "mul";
			break;
		case Divide:
			stream << "div";
			break;
		case Modulo:
			stream << "mod";
			break;
		default:
			stream << "[unknown arithmetic instruction]";
			break;
	}

	DumpOperands(stream);
}
