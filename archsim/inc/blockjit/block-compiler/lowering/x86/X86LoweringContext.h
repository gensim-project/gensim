/*
 * X86LoweringContext.h
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86LOWERINGCONTEXT_H_
#define INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86LOWERINGCONTEXT_H_

#include "blockjit/block-compiler/lowering/LoweringContext.h"
#include "blockjit/block-compiler/lowering/InstructionLowerer.h"
#include "blockjit/block-compiler/lowering/x86/X86Encoder.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"
#include "blockjit/block-compiler/lowering/x86/X86Encoder.h"
#include "core/thread/ThreadInstance.h"

#include "util/MemAllocator.h"
#include "util/wutils/small-set.h"
#include "util/vbitset.h"

#include <map>
#include <vector>

namespace captive
{
	namespace arch
	{
		namespace jit
		{
			namespace lowering
			{
				namespace x86
				{

					class X86LoweringContext : public LoweringContext
					{
					public:
						typedef X86Encoder encoder_t;

						X86LoweringContext(uint32_t stack_frame_size, encoder_t &encoder, const archsim::core::thread::ThreadInstance *thread, const archsim::util::vbitset &used_regs);
						virtual ~X86LoweringContext();

						encoder_t &GetEncoder()
						{
							return _encoder;
						}

						virtual offset_t GetEncoderOffset() override;

						virtual bool Prepare(const TranslationContext &ctx) override;

						typedef std::map<const X86Register*, uint32_t> stack_map_t;

						stack_map_t &GetStackMap()
						{
							return _stack_map;
						}
						bool &GetIsStackFixed()
						{
							return _stack_fixed;
						}

						const archsim::core::thread::ThreadInstance *GetThread() { return thread_; }
						
						inline void load_state_field(const std::string &entry, const X86Register& reg)
						{
							GetEncoder().mov(X86Memory::get(BLKJIT_CPUSTATE_REG,  GetThread()->GetStateBlock().GetBlockOffset(entry)), reg);
						}

						inline void lea_state_field(const std::string &entry, const X86Register& reg)
						{
							GetEncoder().lea(X86Memory::get(BLKJIT_CPUSTATE_REG, GetThread()->GetStateBlock().GetBlockOffset(entry)), reg);
						}
						

						void emit_save_reg_state(int num_operands, stack_map_t&, bool &fixed_stack, uint32_t live_regs = 0xffffffff);
						void emit_restore_reg_state(int num_operands, stack_map_t&, bool fixed_stack, uint32_t live_regs = 0xffffffff);
						void encode_operand_function_argument(const shared::IROperand *oper, const X86Register& reg, stack_map_t&);
						void encode_operand_to_reg(const shared::IROperand *operand, const X86Register& reg);

						inline void assign(uint8_t id, const X86Register& r8, const X86Register& r4, const X86Register& r2, const X86Register& r1)
						{
							if(register_assignments.size() <= id) register_assignments.resize(id+1);
							auto &regs = register_assignments[id];
							regs.b1 = &r1;
							regs.b2 = &r2;
							regs.b4 = &r4;
							regs.b8 = &r8;
						}

						inline const X86Register &get_allocable_register(int index, int size) const
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
						
						inline const X86Register& register_from_operand(const captive::shared::IROperand *oper, int force_width = 0) const
						{
							assert(oper->alloc_mode == captive::shared::IROperand::ALLOCATED_REG);

							if (!force_width) force_width = oper->size;

							return get_allocable_register(oper->alloc_data, force_width);
						}

						inline X86Memory stack_from_operand(const captive::shared::IROperand *oper) const
						{
							assert(oper->alloc_mode == captive::shared::IROperand::ALLOCATED_STACK);
							assert(oper->size <= 8);

							return X86Memory(REG_RSP, oper->alloc_data);
						}

						inline const X86Register& get_temp(int id, int width)
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

						inline const X86Register& unspill_temp(const captive::shared::IROperand *oper, int id)
						{
							const X86Register& tmp = get_temp(id, oper->size);
							GetEncoder().mov(stack_from_operand(oper), tmp);
							return tmp;
						}
						
					protected:
						virtual bool LowerHeader(const TranslationContext &ctx) override;
						virtual bool PerformRelocations(const TranslationContext &ctx) override;

					private:
						stack_map_t _stack_map;
						X86Encoder &_encoder;
						bool _stack_fixed;
						const archsim::core::thread::ThreadInstance *thread_;
						
						archsim::util::vbitset used_phys_regs;
						
						struct reg_assignment {
							const x86::X86Register *b1;
							const x86::X86Register *b2;
							const x86::X86Register *b4;
							const x86::X86Register *b8;
						};
						std::vector<struct reg_assignment> register_assignments;

					};

				}
			}
		}
	}
}



#endif /* INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86LOWERINGCONTEXT_H_ */
