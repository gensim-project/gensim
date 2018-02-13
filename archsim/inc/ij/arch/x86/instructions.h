/*
 * File:   instructions.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 10:48
 */

#ifndef INSTRUCTIONS_H
#define	INSTRUCTIONS_H

#ifndef X86_MACROS_INTERNAL
#error "Do not include this file directly"
#endif

#include "ij/arch/x86/registers.h"

namespace x86
{
	namespace insn
	{
		using namespace reg;

		struct MemAccessInfo {
		public:
			int32_t displ;
			uint32_t scale;

			RegId base;
			RegId index;

			static MemAccessInfo get(RegId base, RegId index, uint32_t scale, int32_t displ = 0)
			{
				assert(index == RegRIZ || (RS(base) == RS(index)));

				MemAccessInfo info;
				info.base = base;
				info.index = index;
				info.scale = scale;
				info.displ = displ;

				return info;
			};

			static MemAccessInfo get(RegId base, int32_t displ = 0)
			{
				return get(base, RegRIZ, 0, displ);
			};
		};

		enum ModRMDirection {
			RegToRM,
			RMToReg,
		};

		template<typename src_t, typename dst_t>
		struct X86InstructionModRM {
			src_t source;
			dst_t dest;

			uint16_t opcode;
			ModRMDirection direction;

			static X86InstructionModRM get(uint16_t opcode, src_t source, dst_t dest, ModRMDirection dir)
			{
				X86InstructionModRM insn;
				insn.opcode = opcode;
				insn.source = source;
				insn.dest = dest;
				insn.direction = dir;

				return insn;
			}
		};

		static inline void EMIT(uint8_t *&code_buffer, const X86InstructionModRM<RegId, RegId> &insn)
		{
			// --- Only permit same-sized register operations
			assert(RS(insn.source) == RS(insn.dest));

			RegId reg_reg = insn.direction == RegToRM ? insn.source : insn.dest;
			RegId rm_reg = insn.direction == RegToRM ? insn.dest : insn.source;

			assert(!NEWREG(rm_reg) && "8-bit new registers not permitted as RM component");

			// --- Prepare fields
			uint8_t mod = 3;				// Register -> Register
			uint8_t reg = RN(reg_reg);	// Source Register
			uint8_t rm = RN(rm_reg);		// Destination Register

			uint8_t b = !!HR(rm_reg);
			uint8_t x = 0;
			uint8_t r = !!HR(reg_reg);
			uint8_t w = !!(RS(rm_reg) == 8);

			// --- Check for 16-bit registers
			if (RS(rm_reg) == 2)
				E(0x66);

			// --- REX prefix
			if (NEWREG(reg_reg) || b || x || r || w)
				EMIT_REX_BYTE(code_buffer, b, x, r, w);

			// --- Emit the OPCODE
			if (insn.opcode > 0xff) {
				E(0x0f);
			}
			E(insn.opcode & 0xff);

			// --- Emit the MODRM
			EMIT_MODRM(code_buffer, mod, reg, rm);
		}

		static inline void EMIT_MEM(uint8_t *&code_buffer, uint16_t op, const RegId xreg, const MemAccessInfo &mem)
		{
			assert(mem.base != RegRSP);
			/*assert(mem.base != RegR12B);
			assert(mem.base != RegR12W);
			assert(mem.base != RegR12D);
			assert(mem.base != RegR12);*/

			uint8_t mod, reg, rm;

			// If there is no displacement, and the base register is not RBP, we can use an EADDR
			// encoding.  Alternatively, if we're trying to do a PC-relative operation, we *must* use the EADDR encoding.
			if ((mem.displ == 0 && mem.base != RegRBP && mem.base != RegR13) || mem.base == RegRIP) mod = 0;
			// If the displacement fits in an 8-bit integer, we can use a DISP8 encoding.
			else if ((mem.displ > INT8_MIN) && (mem.displ < INT8_MAX)) mod = 1;
			// Otherwise, we need to use a DISP32 encoding.
			else mod = 2;

			reg = RN(xreg);
			rm = RN(mem.base);

			// If we want a scale, then we need to use a SIB byte, which means we override the RM field.
			if (mem.scale != 0) rm = 4;

			uint8_t b = !!HR(mem.base);
			uint8_t x = !!(mem.scale != 0 && HR(mem.index));
			uint8_t r = !!HR(xreg);
			uint8_t w = !!(RS(xreg) == 8);

			// --- Emissions

			// --- Emit any prefixes
			if (RS(mem.base) == 4) E(0x67);		// Address-size override
			if (RS(xreg) == 2) E(0x66);					// 16-bit operands

			// --- Emit a REX
			// --- REX prefix
			if (NEWREG(xreg) || b || x || r || w)
				EMIT_REX_BYTE(code_buffer, b, x, r, w);

			// --- Emit the opcode
			if (op > 0xff) {
				E(0x0f);
			}
			E(op & 0xff);

			// --- Emit the MODRM
			EMIT_MODRM(code_buffer, mod, reg, rm);

			// If we're emitting a SIB byte...
			if (rm == 4) {
				uint32_t true_scale = mem.scale;
				RegId true_index = mem.index;
				if (mem.base == RegR12 && mem.scale == 0) {
					true_scale= 1;
					true_index = RegRIZ;
				}

				uint8_t ss = 0;
				switch(true_scale) {
					case 1:
						ss = 0;
						break;
					case 2:
						ss = 1;
						break;
					case 4:
						ss = 2;
						break;
					case 8:
						ss = 3;
						break;
					default:
						assert(false && "Invalid scale");
				}

				uint8_t index = RN(true_index);
				uint8_t base = RN(mem.base);

				assert(base != 5);

				EMIT_SIB(code_buffer, ss, index, base);
			}

			// Emit an 8-bit displacement.
			if (mod == 1) EMIT_CONSTANT<1>(code_buffer, mem.displ);
			// Or, emit a 32-bit displacement.
			else if (mod == 2) EMIT_CONSTANT<4>(code_buffer, mem.displ);
		}

		static inline void EMIT(uint8_t *&code_buffer, const X86InstructionModRM<RegId, MemAccessInfo> &insn)
		{
			EMIT_MEM(code_buffer, insn.opcode, insn.source, insn.dest);
		}

		static inline void EMIT(uint8_t *&code_buffer, const X86InstructionModRM<MemAccessInfo, RegId> &insn)
		{
			EMIT_MEM(code_buffer, insn.opcode, insn.dest, insn.source);
		}
	}

}
#endif	/* INSTRUCTIONS_H */

