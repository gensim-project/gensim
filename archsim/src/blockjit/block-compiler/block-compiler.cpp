/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/block-compiler/transforms/Transform.h"
#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/ir-sorter.h"
#include "blockjit/IRInstruction.h"
#include "blockjit/IROperand.h"
#include "blockjit/IRPrinter.h"

#include <algorithm>
#include <set>
#include <list>
#include <map>
#include <queue>
#include <fstream>
#include <unordered_map>

#include "util/wutils/small-set.h"
#include "util/wutils/maybe-map.h"
#include "util/wutils/tick-timer.h"
#include "util/wutils/dense-set.h"
#include "util/SimOptions.h"

#include "util/LogContext.h"

UseLogContext(LogBlockJit)

#include "translate/jit_funs.h"
#include "translate/profile/Region.h"

#include <iostream>
#include "blockjit/blockjit-shunts.h"

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a)[0]))

extern "C" void cpu_set_mode(void *cpu, uint8_t mode);
extern "C" void cpu_write_device(void *cpu, uint32_t devid, uint32_t reg, uint32_t val);
extern "C" void cpu_read_device(void *cpu, uint32_t devid, uint32_t reg, uint32_t& val);
extern "C" void jit_verify(void *cpu);
extern "C" void jit_rum(void *cpu);

using namespace captive::arch::jit;
using namespace captive::arch::jit::algo;
using namespace captive::shared;

static void dump_insn(IRInstruction *insn);

/* Register Mapping
 *
 * RAX  Allocatable			0    NP
 * RBX  Allocatable			1     P
 * RCX  Temporary			t0   NP
 * RDX  Allocatable			2    NP
 * RSI  Allocatable			3    NP
 * RDI  Register File            NP
 * R8   Allocatable			4    NP
 * R9   Allocatable			5    NP
 * R10  Allocatable			6    NP
 * R11  Allocatable			7    NP
 * R12  Not used                  P
 * R13  Allocatable			8     P
 * R14  Temporary			t1    P
 * R15  Temporary			CPU   P
 *
 */

BlockCompiler::BlockCompiler(TranslationContext& ctx, uint32_t pa, wulib::MemAllocator &allocator, bool emit_interrupt_check, bool emit_chaining_logic)
	: ctx(ctx),
	  pa(pa),
	  emit_interrupt_check(emit_interrupt_check),
	  emit_chaining_logic(emit_chaining_logic),
	  _allocator(allocator)
{

}

void dump_ir(const std::string &name, uint32_t block_pc, TranslationContext &ctx)
{
	if(archsim::options::Debug) {
		archsim::blockjit::IRPrinter printer;
		std::ostringstream filename_str;
		filename_str << "blkjit-" << std::hex << block_pc << "-" << name << ".txt";
		std::ofstream file_stream(filename_str.str().c_str());
		printer.DumpIR(file_stream, ctx);
	}
}

CompileResult BlockCompiler::compile(bool dump_intermediates)
{
	uint32_t max_stack = 0;

	transforms::SortIRTransform sorter;

	transforms::ReorderBlocksTransform reorder;
	if (!reorder.Apply(ctx)) return false;

	transforms::ThreadJumpsTransform threadjumps;
	if (!threadjumps.Apply(ctx)) return false;

	transforms::DeadBlockEliminationTransform dbe;
	if (!dbe.Apply(ctx)) return false;

	transforms::MergeBlocksTransform mergeblocks;
	if (!mergeblocks.Apply(ctx)) return false;
	
	transforms::PeepholeTransform peephole;
	if (!peephole.Apply(ctx)) return false;

	transforms::RegStoreEliminationTransform rse;
	if(!rse.Apply(ctx)) return false;

	if (!sorter.Apply(ctx)) return false;

	transforms::ValueRenumberingTransform vrt;
	if(!vrt.Apply(ctx)) return false;

		// dump before register allocation
	dump_ir("premovelimination", GetBlockPA(), ctx);
	
	transforms::MovEliminationTransform mov_elimination;
	if(!mov_elimination.Apply(ctx)) return false;
	
	dump_ir("preconstantprop", GetBlockPA(), ctx);
	transforms::ConstantPropTransform cpt;
	if (!cpt.Apply(ctx)) return false;
	dump_ir("postconstantprop", GetBlockPA(), ctx);
	
	transforms::DeadStoreElimination dse;
	if(!dse.Apply((ctx))) return false;
	
	sorter.Apply(ctx);
	
	// dump before register allocation
	dump_ir("prealloc", GetBlockPA(), ctx);
		
	transforms::GlobalRegisterAllocationTransform reg_alloc(BLKJIT_NUM_ALLOCABLE);
	if(!reg_alloc.Apply(ctx)) return false;
	dump_ir("postalloc", GetBlockPA(), ctx);
	
//	transforms::GlobalRegisterReuseTransform reg_reuse(reg_alloc.GetUsedPhysRegs());
//	if(!reg_alloc.Apply(ctx)) return false;
//	dump_ir("postgrr", GetBlockPA(), ctx);
	
	if( !post_allocate_peephole()) return false;

	transforms::PostAllocatePeephole pap;
	if(!pap.Apply(ctx)) return false;
	
	
	sorter.Apply(ctx);
	transforms::Peephole2Transform p2;
	if(!p2.Apply(ctx)) return false;

	sorter.Apply(ctx);
	
	// dump before register allocation
	dump_ir("final", GetBlockPA(), ctx);

	return CompileResult(true, reg_alloc.GetStackFrameSize(), reg_alloc.GetUsedPhysRegs());
}

static void make_instruction_nop(IRInstruction *insn, bool set_block)
{
	insn->type = IRInstruction::NOP;
	insn->operands.clear();
	if(set_block) insn->ir_block = NOP_BLOCK;
}

static bool is_mov_nop(IRInstruction *insn, IRInstruction *next, bool set_block)
{
	if (insn->type == IRInstruction::MOV) {
		if ((insn->operands[0].alloc_mode == insn->operands[1].alloc_mode) && (insn->operands[0].alloc_data == insn->operands[1].alloc_data)) {
			make_instruction_nop(insn, set_block);
			return true;
		}

		if (next->type == IRInstruction::MOV) {
			if (insn->operands[0].alloc_mode == next->operands[1].alloc_mode &&
			    insn->operands[0].alloc_data == next->operands[1].alloc_data &&
			    insn->operands[1].alloc_mode == next->operands[0].alloc_mode &&
			    insn->operands[1].alloc_data == next->operands[0].alloc_data) {

				make_instruction_nop(next, set_block);
				return false;
			}
		}

		return false;
	} else {
		return false;
	}
}

// We can merge the instruction if it is an add or subtract of a constant value into a register
static bool is_add_candidate(IRInstruction *insn)
{
	return ((insn->type == IRInstruction::ADD) || (insn->type == IRInstruction::SUB))
	       && insn->operands[0].is_constant() && insn->operands[1].is_alloc_reg();
}

// We can merge the instructions if the mem instruction is a load, from the add's target, into the add's target
static bool is_mem_candidate(IRInstruction *mem, IRInstruction *add)
{
	if(mem->type != IRInstruction::READ_MEM) return false;

	IROperand *add_target = &add->operands[1];

	// We should only be here if the add is a candidate
	assert(add_target->is_alloc_reg());

	IROperand *mem_source = &mem->operands[0];
	IROperand *mem_target = &mem->operands[2];

	if(!mem_source->is_alloc_reg() || !mem_target->is_alloc_reg()) return false;
	if(mem_source->alloc_data != mem_target->alloc_data) return false;
	if(mem_source->alloc_data != add_target->alloc_data) return false;

	return true;
}

static bool is_breaker(IRInstruction *add, IRInstruction *test)
{
	assert(add->type == IRInstruction::ADD || add->type == IRInstruction::SUB);

	if(test->ir_block == NOP_BLOCK) return false;

	if((add->ir_block != test->ir_block) && (test->ir_block != NOP_BLOCK)) {
		return true;
	}

	// If the instruction under test touches the target of the add, then it is a breaker
	if(test->type != IRInstruction::READ_MEM) {
		IROperand *add_target = &add->operands[1];
		for(int op_idx = 0; op_idx < test->operands.size(); ++op_idx) {
			if((test->operands[op_idx].alloc_mode == add_target->alloc_mode) && (test->operands[op_idx].alloc_data == add_target->alloc_data)) return true;
		}
	}
	return false;
}

bool BlockCompiler::post_allocate_peephole()
{
	for(uint32_t ir_idx = 0; ir_idx < ctx.count() - 1; ++ir_idx) {
		is_mov_nop(ctx.at(ir_idx), ctx.at(ir_idx + 1), true);
	}

	uint32_t add_insn_idx = 0;
	uint32_t mem_insn_idx = 0;
	IRInstruction *add_insn, *mem_insn;

	//~ printf("*****\n");
	//~ dump_ir();

	while(true) {
		// get the next add instruction
		do {
			if(add_insn_idx >= ctx.count()) goto exit;
			add_insn = ctx.at(add_insn_idx++);
		} while(!is_add_candidate(add_insn));

		//~ printf("Considering add=%u\n", add_insn_idx-1);

		mem_insn_idx = add_insn_idx;
		bool broken = false;
		do {
			if(mem_insn_idx >= ctx.count()) goto exit;
			mem_insn = ctx.at(mem_insn_idx++);
			if(is_breaker(add_insn, mem_insn)) {
				//~ printf("Broken by %u\n", mem_insn_idx);
				broken = true;
				break;
			}

		} while(!is_mem_candidate(mem_insn, add_insn));

		if(broken) continue;

		//~ printf("Considering mem=%u\n", mem_insn_idx-1);

		assert(mem_insn->operands[1].is_constant());
		if(add_insn->type == IRInstruction::ADD)
			mem_insn->operands[1].value += add_insn->operands[0].value;
		else
			mem_insn->operands[1].value -= add_insn->operands[0].value;

		make_instruction_nop(add_insn, true);
	}
exit:
	return true;
}

bool BlockCompiler::build_cfg(block_list_t& blocks, cfg_t& succs, cfg_t& preds, block_list_t& exits)
{
	IRBlockId current_block_id = INVALID_BLOCK_ID;

	blocks.reserve(ctx.block_count());
	exits.reserve(ctx.block_count());

	for (unsigned int ir_idx = 0; ir_idx < ctx.count(); ir_idx++) {
		IRInstruction *insn = ctx.at(ir_idx);
		if(insn->ir_block == NOP_BLOCK) continue;

		if (insn->ir_block != current_block_id) {
			current_block_id = insn->ir_block;
			blocks.push_back(current_block_id);
		}

		switch (insn->type) {
			case IRInstruction::BRANCH: {
				assert(insn->operands[1].is_block() && insn->operands[2].is_block());

				IRBlockId s1 = (IRBlockId)insn->operands[1].value;
				IRBlockId s2 = (IRBlockId)insn->operands[2].value;

				succs[current_block_id].push_back(s1);
				succs[current_block_id].push_back(s2);

				preds[s1].push_back(current_block_id);
				preds[s2].push_back(current_block_id);
				break;
			}

			case IRInstruction::JMP: {
				assert(insn->operands[0].is_block());

				IRBlockId s1 = (IRBlockId)insn->operands[0].value;

				succs[current_block_id].push_back(s1);
				preds[s1].push_back(current_block_id);

				break;
			}

			case IRInstruction::DISPATCH:
			case IRInstruction::RET: {
				exits.push_back(current_block_id);
				break;
			}

			default:
				continue;
		}
	}

	return true;
}
