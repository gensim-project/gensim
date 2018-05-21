/*
 * File:   block-compiler.h
 * Author: spink
 *
 * Created on 27 April 2015, 11:58
 */

#ifndef BLOCK_COMPILER_H
#define	BLOCK_COMPILER_H

#include <define.h>
#include "blockjit/ir.h"
#include "blockjit/encode.h"
#include "blockjit/IRInstruction.h"
#include "blockjit/IROperand.h"
#include "blockjit/block-compiler/analyses/Analysis.h"

#include "blockjit/translation-context.h"
#include "util/wutils/small-set.h"
#include "core/thread/ThreadInstance.h"

#include <map>
#include <vector>
#include <sstream>
#include <iomanip>

#include "blockjit/blockjit-abi.h"

namespace captive
{
	namespace arch
	{
		namespace jit
		{
			class BlockCompiler
			{
			public:
				BlockCompiler(TranslationContext& ctx, uint32_t pa, wulib::MemAllocator &allocator, bool emit_interrupt_check = false, bool emit_chaining_logic = false);
				size_t compile(shared::block_txln_fn& fn, bool dump_intermediates = false);

				void dump_ir(std::ostringstream &ostr);
				void dump_ir();

				bool emit_interrupt_check;
				bool emit_chaining_logic;

				void set_cpu(archsim::core::thread::ThreadInstance *cpu)
				{
					_cpu = cpu;
				}
				const archsim::core::thread::ThreadInstance *get_cpu()
				{
					return _cpu;
				}

				uint32_t GetBlockPA() const
				{
					return pa;
				}
			private:
				wulib::MemAllocator &_allocator;
				TranslationContext& ctx;
				x86::X86Encoder encoder;
				archsim::core::thread::ThreadInstance *_cpu;
				uint32_t pa;


				PopulatedSet<BLKJIT_NUM_ALLOCABLE> used_phys_regs;

				typedef std::map<shared::IRBlockId, std::vector<shared::IRBlockId>> cfg_t;
				typedef std::vector<shared::IRBlockId> block_list_t;

				bool analyse(uint32_t& max_stack);
				bool build_cfg(block_list_t& blocks, cfg_t& succs, cfg_t& preds, block_list_t& exits);
				bool post_allocate_peephole();
				bool lower_stack_to_reg();
				bool reg_value_reuse();

				bool reg_dse();

				// TODO: all this register related stuff should really be in the lowering context
				typedef std::map<const captive::arch::x86::X86Register*, uint32_t> stack_map_t;
				struct reg_assignment {
					const x86::X86Register *b1;
					const x86::X86Register *b2;
					const x86::X86Register *b4;
					const x86::X86Register *b8;
				};
				std::vector<struct reg_assignment> register_assignments;

			public:

				void emit_save_reg_state(int num_operands, stack_map_t&, bool &fixed_stack, uint32_t live_regs = 0xffffffff);
				void emit_restore_reg_state(int num_operands, stack_map_t&, bool fixed_stack, uint32_t live_regs = 0xffffffff);
				void encode_operand_function_argument(const shared::IROperand *oper, const x86::X86Register& reg, stack_map_t&);
				void encode_operand_to_reg(const shared::IROperand *operand, const x86::X86Register& reg);


				inline const x86::X86Register &get_allocable_register(int index, int size) const
				{
					auto &regs = register_assignments.at(index);
					switch(size) {
						case 1:
							return *regs.b1;
						case 2:
							return *regs.b2;
						case 4:
							return *regs.b4;
						case 8:
							return *regs.b8;
					}
					__builtin_unreachable();
				}

				inline void assign(uint8_t id, const x86::X86Register& r8, const x86::X86Register& r4, const x86::X86Register& r2, const x86::X86Register& r1)
				{
					if(register_assignments.size() <= id) register_assignments.resize(id+1);
					auto &regs = register_assignments[id];
					regs.b1 = &r1;
					regs.b2 = &r2;
					regs.b4 = &r4;
					regs.b8 = &r8;
				}

				inline const x86::X86Register& register_from_operand(const captive::shared::IROperand *oper, int force_width = 0) const
				{
					assert(oper->alloc_mode == captive::shared::IROperand::ALLOCATED_REG);

					if (!force_width) force_width = oper->size;

					return get_allocable_register(oper->alloc_data, force_width);
				}

				inline x86::X86Memory stack_from_operand(const captive::shared::IROperand *oper) const
				{
					assert(oper->alloc_mode == captive::shared::IROperand::ALLOCATED_STACK);
					assert(oper->size <= 8);

					return x86::X86Memory(x86::REG_RSP, oper->alloc_data);
				}

				inline const x86::X86Register& get_temp(int id, int width)
				{
					switch (id) {
						case 0: {
							return BLKJIT_TEMPS_0(width);
						}
						case 1: {
							return BLKJIT_TEMPS_1(width);
						}
						case 2: {
							return BLKJIT_TEMPS_2(width);
						}
					}
					__builtin_unreachable();
				}

				inline const x86::X86Register& unspill_temp(const captive::shared::IROperand *oper, int id)
				{
					const x86::X86Register& tmp = get_temp(id, oper->size);
					encoder.mov(stack_from_operand(oper), tmp);
					return tmp;
				}

				inline void load_state_field(const std::string &entry, const x86::X86Register& reg)
				{
					encoder.mov(x86::X86Memory::get(BLKJIT_CPUSTATE_REG,  get_cpu()->GetStateBlock().GetBlockOffset(entry)), reg);
				}

				inline void lea_state_field(const std::string &entry, const x86::X86Register& reg)
				{
					encoder.lea(x86::X86Memory::get(BLKJIT_CPUSTATE_REG, get_cpu()->GetStateBlock().GetBlockOffset(entry)), reg);
				}
			};
		}
	}
}

#endif	/* BLOCK_COMPILER_H */

