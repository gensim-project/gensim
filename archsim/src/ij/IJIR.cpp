#include "ij/IJIR.h"
#include "ij/ir/IRValue.h"
#include "ij/ir/IRInstruction.h"
#include "ij/ir/IRBuilder.h"

using namespace archsim::ij;

IJIR::IJIR(uint8_t nr_params) : next_name(0)
{
	entry_block = CreateBlock("entry");

	for (int i = 0; i < nr_params; i++) {
		params.push_back(AllocateGlobal(4));
	}

	builder = new ir::IRBuilder(*this, *entry_block);
}

IJIR::~IJIR()
{
	delete builder;

	for (auto block : blocks) {
		delete block;
	}

	for (auto val : global_values) {
		delete val;
	}
}

ir::IRVariable* IJIR::AllocateGlobal(uint8_t size, std::string name)
{
	ir::IRVariable *val = new ir::IRVariable(*this, size, name);
	global_values.push_back(val);

	return val;
}

ir::IRConstant* IJIR::GetConstantInt(uint8_t size, uint64_t value)
{
	ir::IRConstant *cnst = new ir::IRConstant(*this, size, value);
	global_values.push_back(cnst);

	return cnst;
}

IJIRBlock* IJIR::CreateBlock(std::string name)
{
	IJIRBlock* block = new IJIRBlock(*this, name);
	blocks.push_back(block);

	return block;
}

ir::IRValue* IJIR::GetParameterValue(uint8_t idx)
{
	if (idx >= params.size())
		return NULL;

	return params[idx];
}

void IJIR::Dump(std::ostream& stream) const
{
	for (auto block : blocks) {
		stream << "block: " << block->GetName() << std::endl;
		block->Dump(stream);
		stream << std::endl;
	}
}

IJIRBlock::IJIRBlock(IJIR& ir, std::string name) : ir(ir), name(name)
{

}

IJIRBlock::~IJIRBlock()
{
	for (auto insn : instructions) {
		delete insn;
	}

	for (auto val : local_values) {
		delete val;
	}
}

ir::IRVariable* IJIRBlock::AllocateLocal(uint8_t size, std::string name)
{
	ir::IRVariable *val = new ir::IRVariable(ir, size, name);
	val->SetScope(this);

	local_values.push_back(val);
	return val;
}

ir::IRInstruction* IJIRBlock::AddInstruction(ir::IRInstruction* insn)
{
	instructions.push_back(insn);
	return insn;
}

void IJIRBlock::Dump(std::ostream& stream) const
{
	for (auto insn : instructions) {
		stream << "  ";
		insn->Dump(stream);
		stream << std::endl;
	}
}
