/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   translation-context.h
 * Author: spink
 *
 * Created on 27 February 2015, 15:07
 */

#ifndef TRANSLATION_CONTEXT_H
#define	TRANSLATION_CONTEXT_H

#include <define.h>
#include <malloc.h>
#include "blockjit/ir.h"
#include "blockjit/IRInstruction.h"
#include "util/linked-vector.h"

#include <algorithm>

namespace captive
{
	namespace arch
	{
		namespace jit
		{
			class TranslationContext
			{
			public:
				TranslationContext();
				~TranslationContext();
				
				void clear();
				
				inline void add_instruction(shared::IRBlockId block_id, const shared::IRInstruction& instruction)
				{
					ensure_buffer(_ir_insn_count + 1);

					new(&_ir_insns[_ir_insn_count]) shared::IRInstruction(shared::IRInstruction::NOP);
					_ir_insns[_ir_insn_count] = instruction;
					_ir_insns[_ir_insn_count].ir_block = block_id;
					_ir_insn_count++;
				}
				
				inline shared::IRInstruction &get_next_instruction() {
					ensure_buffer(_ir_insn_count + 1);
					
					// construct object
					auto ptr = &_ir_insns[_ir_insn_count++];
					new(ptr) shared::IRInstruction(shared::IRInstruction::NOP);
					
					return *ptr;
				}
				inline shared::IRInstruction *get_next_instruction_ptr() {
					ensure_buffer(_ir_insn_count + 1);
					
					// return pointer without constructing object
					auto ptr = &_ir_insns[_ir_insn_count++];					
					return ptr;
				}

				inline shared::IRBlockId alloc_block()
				{
					return _ir_block_count++;
				}

				inline shared::IRRegId alloc_reg(uint8_t size)
				{
					return _ir_reg_count++;
				}

				inline uint32_t count() const
				{
					return _ir_insn_count;
				}
				inline uint32_t reg_count() const
				{
					return _ir_reg_count;
				}
				inline uint32_t block_count() const
				{
					return _ir_block_count;
				}

				inline shared::IRInstruction *at(uint32_t idx) const
				{
					return &_ir_insns[idx];
				}
				inline void put(uint32_t idx, shared::IRInstruction& insn)
				{
					_ir_insns[idx] = insn;
				}

				inline const shared::IRInstruction *begin() const
				{
					return _ir_insns;
				}
				inline const shared::IRInstruction *end() const
				{
					return _ir_insns + count();
				}
				inline shared::IRInstruction *begin()
				{
					return _ir_insns;
				}
				inline shared::IRInstruction *end()
				{
					return _ir_insns + count();
				}

				inline void swap(uint32_t a, uint32_t b)
				{
					if(a == b) return;

					// If we're swapping NOPs, just swap the block index and nothing else
					if(_ir_insns[a].type == shared::IRInstruction::NOP && _ir_insns[b].type == shared::IRInstruction::NOP) {
						uint32_t tmp = _ir_insns[a].ir_block;
						_ir_insns[a].ir_block = _ir_insns[b].ir_block;
						_ir_insns[b].ir_block = tmp;
					} else {
						std::swap(_ir_insns[a], _ir_insns[b]);
					}
				}

				inline const shared::IRInstruction *get_ir_buffer() const
				{
					return _ir_insns;
				}
				inline void free_ir_buffer()
				{
					for(unsigned i = 0; i < count(); ++i) {
						_ir_insns[i].~IRInstruction();
					}
					free(_ir_insns);
					_ir_insns = nullptr;
				}
				inline shared::IRInstruction *get_ir_buffer()
				{
					return _ir_insns;
				}

				inline void set_ir_buffer(shared::IRInstruction *new_buffer)
				{
					_ir_insns = new_buffer;
				}

				// TODO: if !NDEBUG, check that max block is actually the max block number
				void recount_blocks(uint32_t max_block)
				{
					_ir_block_count = max_block;
				}
				void recount_regs(uint32_t max_reg)
				{
					_ir_reg_count = max_reg;
				}
				
				size_t size_bytes() const
				{
					return sizeof(*_ir_insns) * _ir_insn_count;
				}
				
				void trim()
				{
					int64_t nop_block_start = -1;
					for(uint32_t i = 0; i < _ir_insn_count; ++i) {
						if(_ir_insns[i].ir_block == NOP_BLOCK) {
							nop_block_start = i;
							break;
						}
					}
					if(nop_block_start == -1) {
						return;
					}
					
					_ir_insn_count = nop_block_start;
					_ir_insns = (shared::IRInstruction*)realloc(_ir_insns, sizeof(shared::IRInstruction) * _ir_insn_count);
					_ir_insn_buffer_size = _ir_insn_count;
				}

			private:
				shared::IRBlockId _current_block_id;

				uint32_t _ir_block_count;
				uint32_t _ir_reg_count;

				shared::IRInstruction *_ir_insns;

				uint32_t _ir_insn_count;
				uint32_t _ir_insn_buffer_size;

				inline void ensure_buffer(uint32_t elem_capacity)
				{
					uint32_t required_size = (sizeof(shared::IRInstruction) * elem_capacity);

					if (_ir_insn_buffer_size < required_size) {
						_ir_insn_buffer_size += sizeof(shared::IRInstruction) * 256;
						assert(_ir_insn_buffer_size >= required_size);
						_ir_insns = (shared::IRInstruction *)realloc(_ir_insns, _ir_insn_buffer_size);
						assert(_ir_insns);
					}
				}	
		};
		}
	}
}

#endif	/* TRANSLATION_CONTEXT_H */
