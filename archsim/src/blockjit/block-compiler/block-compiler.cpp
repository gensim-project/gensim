#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/block-compiler/transforms/Transform.h"
#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/ir-sorter.h"
#include "blockjit/IRInstruction.h"
#include "blockjit/IROperand.h"

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
	  encoder(allocator),
	  _allocator(allocator)
{

}

size_t BlockCompiler::compile(block_txln_fn& fn, bool dump_intermediates)
{
	uint32_t max_stack = 0;

	tick_timer timer(0, stderr);

	transforms::SortIRTransform sorter;
	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-preopt-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	timer.reset();
	transforms::ReorderBlocksTransform reorder;
	if (!reorder.Apply(ctx)) return false;
	timer.tick("Reorder");

	transforms::ThreadJumpsTransform threadjumps;
	if (!threadjumps.Apply(ctx)) return false;
	timer.tick("JT");

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-reorder-jt-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	transforms::DeadBlockEliminationTransform dbe;
	if (!dbe.Apply(ctx)) return false;
	timer.tick("DBE");

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-reorder-jt-dbe-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	transforms::MergeBlocksTransform mergeblocks;
	if (!mergeblocks.Apply(ctx)) return false;
	timer.tick("MB");

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-o0-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	transforms::ConstantPropTransform cpt;
	if (!cpt.Apply(ctx)) return false;
	timer.tick("Cprop");

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-o1-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	transforms::PeepholeTransform peephole;
	if (!peephole.Apply(ctx)) return false;
	timer.tick("Peep");

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-o2-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

//	if (!value_merging()) return false;
	timer.tick("VM");

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-o3-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	sorter.Apply(ctx);
	transforms::RegValueReuseTransform rvr;
//	if(!rvr.Apply(ctx)) return false;
	timer.tick("RVR");

	transforms::RegStoreEliminationTransform rse;
	if(!rse.Apply(ctx)) return false;
	timer.tick("RSE");

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-o4-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	if (!sorter.Apply(ctx)) return false;
	timer.tick("Sort");

	transforms::ValueRenumberingTransform vrt;
	if(!vrt.Apply(ctx)) return false;

	timer.tick("VRT");

	transforms::RegisterAllocationTransform reg_alloc(BLKJIT_NUM_ALLOCABLE);
	if(!reg_alloc.Apply(ctx)) return false;

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-o5-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	timer.tick("Analyse");
	if( !post_allocate_peephole()) return false;
	timer.tick("PAP");

	transforms::PostAllocatePeephole pap;
	if(!pap.Apply(ctx)) return false;
	
	auto used_phys_regs = reg_alloc.GetUsedPhysRegs();

	if( !lower_stack_to_reg(used_phys_regs)) return false;
	timer.tick("LSTR");

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-o6-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	sorter.Apply(ctx);
	transforms::Peephole2Transform p2;
	if(!p2.Apply(ctx)) return false;

	if(dump_intermediates) {
		std::ostringstream str;
		str << "blkjit-o7-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	sorter.Apply(ctx);


	lowering::x86::X86LoweringContext lowering(reg_alloc.GetStackFrameSize(), encoder, get_cpu(), used_phys_regs);
	lowering.Prepare(ctx, *this);

	
	if(!lowering.Lower(ctx)) {
		LC_ERROR(LogBlockJit) << "Failed to lower block";
		return false;
	}

	timer.tick("Lower");

	timer.dump("blockcompiler ");

	fn = (block_txln_fn)encoder.get_buffer();
	fn = (block_txln_fn)_allocator.Reallocate(encoder.get_buffer(), encoder.get_buffer_size());

	return encoder.get_buffer_size();
}

static void make_instruction_nop(IRInstruction *insn, bool set_block)
{
	insn->type = IRInstruction::NOP;
	insn->operands[0].type = IROperand::NONE;
	insn->operands[1].type = IROperand::NONE;
	insn->operands[2].type = IROperand::NONE;
	insn->operands[3].type = IROperand::NONE;
	insn->operands[4].type = IROperand::NONE;
	insn->operands[5].type = IROperand::NONE;
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
		for(int op_idx = 0; op_idx < 6; ++op_idx) {
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

bool BlockCompiler::analyse(uint32_t& max_stack)
{
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



static void dump_insn(IRInstruction *insn, std::ostringstream &str)
{
	assert(insn->type < captive::shared::num_descriptors);
	const struct insn_descriptor *descr = &insn_descriptors[insn->type];

	str << " " << std::left << std::setw(12) << std::setfill(' ') << descr->mnemonic;

	for (int op_idx = 0; op_idx < 6; op_idx++) {
		IROperand *oper = &insn->operands[op_idx];

		if (descr->format[op_idx] != 'X') {
			if (descr->format[op_idx] == 'M' && !oper->is_valid()) continue;

			if (op_idx > 0) str << ", ";

			if (oper->is_vreg()) {
				char alloc_char = oper->alloc_mode == IROperand::NOT_ALLOCATED ? 'N' : (oper->alloc_mode == IROperand::ALLOCATED_REG ? 'R' : (oper->alloc_mode == IROperand::ALLOCATED_STACK ? 'S' : '?'));
				str << "i" << (uint32_t)oper->size << " r" << std::dec << oper->value << "(" << alloc_char << oper->alloc_data << ")";
			} else if (oper->is_constant()) {
				str << "i" << (uint32_t)oper->size << " $0x" << std::hex << oper->value;
			} else if (oper->is_pc()) {
				str << "i4 pc (" << std::hex << oper->value << ")";
			} else if (oper->is_block()) {
				str << "b" << std::dec << oper->value;
			} else if (oper->is_func()) {
				str << "&" << std::hex << oper->value;
			} else {
				str << "<invalid>";
			}
		}
	}
}

void BlockCompiler::dump_ir(std::ostringstream &ostr)
{
	IRBlockId current_block_id = INVALID_BLOCK_ID;

	for (uint32_t ir_idx = 0; ir_idx < ctx.count(); ir_idx++) {
		IRInstruction *insn = ctx.at(ir_idx);

		if (current_block_id != insn->ir_block) {
			current_block_id = insn->ir_block;
			ostr << "block " << std::hex << current_block_id << ":\n";
		}

		dump_insn(insn, ostr);

		ostr << std::endl;
	}
}

void BlockCompiler::dump_ir()
{
	std::ostringstream str;
	dump_ir(str);
	std::cout << str.str();
}


bool BlockCompiler::lower_stack_to_reg(archsim::util::vbitset &used_phys_regs)
{
	std::map<uint32_t, uint32_t> lowered_entries;
	archsim::util::vbitset avail_phys_regs = used_phys_regs;
	avail_phys_regs.invert();

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);

		// Don't need to do any clever allocation here since the stack entries should already be re-used

		for(unsigned int op_idx = 0; op_idx < 6; ++op_idx) {
			IROperand &op = insn->operands[op_idx];
			if(!op.is_valid()) break;

			if(op.is_alloc_stack()) {
				int32_t selected_reg;
				if(!lowered_entries.count(op.alloc_data)) {
					if(avail_phys_regs.all_clear()) continue;

					selected_reg = avail_phys_regs.get_lowest_set();
					avail_phys_regs.set(selected_reg, 0);
					assert(selected_reg != -1);
					lowered_entries[op.alloc_data] = selected_reg;
					used_phys_regs.set(selected_reg, 1);

				} else {
					selected_reg = lowered_entries[op.alloc_data];
				}
				op.allocate(IROperand::ALLOCATED_REG, selected_reg);
			}
		}
	}

	return true;
}
