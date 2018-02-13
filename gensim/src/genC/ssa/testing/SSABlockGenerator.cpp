/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "define.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/testing/SSAActionGenerator.h"
#include "genC/ssa/testing/SSABlockGenerator.h"
#include "genC/ssa/testing/MachineState.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "arch/ArchDescription.h"




using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa;

SSABlockGenerator::SSABlockGenerator(random_t& random, unsigned stmts, SSAFormAction* action, const std::vector<SSABlock*>& possible_targets, SSAContext& context) : _random(random), _stmts_per_block(stmts), _action(action), _possible_targets(possible_targets), _context(context)
{
	FillGenerators();
}

void SSABlockGenerator::GenerateStatement(SSABlock *parent)
{
	SSAStatement *new_statement = _generators[_random() % _generators.size()](parent, this);

	// We might have tried to create a statement but the statement generator
	// failed for some reason (e.g. not enough existing values)
	if(new_statement == nullptr) {
		return;
	}

	if(new_statement->HasValue()) {
		_values.push_back(new_statement);
	}
}

void SSABlockGenerator::GenerateControlFlow(SSABlock* parent)
{
	bool success = false;
	while(!success) {
		SSAStatement *stmt = _generators_controlflow.at(_random() % _generators_controlflow.size())(parent, this);
		if(stmt != nullptr) {
			success = true;
		}
	}
}

SSABlock* SSABlockGenerator::Generate()
{
	SSABlock *block = new SSABlock(_context, *_action);

	int stmts = _random() % _stmts_per_block;
	while(block->GetStatements().size() < stmts) {
		GenerateStatement(block);
	}

	// create control flow out of the block
	GenerateControlFlow(block);

	return block;
}

SSAStatement *generate_const(SSABlock *block, SSABlockGenerator *gen)
{
	using gensim::genc::IRType;
	using gensim::genc::IRTypes;

	IRType the_type = gen->RandomType();


	uint64_t the_value = gen->Random();
	the_value &= (1ULL << (8*the_type.Size()))-1;

	return new SSAConstantStatement(block, gensim::genc::IRConstant::Integer(the_value), the_type);
}

SSAStatement *generate_binary_stmt(SSABlock *block, SSABlockGenerator *gen)
{
	if(gen->Values().size() < 2) {
		return nullptr;
	}

	// get lhs and rhs
	SSAStatement *lhs = gen->Values().at(gen->Random() % gen->Values().size());
	SSAStatement *rhs = gen->Values().at(gen->Random() % gen->Values().size());

	using namespace gensim::genc::BinaryOperator;
	const EBinaryOperator operators[] = {
		Add,
		Subtract,
		Multiply,
		Divide,
		Modulo,
		ShiftLeft,
		ShiftRight,
		Bitwise_Or,
		Bitwise_And,
		Bitwise_XOR,
		SignedShiftRight,

		Logical_Or,
		Logical_And,

		Equality,
		Inequality,
		LessThan,
		GreaterThan,
		LessThanEqual,
		GreaterThanEqual
	};

	EBinaryOperator op = operators[(gen->Random() % (sizeof(operators)/sizeof(operators[0])))];

	// if types don't match, then cast types
	if(lhs->GetType() != rhs->GetType()) {
		gensim::genc::IRType rtype = gensim::genc::IRType::Resolve(op, lhs->GetType(), rhs->GetType());

		if(lhs->GetType() != rtype) {
			lhs = new SSACastStatement(block, rtype, lhs);
		}

		if(rhs->GetType() != rtype) {
			rhs = new SSACastStatement(block, rtype, rhs);
		}
	}

	return new SSABinaryArithmeticStatement(block, lhs, rhs, op);
}

SSAStatement *generate_register_read(SSABlock *block, SSABlockGenerator *gen)
{
	bool banked = gen->Random() % 2;

	if(banked) {
		// We need at least one existing value to use as a register bank index
		if(gen->Values().empty()) {
			return nullptr;
		}

		int bank_idx = gen->Random() % gen->Arch()->GetRegFile().GetBanks().size();
		auto &bank = gen->Arch()->GetRegFile().GetBanks().at(bank_idx);

		SSAStatement *regnumexpr = gen->RandomValue();

		return SSARegisterStatement::CreateBankedRead(block, bank_idx, regnumexpr);
	} else {
		int slot_idx = gen->Random() % gen->Arch()->GetRegFile().GetSlots().size();
		auto &slot = gen->Arch()->GetRegFile().GetSlots().at(slot_idx);

		return SSARegisterStatement::CreateRead(block, slot_idx);
	}
}

SSAStatement *generate_register_write(SSABlock *block, SSABlockGenerator *gen)
{
	bool banked = gen->Random() % 2;

	if(banked) {
		// we need at least two existing values to use as register bank index and value to be written
		if(gen->Values().size() < 2) {
			return nullptr;
		}
		int bank_idx = gen->Random() % gen->Arch()->GetRegFile().GetBanks().size();
		auto &bank = gen->Arch()->GetRegFile().GetBanks().at(bank_idx);

		SSAStatement *regnumexpr = gen->RandomValue();
		SSAStatement *valueexpr = gen->RandomValue();

		// cast value
		valueexpr = new SSACastStatement(block, bank->GetRegisterIRType(), valueexpr);

		return SSARegisterStatement::CreateBankedWrite(block, bank_idx, regnumexpr, valueexpr);
	} else {
		// we need at least one existing value to write to the register
		if(gen->Values().empty()) {
			return nullptr;
		}

		int slot_idx = gen->Random() % gen->Arch()->GetRegFile().GetSlots().size();
		auto &slot = gen->Arch()->GetRegFile().GetSlots().at(slot_idx);

		SSAStatement *valueexpr = gen->RandomValue();

		valueexpr = new SSACastStatement(block, slot->GetIRType(), valueexpr);
		return SSARegisterStatement::CreateWrite(block, slot_idx, valueexpr);
	}
}

SSAStatement *generate_variable_write(SSABlock *block, SSABlockGenerator *gen)
{
	SSASymbol *target = gen->RandomSymbol();
	SSAStatement *expression = gen->RandomValue();

	if(expression == nullptr) {
		return nullptr;
	}

	if(expression->GetType() != target->GetType()) {
		expression = new SSACastStatement(block, target->GetType(), expression);
	}
	return new SSAVariableWriteStatement(block, target, expression);
}

SSAStatement *generate_variable_read(SSABlock *block, SSABlockGenerator *gen)
{
	SSASymbol *target = gen->RandomSymbol();

	return new SSAVariableReadStatement(block, target);
}

SSAStatement *generate_memory_read(SSABlock *block, SSABlockGenerator *gen)
{
	SSAStatement *addr = gen->RandomValue();
	SSASymbol *target = gen->RandomSymbol();

	if(addr == nullptr || target == nullptr) {
		return nullptr;
	}

	int width = target->GetType().ElementSize();
	bool sign = gen->Random() % 2;

	return &SSAMemoryReadStatement::CreateRead(block, addr, target, width, sign, false);
}

SSAStatement *generate_memory_write(SSABlock *block, SSABlockGenerator *gen)
{
	SSAStatement *addr = gen->RandomValue();
	SSAStatement *value = gen->RandomValue();

	if(addr == nullptr || value == nullptr) {
		return nullptr;
	}

	int width = 1 << (gen->Random() % 3);

	return &SSAMemoryWriteStatement::CreateWrite(block, addr, value, width);
}

SSAControlFlowStatement *generate_return(SSABlock *block, SSABlockGenerator *gen)
{
	SSAStatement *return_value = nullptr;
	const SSAType &rtype = block->Parent->GetPrototype().ReturnType();
	if(rtype != gensim::genc::IRTypes::Void) {
		return_value = gen->RandomValue();
		if(return_value == nullptr) {
			return_value = new SSAConstantStatement(block, gensim::genc::IRConstant::Integer(0), rtype);
		}

		if(return_value->GetType() != rtype) {
			return_value = new SSACastStatement(block, rtype, return_value);
		}
	}

	return new SSAReturnStatement(block, return_value);
}

SSAControlFlowStatement *generate_jump(SSABlock *block, SSABlockGenerator *gen)
{
	if(gen->Targets().empty()) {
		return nullptr;
	}
	return new SSAJumpStatement(block, *gen->RandomTarget());
}

SSAControlFlowStatement *generate_if(SSABlock *block, SSABlockGenerator *gen)
{
	if(gen->Values().empty()) {
		return nullptr;
	}
	if(gen->Targets().empty()) {
		return nullptr;
	}
	return new SSAIfStatement(block, gen->RandomValue(), *gen->RandomTarget(), *gen->RandomTarget());
}

SSAStatement *generate_call(SSABlock *block, SSABlockGenerator *gen)
{
	if(gen->Values().empty()) {
		return nullptr;
	}

	// pick a callee
	SSAFormAction *callee = gen->RandomCallee();
	std::vector<SSAValue*> args;

	for(auto i : callee->GetPrototype().GetIRSignature().GetParams()) {
		if(i.GetType().Reference) {
			UNIMPLEMENTED;
		}

		SSAStatement *value = gen->RandomValue();
		if(value->GetType() != i.GetType()) {
			value = new SSACastStatement(block, i.GetType(), value);
		}
		args.push_back(value);
	}

	return new SSACallStatement(block, callee, args);
}

SSAStatement *generate_select_statement(SSABlock *block, SSABlockGenerator *gen)
{
	if(gen->Values().empty()) {
		return nullptr;
	}

	return new SSASelectStatement(block, gen->RandomValue(), gen->RandomValue(), gen->RandomValue());
}

gensim::genc::IRType SSABlockGenerator::RandomType()
{

	gensim::genc::IRType the_type;
	int type_width = 1 << (Random() % 4);
	bool is_signed = Random() % 2;

	switch(type_width) {
		case 1:
			if(is_signed) {
				the_type = IRTypes::Int8;
			} else {
				the_type = IRTypes::UInt8;
			}
			break;
		case 2:
			if(is_signed) {
				the_type = IRTypes::Int16;
			} else {
				the_type = IRTypes::UInt16;
			}
			break;
		case 4:
			if(is_signed) {
				the_type = IRTypes::Int32;
			} else {
				the_type = IRTypes::UInt32;
			}
			break;
		case 8:
			if(is_signed) {
				the_type = IRTypes::Int64;
			} else {
				the_type = IRTypes::UInt64;
			}
			break;
		default:
			throw std::logic_error("");
	}

	return the_type;
}

SSASymbol* SSABlockGenerator::RandomSymbol()
{
	// 50-50 to create new symbol or return an existing one
	bool r = Random() % 2;
	if(r || _symbols.empty()) {
		IRType type = RandomType();
		std::string name = "sym_" + std::to_string(_action->Symbols().size());

		SSASymbol *symbol = new SSASymbol(_context, name, type, Symbol_Local);
		_symbols.push_back(symbol);
		_action->AddSymbol(symbol);
		return symbol;
	} else {
		int i = Random() % _action->Symbols().size();
		auto it = _action->Symbols().begin();
		while(i--)it++;
		return *it;
	}

}

SSAFormAction* SSABlockGenerator::RandomCallee()
{
	//strongly bias calling an existing target
	bool r = Random() % 10;
	if(r == 0 || _callees.empty()) {
		SSAActionGenerator gen (_random, 3, 3, false);
		SSAFormAction *callee = gen.Generate(_context, "action"+std::to_string(_callees.size()));
		_callees.push_back(callee);
		return callee;
	} else {
		return _callees.at(Random() % _callees.size());
	}

}

void SSABlockGenerator::FillGenerators()
{
	_generators.push_back(generate_const);
	_generators.push_back(generate_binary_stmt);
	_generators.push_back(generate_register_read);
	_generators.push_back(generate_register_write);
	_generators.push_back(generate_variable_write);
	_generators.push_back(generate_variable_read);
	_generators.push_back(generate_memory_write);
	_generators.push_back(generate_memory_read);

	_generators.push_back(generate_select_statement);

	_generators.push_back(generate_call);

	_generators_controlflow.push_back(generate_return);
	_generators_controlflow.push_back(generate_jump);
	_generators_controlflow.push_back(generate_if);
}
