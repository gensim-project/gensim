/*
 * TraceManager.h
 *
 *  Created on: 8 Aug 2014
 *      Author: harry
 */

#if 0
#ifndef TRACEMANAGER_H_
#define TRACEMANAGER_H_

#include "concurrent/Thread.h"
#include "concurrent/Mutex.h"
#include "concurrent/ConditionVariable.h"
#include "concurrent/ThreadsafeQueue.h"
#include "tracing/TraceTypes.h"
#include "util/SimOptions.h"

#include "libtrace/TraceSource.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
#include <fstream>

namespace archsim
{
	namespace abi
	{
		struct BinarySymbol;
	}
}

namespace gensim
{

	class BaseDisasm;
	class BaseDecode;
	class Processor;

	class TraceManager
	{
	public:

		inline void Trace_Insn(uint32_t PC, uint32_t IR, bool JIT, uint8_t isa_mode, uint8_t irq_mode, uint8_t exec)
		{
			assert(!IsTerminated() && !IsPacketOpen());

			InstructionHeaderRecord *header = (InstructionHeaderRecord*)(getNextPacket());
			*header = InstructionHeaderRecord(isa_mode, PC);

			InstructionCodeRecord *code = (InstructionCodeRecord*)(getNextPacket());
			*code = InstructionCodeRecord(irq_mode, IR);

			packet_open = true;
		}

		inline void Trace_End_Insn()
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			Tracing_Packet_Count++;
			packet_open = false;
		}

		/*
		 * Vector Register Operation Tracing
		 */
		inline void Trace_Vector_Bank_Reg_Read(bool Trace, uint8_t Bank, uint8_t Regnum, uint8_t Regindex, uint32_t Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			if(archsim::options::SimpleTrace) return;

			// XXX TODO
		}

		inline void Trace_Vector_Bank_Reg_Read(bool Trace, std::string BankName, uint8_t Regnum, uint8_t Regindex, uint32_t Value)
		{
			Trace_Vector_Bank_Reg_Read(Trace, get_regbank_id(BankName), Regnum, Regindex, Value);
		}

		inline void Trace_Vector_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, uint8_t Regindex, uint32_t Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			if (archsim::options::SimpleTrace) return;

			// XXX TODO
		}

		inline void Trace_Vector_Bank_Reg_Write(bool Trace, std::string BankName, uint8_t Regnum, uint8_t Regindex, uint32_t Value)
		{
			Trace_Vector_Bank_Reg_Write(Trace, get_regbank_id(BankName), Regnum, Regindex, Value);
		}

		/*
		 * Banked Register Operation Tracing
		 */

		inline void Trace_Bank_Reg_Read(bool Trace, uint8_t Bank, uint8_t Regnum, uint32_t Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			if (archsim::options::SimpleTrace) return;

			BankRegReadRecord *header = (BankRegReadRecord*)(getNextPacket());
			*header = BankRegReadRecord(Bank, Regnum, Value);
		}

		inline void Trace_Bank_Reg_Read(bool Trace, std::string BankName, uint8_t Regnum, uint32_t Value)
		{
			Trace_Bank_Reg_Read(Trace, get_regbank_id(BankName), Regnum, Value);
		}

		inline void Trace_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, uint32_t Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			if (archsim::options::SimpleTrace) return;

			BankRegWriteRecord *header = (BankRegWriteRecord*)(getNextPacket());
			*header = BankRegWriteRecord(Bank, Regnum, Value);
		}

		inline void Trace_Bank_Reg_Write(bool Trace, std::string BankName, uint8_t Regnum, uint32_t Value)
		{
			Trace_Bank_Reg_Write(Trace, get_regbank_id(BankName), Regnum, Value);
		}

		/*
		 * Register Operation Tracing
		 */
		inline void Trace_Reg_Read(bool Trace, uint8_t Regnum, uint32_t Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			if (archsim::options::SimpleTrace) return;

			RegReadRecord *header = (RegReadRecord*)(getNextPacket());
			*header = RegReadRecord(Regnum, Value);
		}

		inline void Trace_Reg_Read(bool Trace, std::string RegName, uint32_t Value)
		{
			Trace_Reg_Read(Trace, get_reg_id(RegName), Value);
		}

		inline void Trace_Reg_Write(bool Trace, uint8_t Regnum, uint32_t Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			if (archsim::options::SimpleTrace) return;

			RegWriteRecord *header = (RegWriteRecord*)(getNextPacket());
			*header = RegWriteRecord(Regnum, Value);
		}

		inline void Trace_Reg_Write(bool Trace, std::string RegName, uint32_t Value)
		{
			Trace_Reg_Write(Trace, get_reg_id(RegName), Value);
		}

		/*
		 * Memory Operation Tracing
		 */
		inline void Trace_Mem_Read(bool Trace, uint32_t Addr, uint32_t Value, uint32_t Width=4)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			if (archsim::options::SimpleTrace) return;

			MemReadAddrRecord *header = (MemReadAddrRecord*)(getNextPacket());
			*header = MemReadAddrRecord(Width, Addr);

			MemReadDataRecord *data = (MemReadDataRecord*)(getNextPacket());
			*data = MemReadDataRecord(Width, Value);
		}

		inline void Trace_Mem_Write(bool Trace, uint32_t Addr, uint32_t Value, uint32_t Width=4)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			if (archsim::options::SimpleTrace) return;

			MemWriteAddrRecord *header = (MemWriteAddrRecord*)(getNextPacket());
			*header = MemWriteAddrRecord(Width, Addr);

			MemWriteDataRecord *data = (MemWriteDataRecord*)(getNextPacket());
			*data = MemWriteDataRecord(Width, Value);
		}

		inline bool IsTerminated() const
		{
			return is_terminated;
		}

		inline bool IsPacketOpen() const
		{
			return packet_open;
		}

		void SetAggressiveFlush(bool b)
		{
			aggressive_flushing_ = b;
		}
		bool GetAggressiveFlush() const
		{
			return aggressive_flushing_;
		}

		void SetSink(TraceSink *sink)
		{
			sink_ = sink;
		}

		void Flush();

	protected:
		uint32_t IO_Packet_Count;
		uint32_t Tracing_Packet_Count;
		bool packet_open;

		gensim::Processor *cpu;
	private:
		TraceRecord *getNextPacket()
		{
			if(packet_buffer_pos == packet_buffer_end || GetAggressiveFlush()) EmitPackets();
			return packet_buffer_pos++;
		}

		TraceSink *sink_;

		std::vector<std::string> VectorRegBanks;
		std::vector<std::string> RegBanks;
		std::vector<std::string> Regs;

		TraceRecord *packet_buffer;
		TraceRecord *packet_buffer_pos;
		TraceRecord *packet_buffer_end;

		bool is_terminated;
		bool suppressed;
		bool aggressive_flushing_;

		TraceManager();

	protected:
		inline uint8_t get_reg_id(std::string reg_name)
		{
			std::vector<std::string>::iterator i = std::find(Regs.begin(), Regs.end(), reg_name);
			if (i == Regs.end()) {
				Regs.push_back(reg_name);
				return (uint8_t)(Regs.size() - 1);
			}
			return (uint8_t)std::distance(Regs.begin(), i);
		}

		inline std::string get_reg_name(uint32_t reg_id) const
		{
			assert(reg_id < Regs.size());
			return Regs[reg_id];
		}

		inline uint8_t get_regbank_id(const std::string reg_name)
		{
			std::vector<std::string>::iterator i = std::find(RegBanks.begin(), RegBanks.end(), reg_name);
			if (i == RegBanks.end()) {
				RegBanks.push_back(reg_name);
				return (uint8_t)(RegBanks.size() - 1);
			}
			return (uint8_t)std::distance(RegBanks.begin(), i);
		}

		inline std::string get_regbank_name(uint32_t reg_id) const
		{
			assert(reg_id < RegBanks.size());
			return RegBanks[reg_id];
		}

	};

	class BinaryFileTraceSink : public TraceSink
	{
	public:
		BinaryFileTraceSink(gensim::Processor *cpu, FILE *outfile);
		~BinaryFileTraceSink();

		void SinkPackets(const TraceRecord* start, const TraceRecord* end) override;
		void Flush() override;

	private:
		FILE *outfile_;
		std::vector<TraceRecord> records_;
	};

	class TextFileTraceSink : public TraceSink
	{
	public:
		TextFileTraceSink(gensim::Processor *cpu, FILE *outfile);
		~TextFileTraceSink();

		void SinkPackets(const TraceRecord* start, const TraceRecord* end) override;
		void Flush() override;

	private:
		void WritePacket(const TraceRecord *pkt);

		void WriteInstructionHeader(const InstructionHeaderRecord* record);
		void WriteInstructionCode(const InstructionCodeRecord* record);
		void WriteRegRead(const RegReadRecord* record);
		void WriteRegWrite(const RegWriteRecord* record);
		void WriteBankRegRead(const BankRegReadRecord* record);
		void WriteBankRegWrite(const BankRegWriteRecord* record);
		void WriteMemReadAddr(const MemReadAddrRecord* record);
		void WriteMemReadData(const MemReadDataRecord* record);
		void WriteMemWriteAddr(const MemWriteAddrRecord* record);
		void WriteMemWriteData(const MemWriteDataRecord* record);

		FILE *outfile_;
		gensim::Processor *cpu_;
		gensim::BaseDecode *decode_;

		uint32_t isa_mode_;
		uint32_t pc_;
	};
}


#endif /* TRACEMANAGER_H_ */
#endif