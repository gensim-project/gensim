/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/testing/SSAActionGenerator.h"
#include "genC/ssa/testing/SSABlockGenerator.h"
#include "genC/ssa/statement/SSAJumpStatement.h"
#include "genC/ssa/statement/SSAConstantStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::testing;

SSAActionGenerator::SSAActionGenerator(random_t& random, unsigned max_blocks, unsigned stmts_per_block, bool allow_loops) : _random(random), _max_blocks(max_blocks), _stmts_per_block(stmts_per_block), _allow_loops(allow_loops)
{

}

static gensim::genc::IRType RandomType(SSAActionGenerator::random_t &Random)
{
	using namespace gensim::genc;
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

SSAFormAction* SSAActionGenerator::Generate(SSAContext& context, const std::string &action_name)
{
	IRSignature::param_type_list_t params;
	int num_params = _random() % 4;
	for(int i = 0; i < num_params; ++i) {
		params.push_back(IRParam("param"+std::to_string(i), RandomType(_random)));
	}

	IRSignature sig (action_name, RandomType(_random), params);
	SSAFormAction *action = new SSAFormAction(context, SSAActionPrototype(sig));
	action->EntryBlock = new SSABlock(context, *action);
	action->Arch = &context.GetArchDescription();

	std::vector<SSABlock*> blocks;

	// generate a 'final' block
	SSABlock *final_block = generateBlock(context, true, action, blocks);
	blocks.push_back(final_block);

	// generate intermediate blocks
	while(blocks.size() < _max_blocks) {
		blocks.push_back(generateBlock(context, false, action, blocks));
	}

	auto entryblock = action->EntryBlock;
	// fill all symbols
	for(auto i : action->Symbols()) {
		uint64_t value = _random();
		IRConstant cvalue = IRType::Cast(IRConstant::Integer(value), IRTypes::UInt64, i->GetType());
		SSAConstantStatement *constant = new SSAConstantStatement(entryblock, cvalue, i->GetType());
		new SSAVariableWriteStatement(entryblock, i, constant);
	}

	// link entry block to a random intermediate block
	new SSAJumpStatement(action->EntryBlock, *blocks.at(_random() % blocks.size()));

	context.AddAction(action);

	return action;
}

SSABlock* SSAActionGenerator::generateBlock(SSAContext& context, bool must_return, SSAFormAction* action, std::vector<SSABlock*>& possible_target_blocks)
{
	SSABlockGenerator generator(_random, _stmts_per_block, action, possible_target_blocks, context);
	return generator.Generate();
}
