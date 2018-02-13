/*
 * File:   IJIR.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 14:45
 */

#ifndef IJIR_H
#define	IJIR_H

#include <string>
#include <list>
#include <vector>
#include <ostream>

namespace archsim
{
	namespace ij
	{
		namespace ir
		{
			class IRValue;
			class IRVariable;
			class IRInstruction;
			class IRBuilder;
			class IRConstant;
		}

		class IJIR;
		class IJIRBlock
		{
		public:
			IJIRBlock(IJIR& ir, std::string name = "");
			~IJIRBlock();

			ir::IRVariable *AllocateLocal(uint8_t size, std::string name = "");

			ir::IRInstruction *AddInstruction(ir::IRInstruction *insn);
			inline const std::list<ir::IRInstruction *>& GetInstructions() const
			{
				return instructions;
			}

			inline std::string GetName() const
			{
				return name;
			}

			void Dump(std::ostream& stream) const;

		private:
			IJIR& ir;
			std::string name;
			std::list<ir::IRValue *> local_values;
			std::list<ir::IRInstruction *> instructions;
		};

		class IJIR
		{
		public:
			IJIR(uint8_t nr_params = 0);
			~IJIR();

			inline std::string GetUniqueName() const
			{
				return std::to_string(next_name++);
			}

			ir::IRValue *GetParameterValue(uint8_t idx);
			ir::IRVariable *AllocateGlobal(uint8_t size, std::string name = "");

			inline ir::IRConstant *GetConstantInt8(uint8_t value)
			{
				return GetConstantInt(1, value);
			}
			inline ir::IRConstant *GetConstantInt16(uint16_t value)
			{
				return GetConstantInt(2, value);
			}
			inline ir::IRConstant *GetConstantInt32(uint32_t value)
			{
				return GetConstantInt(4, value);
			}
			inline ir::IRConstant *GetConstantInt64(uint64_t value)
			{
				return GetConstantInt(8, value);
			}

			IJIRBlock *CreateBlock(std::string name = "");

			inline IJIRBlock& GetEntryBlock() const
			{
				return *entry_block;
			}
			inline const std::list<IJIRBlock *>& GetBlocks() const
			{
				return blocks;
			}
			inline ir::IRBuilder& GetBuilder() const
			{
				return *builder;
			}

			void Dump(std::ostream& stream) const;

		private:
			ir::IRConstant *GetConstantInt(uint8_t size, uint64_t value);

			IJIRBlock *entry_block;
			ir::IRBuilder *builder;
			std::vector<ir::IRValue *> params;
			std::list<IJIRBlock *> blocks;
			std::list<ir::IRValue *> global_values;

			mutable uint8_t next_name;
		};
	}
}

#endif	/* IJIR_H */

