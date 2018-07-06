/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * IROperand.h
 *
 *  Created on: 30 Sep 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_IROPERAND_H_
#define INC_BLOCKJIT_IROPERAND_H_

#include "blockjit/ir.h"

#include <functional>

namespace captive
{
	namespace shared
	{
		struct IROperand {
			enum IROperandType {
				NONE,
				CONSTANT,
				VREG,
				BLOCK,
				FUNC,
				PC
			};

			enum IRAllocationMode {
				NOT_ALLOCATED,
				ALLOCATED_REG,
				ALLOCATED_STACK
			};

			uint64_t value;
			uint16_t alloc_data : 14;
			IRAllocationMode alloc_mode : 2;
			IROperandType type : 4;

			// size in BYTES
			uint8_t size : 4;

			IROperand() : value(0), alloc_data(0), alloc_mode(NOT_ALLOCATED), type(NONE), size(0) { }

			IROperand(IROperandType type, uint64_t value, uint8_t size) : value(value), alloc_data(0), alloc_mode(NOT_ALLOCATED), type(type), size(size) { }

			inline bool is_allocated() const
			{
				return alloc_mode != NOT_ALLOCATED;
			}
			inline bool is_alloc_reg() const
			{
				return alloc_mode == ALLOCATED_REG;
			}
			inline bool is_alloc_stack() const
			{
				return alloc_mode == ALLOCATED_STACK;
			}

			inline void allocate(IRAllocationMode mode, uint16_t data)
			{
				alloc_mode = mode;
				alloc_data = data;
			}

			inline bool is_valid() const
			{
				return type != NONE;
			}
			inline bool is_constant() const
			{
				return type == CONSTANT;
			}
			inline bool is_vreg() const
			{
				return type == VREG;
			}
			inline bool is_block() const
			{
				return type == BLOCK;
			}
			inline bool is_func() const
			{
				return type == FUNC;
			}
			inline bool is_pc() const
			{
				return type == PC;
			}

			static IROperand none()
			{
				return IROperand(NONE, 0, 0);
			}

			static IROperand constant(uint64_t value, uint8_t size)
			{
				return IROperand(CONSTANT, value, size);
			}

			static IROperand const8(uint8_t value)
			{
				return IROperand(CONSTANT, value, 1);
			}
			static IROperand const16(uint16_t value)
			{
				return IROperand(CONSTANT, value, 2);
			}
			static IROperand const32(uint32_t value)
			{
				return IROperand(CONSTANT, value, 4);
			}
			static IROperand const64(uint64_t value)
			{
				return IROperand(CONSTANT, value, 8);
			}
			static IROperand const128(uint64_t value)
			{
				return IROperand(CONSTANT, value, 16);
			}

			static IROperand const_float(float value)
			{
				union {
					float f;
					uint32_t i;
				};
				f = value;
				return IROperand(CONSTANT, i, 4);
			}
			static IROperand const_double(double value)
			{
				union {
					double f;
					uint64_t i;
				};
				f = value;
				return IROperand(CONSTANT, i, 8);
			}

			static IROperand vreg(IRRegId id, uint8_t size)
			{
				return IROperand(VREG, (uint64_t)id, size);
			}

			static IROperand pc(uint32_t offset)
			{
				return IROperand(PC, (uint64_t)offset, 4);
			}

			static IROperand block(IRBlockId id)
			{
				return IROperand(BLOCK, (uint64_t)id, 0);
			}
			static IROperand func(void *addr)
			{
				return IROperand(FUNC, (uint64_t)addr, 0);
			}

			inline IRRegId get_vreg_idx() const
			{
				assert(is_vreg());
				return value;
			}

			bool operator==(const IROperand &other) const
			{
				if(value == other.value && alloc_mode == other.alloc_mode && alloc_data == other.alloc_data && type == other.type && size == other.size) return true;
				return false;
			}
		} __packed;
	}
}

namespace std
{
	using captive::shared::IROperand;

	template<> struct hash<IROperand> {
		size_t operator()(const IROperand &op) const
		{
			// bad hash
			return op.value;
		}
	};

}


#endif /* INC_BLOCKJIT_IROPERAND_H_ */
