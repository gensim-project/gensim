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
#include "gensim/gensim_processor_state.h"

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
using namespace captive::arch::x86;
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
	int i = 0;
#define ASSIGN_REGS(x) assign(i++, x(8), x(4), x(2), x(1))
	ASSIGN_REGS(REGS_RAX);
	ASSIGN_REGS(REGS_RBX);
	ASSIGN_REGS(REGS_RCX);
	ASSIGN_REGS(REGS_R8);
	ASSIGN_REGS(REGS_R9);
	ASSIGN_REGS(REGS_R10);
	ASSIGN_REGS(REGS_R11);
	ASSIGN_REGS(REGS_R13);
	ASSIGN_REGS(REGS_R14);

#undef ASSIGN_REGS
}

size_t BlockCompiler::compile(block_txln_fn& fn)
{
	uint32_t max_stack = 0;

	bool dump_to_the_max = true;
	dump_to_the_max &= archsim::options::Debug;

	tick_timer timer(0, stderr);

	transforms::SortIRTransform sorter;
	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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
	if(!rvr.Apply(ctx)) return false;
	timer.tick("RVR");

	transforms::RegStoreEliminationTransform rse;
	if(!rse.Apply(ctx)) return false;
	timer.tick("RSE");

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if (!analyse(max_stack)) return false;

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if( !lower_stack_to_reg()) return false;
	timer.tick("LSTR");

	if(dump_to_the_max) {
		sorter.Apply(ctx);
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

	if(dump_to_the_max) {
		sorter.Apply(ctx);
		std::ostringstream str;
		str << "blkjit-o7-" << std::hex << this->pa << ".txt";
		std::ofstream of (str.str());
		str.str("");
		dump_ir(str);
		of << str.str();
		of.close();
	}

	sorter.Apply(ctx);

//	analyses::HostRegLivenessAnalysis liveness_analysis;
//	auto liveness = liveness_analysis.Run(ctx);

	lowering::x86::X86LoweringContext lowering(max_stack, encoder);
	lowering.Prepare(ctx, *this);
//	lowering.SetLivenessData(liveness);

	if(!lowering.Lower(ctx)) {
		LC_ERROR(LogBlockJit) << "Failed to lower block";
		return false;
	}

	timer.tick("Lower");

	timer.dump("blockcompiler ");

//	printf("block %x has address %p\n", pa, (void*)encoder.get_buffer());
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
	tick_timer timer(0);
	timer.reset();

	IRBlockId latest_block_id = INVALID_BLOCK_ID;

	used_phys_regs.clear();

	std::vector<int32_t> allocation (ctx.reg_count());
	maybe_map<IRRegId, uint32_t, 128> global_allocation (ctx.reg_count());	// global register allocation

	typedef dense_set<IRRegId> live_set_t;
	live_set_t live_ins(ctx.reg_count()), live_outs(ctx.reg_count());
	std::vector<live_set_t::iterator> to_erase;

	PopulatedSet<BLKJIT_NUM_ALLOCABLE> avail_regs; // Register indicies that are available for allocation.
	uint32_t next_global = 0;	// Next stack location for globally allocated register.

	std::vector<int32_t> vreg_seen_block (ctx.reg_count(), -1);

	timer.tick("Init");

	// Build up a map of which vregs have been seen in which blocks, to detect spanning vregs.
	int32_t ir_idx;
	for (ir_idx = 0; ir_idx < (int32_t)ctx.count(); ir_idx++) {
		IRInstruction *insn = ctx.at(ir_idx);

		if(insn->ir_block == NOP_BLOCK) break;
		if(insn->type == IRInstruction::BARRIER) next_global = 0;

		for (int op_idx = 0; op_idx < 6; op_idx++) {
			IROperand *oper = &insn->operands[op_idx];
			if(!oper->is_valid()) break;

			// If the operand is a vreg, and is not already a global...
			if (oper->is_vreg() && (global_allocation.count(oper->value) == 0)) {
				auto seen_in_block = vreg_seen_block[oper->value];

				// If we have already seen this operand, and not in the same block, then we
				// must globally allocate it.
				if (seen_in_block != -1 && seen_in_block != (int32_t)insn->ir_block) {
					global_allocation[oper->value] = next_global;
					next_global += 8;
					if(next_global > max_stack) max_stack = next_global;
				}

				vreg_seen_block[oper->value] = insn->ir_block;
			}
		}
	}

	timer.tick("Globals");

	// ir_idx is set up by the previous loop to point either to the end of the context, or to the last NOP
	for (; ir_idx >= 0; ir_idx--) {
		// Grab a pointer to the instruction we're looking at.
		IRInstruction *insn = ctx.at(ir_idx);
		if(insn->ir_block == NOP_BLOCK) continue;

		// Make sure it has a descriptor.
		assert(insn->type < captive::shared::num_descriptors);
		const struct insn_descriptor *descr = &insn_descriptors[insn->type];

		// Detect a change in block
		if (latest_block_id != insn->ir_block) {
			// Clear the live-in working set and current allocations.
			live_ins.clear();
			allocation.assign(ctx.reg_count(), -1);

			// Reset the available register bitfield
			avail_regs.fill((1 << BLKJIT_NUM_ALLOCABLE)-1);

			// Update the latest block id.
			latest_block_id = insn->ir_block;
		}

		if(insn->type == IRInstruction::BARRIER) {
			next_global = 0;
		}

		// Clear the live-out set, and make every current live-in a live-out.
		live_outs.copy(live_ins);

		// Loop over the VREG operands and update the live-in set accordingly.
		for (int o = 0; o < 6; o++) {
			if (!insn->operands[o].is_valid()) break;
			if (insn->operands[o].type != IROperand::VREG) continue;

			IROperand *oper = &insn->operands[o];
			IRRegId reg = (IRRegId)oper->value;

			if (descr->format[o] == 'I') {
				// <in>
				live_ins.insert(reg);
			} else if (descr->format[o] == 'O') {
				// <out>
				live_ins.erase(reg);
			} else if (descr->format[o] == 'B') {
				// <in/out>
				live_ins.insert(reg);
			}
		}

		//printf("  [%03d] %10s", ir_idx, descr->mnemonic);

		// Release LIVE-OUTs that are not LIVE-INs
		for (auto out : live_outs) {
			if(global_allocation.count(out)) continue;
			if (!live_ins.count(out)) {
				assert(allocation[out] != -1);

				// Make the released register available again.
				avail_regs.set(allocation[out]);
			}
		}

		// Allocate LIVE-INs
		for (auto in : live_ins) {
			// If the live-in is not already allocated, allocate it.
			if (allocation[in] == -1 && global_allocation.count(in) == 0) {
				int32_t next_reg = avail_regs.next_avail();

				if (next_reg == -1) {
					global_allocation[in] = next_global;
					next_global += 8;
					if(max_stack < next_global) max_stack = next_global;

				} else {
					allocation[in] = next_reg;

					avail_regs.clear(next_reg);
					used_phys_regs.set(next_reg);
				}
			}
		}

		// If an instruction has no output or both operands which are live outs then the instruction is dead
		bool not_dead = false;
		bool can_be_dead = !descr->has_side_effects;

		// Loop over operands to update the allocation information on VREG operands.
		for (int op_idx = 0; op_idx < 6; op_idx++) {
			IROperand *oper = &insn->operands[op_idx];
			if(!oper->is_valid()) break;

			if (oper->is_vreg()) {
				if (descr->format[op_idx] != 'I' && live_outs.count(oper->value)) not_dead = true;

				// If this vreg has been allocated to the stack, then fill in the stack entry location here
				//auto global_alloc = global_allocation.find(oper->value);
				if(global_allocation.count(oper->value)) {
					oper->allocate(IROperand::ALLOCATED_STACK, global_allocation[oper->value]);
					if(descr->format[op_idx] == 'O' || descr->format[op_idx] == 'B') not_dead = true;
				} else {

					// Otherwise, if the value has been locally allocated, fill in the local allocation
					//auto alloc_reg = allocation.find((IRRegId)oper->value);

					if (allocation[oper->value] != -1) {
						oper->allocate(IROperand::ALLOCATED_REG, allocation[oper->value]);
					}

				}
			}
		}

		// Remove allocations that are not LIVE-INs
		for (auto out : live_outs) {
			if(global_allocation.count(out)) continue;
			if (!live_ins.count(out)) {
				assert(allocation[out] != -1);

				allocation[out] = -1;
			}
		}

		// If this instruction is dead, remove any live ins which are not live outs
		// in order to propagate dead instruction elimination information
		if(!not_dead && can_be_dead) {
			make_instruction_nop(insn, true);

			to_erase.clear();
			to_erase.reserve(live_ins.size());
			for(auto i = live_ins.begin(); i != live_ins.end(); ++i) {
				const auto &in = *i;
				if(global_allocation.count(in)) continue;
				if(live_outs.count(in) == 0) {
					assert(allocation[in] != -1);

					avail_regs.set(allocation[in]);

					allocation[in] = -1;
					to_erase.push_back(i);
				}
			}
			for(auto e : to_erase)live_ins.erase(*e);
		}
	}

	//printf("block %08x\n", tb.block_addr);
//	dump_ir();
	timer.tick("Allocation");
	//printf("lol ");
	timer.dump("Analysis");
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

void BlockCompiler::emit_save_reg_state(int num_operands, stack_map_t &stack_map, bool &fix_stack, uint32_t live_regs)
{
	uint32_t stack_offset = 0;
	// XXX host architecture dependent
	// On X86-64, the stack pointer on entry to a function must have an offset
	// of 0x8 (such that %rsp + 8 is 16 byte aligned). Since the call instruction
	// will push 8 bytes onto the stack, this means that if an even number of
	// registers are saved, overall an odd number of stack slots will be
	// consumed and the stack alignment requirements will be violated. Thus,
	// we must 'fix' the stack by default.
	fix_stack = true;

	stack_map.clear();

	//First, count required stack slots
	for(int i = used_phys_regs.size()-1; i >= 0; i--) {
		if(used_phys_regs.get(i) && (live_regs & (1 << i))) {
			stack_offset += 8;
			fix_stack = !fix_stack;
		}
	}

	if(fix_stack) {
		encoder.sub(8, REG_RSP);
//		encoder.push(REG_RAX);
	}

	for(int i = used_phys_regs.size()-1; i >= 0; i--) {
		if(used_phys_regs.get(i) && (live_regs & (1 << i))) {
			auto& reg = get_allocable_register(i, 8);

			stack_offset -= 8;
			stack_map[&reg] = stack_offset;
			encoder.push(reg);

		}
	}
}

void BlockCompiler::emit_restore_reg_state(int num_operands, stack_map_t &stack_map, bool fix_stack, uint32_t live_regs)
{

	for(unsigned int i = 0; i < used_phys_regs.size(); ++i) {
		if(used_phys_regs.get(i) && (live_regs & (1 << i))) {
			encoder.pop(get_allocable_register(i, 8));
		}
	}
	if(fix_stack) {
		encoder.add(8, REG_RSP);
//		encoder.pop(REG_RAX);
	}
}

void BlockCompiler::encode_operand_function_argument(const IROperand *oper, const X86Register& target_reg, stack_map_t &stack_map)
{
	if (oper->is_constant() == IROperand::CONSTANT) {
		if(oper->value == 0) encoder.xorr(target_reg, target_reg);
		else encoder.mov(oper->value, target_reg);
	} else if (oper->is_vreg()) {
//		if(get_allocable_register(oper->alloc_data, 8) == REG_RBX) {
//			encoder.mov(REGS_RBX(oper->size), target_reg);
//		} else
		if (oper->is_alloc_reg()) {
			auto &reg = get_allocable_register(oper->alloc_data, 8);
			encoder.mov(X86Memory::get(REG_RSP, stack_map.at(&reg)), target_reg);
		} else {
			int64_t stack_offset = (oper->alloc_data + (8*stack_map.size()));
			encoder.mov(X86Memory::get(REG_RSP, stack_offset), target_reg);
		}
	} else {
		assert(false);
	}
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
			ostr << "block " << current_block_id << ":\n";
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

void BlockCompiler::encode_operand_to_reg(const shared::IROperand *operand, const x86::X86Register& reg)
{
	switch (operand->type) {
		case IROperand::CONSTANT: {
			if (operand->value == 0) {
				encoder.xorr(reg, reg);
			} else {
				encoder.mov(operand->value, reg);
			}

			break;
		}

		case IROperand::VREG: {
			switch (operand->alloc_mode) {
				case IROperand::ALLOCATED_STACK:
					encoder.mov(stack_from_operand(operand), reg);
					break;

				case IROperand::ALLOCATED_REG:
					encoder.mov(register_from_operand(operand, reg.size), reg);
					break;

				default:
					assert(false);
			}

			break;
		}

		default:
			assert(false);
	}
}

bool BlockCompiler::lower_stack_to_reg()
{
	std::map<uint32_t, uint32_t> lowered_entries;
	PopulatedSet<BLKJIT_NUM_ALLOCABLE> avail_phys_regs = used_phys_regs;
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
					if(avail_phys_regs.empty()) continue;

					selected_reg = avail_phys_regs.next_avail();
					avail_phys_regs.clear(selected_reg);
					assert(selected_reg != -1);
					lowered_entries[op.alloc_data] = selected_reg;
					used_phys_regs.set(selected_reg);

				} else {
					selected_reg = lowered_entries[op.alloc_data];
				}
				op.allocate(IROperand::ALLOCATED_REG, selected_reg);
			}
		}
	}

	return true;
}
