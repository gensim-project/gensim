/*
 * File:   X86OutputBuffer.h
 * Author: spink
 *
 * Created on 15 September 2014, 18:28
 */

#ifndef X86OUTPUTBUFFER_H
#define	X86OUTPUTBUFFER_H

#include "ij/IJCompiler.h"
#include "X86Support.h"

namespace archsim
{
	namespace ij
	{
		namespace arch
		{
			namespace x86
			{
				class X86Register;
				class X86Memory;
				class X86Immediate;

				class X86OutputBuffer : public OutputBuffer<uint8_t>
				{
				public:
					X86OutputBuffer(uint8_t *start) : OutputBuffer(start) { }

					void mov(const X86Register& source, const X86Register& dest);
					void mov(const X86Register& source, const X86Memory& dest);
					void mov(const X86Memory& source, const X86Register& dest);
					void mov(const X86Immediate& source, const X86Register& dest);
					void mov(const X86Immediate& source, const X86Memory& dest);

					void _xor(const X86Register& source, const X86Register& dest);
					void _xor(const X86Immediate& source, const X86Register& dest);
					void _xor(const X86Memory& source, const X86Register& dest);
					void _xor(const X86Register& source, const X86Memory& dest);

					void add(const X86Register& source, const X86Register& dest);

					inline void ret()
					{
						E(0xc7);
					}
					inline void int3()
					{
						E(0xcc);
					}

				private:
					void reg2reg(uint16_t op, const X86Register& src, const X86Register& dst);
					void reg2mem(uint16_t op, const X86Register& src, const X86Memory& dst);
					void mem2reg(uint16_t op, const X86Memory& src, const X86Register& dst);
					void imm2reg(uint16_t op, const X86Immediate& src, const X86Register& dst);
					void imm2mem(uint16_t op, const X86Immediate& src, const X86Memory& dst);

					inline void rex(bool b, bool x, bool r, bool w)
					{
						uint8_t rexval = 0x40;

						if (b) rexval |= 0x01;
						if (x) rexval |= 0x02;
						if (r) rexval |= 0x04;
						if (w) rexval |= 0x08;

						E(rexval);
					}

					inline void oper_size_override()
					{
						E(0x00);
					}
					inline void addr_size_override()
					{
						E(0x00);
					}

					inline void mod_rm(int mod, int reg, int rm)
					{
						uint8_t mod_rm_val = (mod & 3) << 6;
						mod_rm_val |= (reg & 7) << 3;
						mod_rm_val |= (rm & 7);

						E(mod_rm_val);
					}

					inline void sib(int scale, int index, int base)
					{
						uint8_t sib_val = (scale & 3) << 6;
						sib_val |= (index & 7) << 3;
						sib_val |= (base & 7);

						E(sib_val);
					}

					inline void opcode(uint16_t op)
					{
						if (op > 0xff) E(0xf0);
						E(op & 0xff);
					}

					inline void const8(uint64_t v)
					{
						E64(v);
					}

					inline void const4(uint32_t v)
					{
						E32(v);
					}

					inline void const2(uint16_t v)
					{
						E16(v);
					}

					inline void const1(uint8_t v)
					{
						E8(v);
					}
				};
			}
		}
	}
}

#endif	/* X86OUTPUTBUFFER_H */

