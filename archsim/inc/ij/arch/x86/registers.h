/*
 * File:   registers.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 10:45
 */

#ifndef REGISTERS_H
#define	REGISTERS_H

#ifndef X86_MACROS_INTERNAL
#error "Do not include this file directly"
#endif

namespace x86
{
	namespace reg
	{
		enum RegId {
			// General purpose registers
			RegAL, RegAX, RegEAX, RegRAX, RegAH,
			RegCL, RegCX, RegECX, RegRCX, RegCH,
			RegDL, RegDX, RegEDX, RegRDX, RegDH,
			RegBL, RegBX, RegEBX, RegRBX, RegBH,

			GP_REG_END,

			// General purpose registers high
			RegR8B, RegR8W, RegR8D, RegR8,
			RegR9B, RegR9W, RegR9D, RegR9,
			RegR10B, RegR10W, RegR10D, RegR10,
			RegR11B, RegR11W, RegR11D, RegR11,
			RegR12B, RegR12W, RegR12D, RegR12,
			RegR13B, RegR13W, RegR13D, RegR13,
			RegR14B, RegR14W, RegR14D, RegR14,
			RegR15B, RegR15W, RegR15D, RegR15,

			GP_REG_HIGH_END,

			RegSIL, RegSI, RegESI, RegRSI,		// Source index
			RegDIL, RegDI, RegEDI, RegRDI,		// Dest index

			RegSPL, RegSP, RegESP, RegRSP,		// Stack Pointer
			RegBPL, RegBP, RegEBP, RegRBP,		// Base Pointer

			// Special Registers
			RegRIP,								// Instruction Pointer
			RegRIZ,								// Virtual ZERO register for SIBs

			// End marker
			RegCOUNT
		};


		class RegInfo
		{
		public:
			RegInfo();

			inline uint8_t RN(RegId reg) const
			{
				assert(rn[reg] != RegCOUNT);
				return rn[reg];
			}
			inline uint8_t RS(RegId reg) const
			{
				assert(rs[reg] != RegCOUNT);
				return rs[reg];
			}

		private:
			uint8_t rn[RegCOUNT];
			uint8_t rs[RegCOUNT];
		};

		extern const RegInfo info;


		inline static uint8_t __attribute__((pure)) RN(RegId reg, bool safe=0)
		{
			assert(safe || (reg != GP_REG_END && reg != GP_REG_HIGH_END));

			switch(reg) {
				case RegAL:
				case RegAX:
				case RegEAX:
				case RegRAX:
					return 0;

				case RegCL:
				case RegCX:
				case RegECX:
				case RegRCX:
					return 1;

				case RegDL:
				case RegDX:
				case RegEDX:
				case RegRDX:
					return 2;

				case RegBL:
				case RegBX:
				case RegEBX:
				case RegRBX:
					return 3;

				case RegAH:
				case RegSPL:
				case RegSP:
				case RegESP:
				case RegRSP:
					return 4;

				case RegCH:
				case RegBPL:
				case RegBP:
				case RegEBP:
				case RegRBP:
				case RegRIP:
					return 5;

				case RegDH:
				case RegSIL:
				case RegSI:
				case RegESI:
				case RegRSI:
					return 6;

				case RegBH:
				case RegDIL:
				case RegDI:
				case RegEDI:
				case RegRDI:
					return 7;

				case RegR8B:
				case RegR8W:
				case RegR8D:
				case RegR8:
					return 0;

				case RegR9B:
				case RegR9W:
				case RegR9D:
				case RegR9:
					return 1;

				case RegR10B:
				case RegR10W:
				case RegR10D:
				case RegR10:
					return 2;

				case RegR11B:
				case RegR11W:
				case RegR11D:
				case RegR11:
					return 3;

				case RegR12B:
				case RegR12W:
				case RegR12D:
				case RegR12:
					return 4;

				case RegR13B:
				case RegR13W:
				case RegR13D:
				case RegR13:
					return 5;

				case RegR14B:
				case RegR14W:
				case RegR14D:
				case RegR14:
					return 6;

				case RegR15B:
				case RegR15W:
				case RegR15D:
				case RegR15:
					return 7;

				case RegRIZ:
					return 4;

				default:
					assert(safe || !"Unable to classify unknown register.");
			}

			return RegCOUNT;
		}

		inline static bool __attribute__((pure)) HR(RegId reg)
		{
			assert(reg != GP_REG_END && reg != GP_REG_HIGH_END && reg != RegRIZ);
			return ((reg > GP_REG_END) && (reg < GP_REG_HIGH_END));
		}

		inline static bool __attribute__((pure)) NEWREG(RegId reg)
		{
			return reg == RegSPL || reg == RegBPL || reg == RegSIL || reg == RegDIL;
		}

		inline static int __attribute__((pure)) RS(RegId reg, bool safe=0)
		{
			assert(safe || (reg != GP_REG_END && reg != GP_REG_HIGH_END && reg != RegRIZ));

			switch(reg) {
				case RegAL:
				case RegBL:
				case RegCL:
				case RegDL:
				case RegAH:
				case RegBH:
				case RegCH:
				case RegDH:
				case RegSPL:
				case RegBPL:
				case RegSIL:
				case RegDIL:
				case RegR8B:
				case RegR9B:
				case RegR10B:
				case RegR11B:
				case RegR12B:
				case RegR13B:
				case RegR14B:
				case RegR15B:
					return 1;

				case RegAX:
				case RegBX:
				case RegCX:
				case RegDX:
				case RegSP:
				case RegBP:
				case RegSI:
				case RegDI:
				case RegR8W:
				case RegR9W:
				case RegR10W:
				case RegR11W:
				case RegR12W:
				case RegR13W:
				case RegR14W:
				case RegR15W:
					return 2;

				case RegEAX:
				case RegEBX:
				case RegECX:
				case RegEDX:
				case RegESP:
				case RegEBP:
				case RegESI:
				case RegEDI:
				case RegR8D:
				case RegR9D:
				case RegR10D:
				case RegR11D:
				case RegR12D:
				case RegR13D:
				case RegR14D:
				case RegR15D:
					return 4;

				case RegRAX:
				case RegRBX:
				case RegRCX:
				case RegRDX:
				case RegRSP:
				case RegRBP:
				case RegRSI:
				case RegRDI:
				case RegR8:
				case RegR9:
				case RegR10:
				case RegR11:
				case RegR12:
				case RegR13:
				case RegR14:
				case RegR15:
				case RegRIP:
					return 8;

				default:
					assert(safe || !"Unable to size unknown register.");
			}

			return RegCOUNT;
		}
	}

}

#endif	/* REGISTERS_H */

