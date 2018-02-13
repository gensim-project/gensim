/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   MachineState.h
 * Author: harry
 *
 * Created on 22 April 2017, 08:47
 */

#ifndef MACHINESTATE_H
#define MACHINESTATE_H

#include <map>
#include <cstdint>
#include <vector>
#include <sstream>
#include <unordered_map>

#include "genC/ir/IRConstant.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			namespace testing
			{
				class InstructionState
				{
				public:
					IRConstant GetField(const std::string &field) const
					{
						return instruction_.at(field);
					}
					void SetField(const std::string &field, const IRConstant &value)
					{
						instruction_[field] = value;
					}
					
					std::string Dump() const
					{
						std::stringstream str;
						str << "Instruction: " << std::endl;
						for(auto i : instruction_) {
							str << "\t" << i.first << " = " << std::hex << i.second.Int() << std::endl;
						}
						return str.str();
					}
				private:
					std::unordered_map<std::string, IRConstant> instruction_;
				};

				class MemoryState
				{
				public:
					void Reset();

					virtual bool Equals(MemoryState &other);

					void Read(uint64_t addr, size_t size, uint8_t *data);
					void Write(uint64_t addr, size_t size, uint8_t *data);

					void WriteByte(uint64_t addr, uint8_t value);
					uint8_t ReadByte(uint64_t addr);
					std::string Dump();
				private:
					std::map<uint64_t, uint8_t> _data;
				};

				class RegisterFileState
				{
				public:
					RegisterFileState();

					void Write64(uint32_t offset, uint64_t data)
					{
						Write(offset, 8, (uint8_t*)&data);
					}
					void Write32(uint32_t offset, uint32_t data)
					{
						Write(offset, 4, (uint8_t*)&data);
					}
					void Write16(uint32_t offset, uint16_t data)
					{
						Write(offset, 2, (uint8_t*)&data);
					}
					void Write8 (uint32_t offset, uint8_t data)
					{
						Write(offset, 1, (uint8_t*)&data);
					}

					uint64_t Read64(uint32_t offset)
					{
						uint64_t data;
						Read(offset, 8, (uint8_t*)&data);
						return data;
					}
					uint32_t Read32(uint32_t offset)
					{
						uint32_t data;
						Read(offset, 4, (uint8_t*)&data);
						return data;
					}
					uint16_t Read16(uint32_t offset)
					{
						uint16_t data;
						Read(offset, 2, (uint8_t*)&data);
						return data;
					}
					uint8_t  Read8 (uint32_t offset)
					{
						uint8_t data;
						Read(offset, 1, (uint8_t*)&data);
						return data;
					}

					virtual void Read(uint32_t offset, size_t size, uint8_t *data) = 0;
					virtual void Write(uint32_t offset, size_t size, const uint8_t *data) = 0;
					virtual void Reset() = 0;
					
					virtual bool IsSet(uint32_t offset) = 0;

					virtual size_t Size() const = 0;
					virtual void SetSize(size_t size) = 0;
					bool Wrap()
					{
						return _wrap;
					}
					void SetWrap(bool wrap)
					{
						_wrap = wrap;
					}

					virtual bool Equals(RegisterFileState &other);
					std::string Dump();
				private:
					bool _wrap;
				};

				class BasicRegisterFileState : public RegisterFileState
				{
				public:
					BasicRegisterFileState();

					virtual void Read(uint32_t offset, size_t size, uint8_t *data) override;
					virtual void Write(uint32_t offset, size_t size, const uint8_t *data) override;
					virtual void Reset() override;

					bool IsSet(uint32_t offset) override;

					size_t Size() const override;
					virtual void SetSize(size_t size) override;

					bool Equals(RegisterFileState& other) override;

				private:
					void WriteByte(uint32_t offset, uint8_t data);
					uint8_t ReadByte(uint32_t offset);
					std::map<uint32_t, uint8_t> data_;
					uint32_t size_;
				};

				class MachineStateInterface
				{
				public:
					virtual RegisterFileState &RegisterFile() = 0;
					virtual MemoryState &Memory() = 0;
					virtual InstructionState &Instruction() = 0;
					void Reset();

					virtual bool Equals(MachineStateInterface &other);
					std::string Dump();
				};

				template <typename regfile_type, typename mem_type> class MachineState : public MachineStateInterface
				{
				public:


					virtual RegisterFileState &RegisterFile() override
					{
						return _register_file;
					}
					virtual MemoryState &Memory() override
					{
						return _memory;
					}

					virtual InstructionState &Instruction() override
					{
						return instruction_;
					}
				private:
					regfile_type _register_file;
					mem_type _memory;
					InstructionState instruction_;
				};
			}
		}
	}
}

#endif /* MACHINESTATE_H */

