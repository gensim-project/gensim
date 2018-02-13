/*
 * File:   X86Support.h
 * Author: spink
 *
 * Created on 15 September 2014, 18:35
 */

#ifndef X86SUPPORT_H
#define	X86SUPPORT_H

#include "define.h"
#include <string>

namespace archsim
{
	namespace ij
	{
		namespace arch
		{
			namespace x86
			{
				class X86Operand
				{
				public:
					X86Operand(uint8_t size) : size(size) { }

					inline uint8_t GetSize() const
					{
						return size;
					}

				private:
					uint8_t size;
				};

				class X86Register : public X86Operand
				{
				public:
					X86Register(uint8_t size, uint8_t idx, std::string name, bool high_reg = false, bool new_reg = false) : X86Operand(size), idx(idx), name(name), hr(high_reg), nr(new_reg) { }

					enum GPRegister {
						GPR_A, GPR_C, GPR_D, GPR_B, GPR_SP, GPR_BP, GPR_SI, GPR_DI, GPR_R8, GPR_R9, GPR_R10, GPR_R11, GPR_R12, GPR_R13, GPR_R14, GPR_R15
					};

					static const X86Register& GetSizedRegister(GPRegister reg, uint8_t size)
					{
						switch (reg) {
							case GPR_A:
								switch (size) {
									case 1:
										return AL;
									case 2:
										return AX;
									case 4:
										return EAX;
									case 8:
										return RAX;
									default:
										assert(false && "Invalid size for GP register");
										break;
								}
								break;
							case GPR_C:
								switch (size) {
									case 1:
										return CL;
									case 2:
										return CX;
									case 4:
										return ECX;
									case 8:
										return RCX;
									default:
										assert(false && "Invalid size for GP register");
										break;
								}
								break;
							case GPR_D:
								switch (size) {
									case 1:
										return DL;
									case 2:
										return DX;
									case 4:
										return EDX;
									case 8:
										return RDX;
									default:
										assert(false && "Invalid size for GP register");
										break;
								}
								break;
							case GPR_B:
								switch (size) {
									case 1:
										return BL;
									case 2:
										return BX;
									case 4:
										return EBX;
									case 8:
										return RBX;
									default:
										assert(false && "Invalid size for GP register");
										break;
								}
								break;

							default:
								assert(false && "Invalid GP register");
								break;
						}
						__builtin_unreachable();
					}

					inline bool IsHighReg() const
					{
						return hr;
					}
					inline bool IsNewReg() const
					{
						return nr;
					}
					inline uint8_t GetIndex() const
					{
						return idx;
					}

					static const X86Register RAX;
					static const X86Register RCX;
					static const X86Register RDX;
					static const X86Register RBX;
					static const X86Register RSP;
					static const X86Register RBP;
					static const X86Register RSI;
					static const X86Register RDI;

					static const X86Register R8;
					static const X86Register R9;
					static const X86Register R10;
					static const X86Register R11;
					static const X86Register R12;
					static const X86Register R13;
					static const X86Register R14;
					static const X86Register R15;

					static const X86Register EAX;
					static const X86Register ECX;
					static const X86Register EDX;
					static const X86Register EBX;
					static const X86Register ESP;
					static const X86Register EBP;
					static const X86Register ESI;
					static const X86Register EDI;

					static const X86Register R8D;
					static const X86Register R9D;
					static const X86Register R10D;
					static const X86Register R11D;
					static const X86Register R12D;
					static const X86Register R13D;
					static const X86Register R14D;
					static const X86Register R15D;

					static const X86Register AX;
					static const X86Register BX;
					static const X86Register CX;
					static const X86Register DX;
					static const X86Register SP;
					static const X86Register BP;
					static const X86Register SI;
					static const X86Register DI;

					static const X86Register R8W;
					static const X86Register R9W;
					static const X86Register R10W;
					static const X86Register R11W;
					static const X86Register R12W;
					static const X86Register R13W;
					static const X86Register R14W;
					static const X86Register R15W;

					static const X86Register AH;
					static const X86Register BH;
					static const X86Register CH;
					static const X86Register DH;
					static const X86Register AL;
					static const X86Register BL;
					static const X86Register CL;
					static const X86Register DL;

					static const X86Register SPL;
					static const X86Register BPL;
					static const X86Register SIL;
					static const X86Register DIL;

					static const X86Register R8B;
					static const X86Register R9B;
					static const X86Register R10B;
					static const X86Register R11B;
					static const X86Register R12B;
					static const X86Register R13B;
					static const X86Register R14B;
					static const X86Register R15B;

				private:
					uint8_t idx;
					std::string name;
					bool hr, nr;
				};

				class X86Memory : public X86Operand
				{
				public:
					X86Memory(uint8_t size, const X86Register& base, const X86Register& index, uint8_t scale, uint32_t displacement)
						: X86Operand(size), base(base), index(index), scale(scale), displacement(displacement) { }

				private:
					const X86Register& base;
					const X86Register& index;
					uint8_t scale;
					uint32_t displacement;
				};

				class X86Immediate : public X86Operand
				{
				public:
					X86Immediate(uint8_t size, uint64_t value) : X86Operand(size), value(value) { }

				private:
					uint64_t value;
				};
			}
		}
	}
}

#endif	/* X86SUPPORT_H */

