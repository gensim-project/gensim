#include "ij/ir/IRValue.h"
#include "ij/IJIR.h"

using namespace archsim::ij::ir;

IRValue::IRValue(IJIR& ir, uint8_t size, std::string name) : ir(ir), size(size), name(name), alloc_class(None), alloc_data(0)
{
	if (this->name == "") {
		this->name = ir.GetUniqueName();
	}
}

IRValue::~IRValue()
{

}

void IRValue::Dump(std::ostream& stream) const
{
	stream << "[unknown value]";
}

IRConstant::IRConstant(IJIR& ir, uint8_t size, uint64_t value, std::string name) : IRValue(ir, size, name), value(value)
{
	Allocate(IRValue::Constant, value);
}

void IRConstant::Dump(std::ostream& stream) const
{
	stream << std::to_string(value);
}

IRVariable::IRVariable(IJIR& ir, uint8_t size, std::string name) : IRValue(ir, size, name)
{

}

void IRVariable::Dump(std::ostream& stream) const
{
	stream << "%" << GetName();
}
