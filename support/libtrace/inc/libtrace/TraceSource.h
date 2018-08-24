/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#ifndef TRACESOURCE_H_
#define TRACESOURCE_H_

#include "RecordTypes.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

namespace libtrace
{
	class TraceSink;

	class TraceSource
	{
	public:
		static const size_t RecordBufferSize = 1024 * 128;
		static const size_t PacketBufferSize = 1024;

		TraceSource(uint32_t BufferSize);
		virtual ~TraceSource();

		typedef std::string reg_name_t;

		virtual void Terminate();
		void EmitPackets();

	private:
		template <typename PCT> void TraceInstructionHeader(PCT pc, uint8_t isa_mode);
		template <typename CodeT> void TraceInstructionCode(CodeT pc, uint8_t irq_mode);
		template <typename PCT> void TraceBundleHeader(PCT pc);

	public:
		template<typename PCT> void Trace_StartBundle(PCT PC)
		{
			assert(!IsTerminated() && !IsPacketOpen());
			TraceBundleHeader(PC);
		}

		template<typename PCT, typename CodeT> void Trace_Insn(PCT PC, CodeT IR, bool JIT, uint8_t isa_mode, uint8_t irq_mode, uint8_t exec)
		{
			assert(!IsTerminated() && !IsPacketOpen());

			TraceInstructionHeader(PC, isa_mode);
			TraceInstructionCode(IR, irq_mode);

			packet_open_ = true;
		}

		inline void Trace_End_Insn()
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());
			packet_open_ = false;
		}

		/*
		 * Vector Register Operation Tracing
		 */
		inline void Trace_Vector_Bank_Reg_Read(bool Trace, uint8_t Bank, uint8_t Regnum, uint8_t Regindex, uint32_t Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());

			// XXX TODO
		}

		inline void Trace_Vector_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, uint8_t Regindex, uint32_t Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());

			// XXX TODO
		}

		/*
		 * Banked Register Operation Tracing
		 */

	private:
		uint32_t getDataWord(char *data, uint32_t total_size, uint32_t word_idx)
		{
			assert(word_idx <= getExtensionCount(total_size));

			uint32_t out_data = 0;
			memcpy((char*)&out_data, data + (word_idx * 4), std::min(total_size, 4U));
			return out_data;
		}

		uint32_t getExtensionCount(uint32_t size)
		{
			if(size <= 4) {
				return 0;
			}
			return (size / 4) - 1;
		}

	public:
		void Trace_Bank_Reg_Read(bool Trace, uint8_t Bank, uint8_t Regnum, char *data, uint32_t size)
		{
			if(!IsPacketOpen()) {
				return;
			}
			assert(!IsTerminated() && IsPacketOpen());

			int extension_count = getExtensionCount(size);

			BankRegReadRecord *header = (BankRegReadRecord*)(getNextPacket());
			*header = BankRegReadRecord(Bank, Regnum, getDataWord(data, size, 0), extension_count);

			for(int i = 0; i < extension_count; ++i) {
				DataExtensionRecord *extension = (DataExtensionRecord*)getNextPacket();
				*extension = DataExtensionRecord(BankRegRead, getDataWord(data, size, i+1));
			}
		}
		void Trace_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, char *data, uint32_t size)
		{
			if(!IsPacketOpen()) {
				return;
			}
			assert(!IsTerminated() && IsPacketOpen());

			int extension_count = getExtensionCount(size);

			BankRegWriteRecord *header = (BankRegWriteRecord*)(getNextPacket());
			*header = BankRegWriteRecord(Bank, Regnum, getDataWord(data, size, 0), extension_count);

			for(int i = 0; i < extension_count; ++i) {
				DataExtensionRecord *extension = (DataExtensionRecord*)getNextPacket();
				*extension = DataExtensionRecord(BankRegWrite, getDataWord(data, size, i+1));
			}
		}

		template<typename T> void Trace_Bank_Reg_Read(bool Trace, uint8_t Bank, uint8_t Regnum, T Value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());

			BankRegReadRecord *header = (BankRegReadRecord*)(getNextPacket());
			*header = BankRegReadRecord(Bank, Regnum, Value, 0);
		}

		template<typename T> void Trace_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, T value)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());

			BankRegWriteRecord *header = (BankRegWriteRecord*)(getNextPacket());
			*header = BankRegWriteRecord(Bank, Regnum, value, 0);
		}

		/*
		 * Register Operation Tracing
		 */
		template <typename T> void Trace_Reg_Read(bool Trace, uint8_t Regnum, T Value);
		template <typename T> void Trace_Reg_Write(bool Trace, uint8_t Regnum, T Value);

		/*
		 * Memory Operation Tracing
		 */
	private:
		template<typename T> void TraceMemReadAddr(T Addr, uint32_t Width);
		template<typename T> void TraceMemReadData(T Data, uint32_t Width);

	public:
		template<typename AddrT, typename DataT> void Trace_Mem_Read(bool Trace, AddrT Addr, DataT Value, uint32_t Width=4)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());

			TraceMemReadAddr<AddrT>(Addr, Width);
			TraceMemReadData<DataT>(Value, Width);
		}

	private:
		template<typename T> void TraceMemWriteAddr(T Addr, uint32_t Width);
		template<typename T> void TraceMemWriteData(T Data, uint32_t Width);

	public:
		template<typename AddrT, typename DataT> void Trace_Mem_Write(bool Trace, AddrT Addr, DataT Value, uint32_t Width=4)
		{
			if(!IsPacketOpen()) return;
			assert(!IsTerminated() && IsPacketOpen());

			TraceMemWriteAddr(Width, Addr);
			TraceMemWriteData(Width, Value);
		}

		inline bool IsTerminated() const
		{
			return is_terminated_;
		}

		inline bool IsPacketOpen() const
		{
			return packet_open_;
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
		bool packet_open_;

	private:
		TraceRecord *getNextPacket()
		{
			if(packet_buffer_pos_ == packet_buffer_end_ || GetAggressiveFlush()) EmitPackets();
			return packet_buffer_pos_++;
		}

		TraceSink *sink_;

		TraceRecord *packet_buffer_;
		TraceRecord *packet_buffer_pos_;
		TraceRecord *packet_buffer_end_;

		bool is_terminated_;
		bool aggressive_flushing_;

		TraceSource();
	};

	template<> inline void TraceSource::TraceBundleHeader(uint32_t pc)
	{
		*(InstructionBundleHeaderRecord*)getNextPacket() = InstructionBundleHeaderRecord(pc, 0);
	}
	template<> inline void TraceSource::TraceBundleHeader(uint64_t pc)
	{
		*(InstructionBundleHeaderRecord*)getNextPacket() = InstructionBundleHeaderRecord(pc, 1);
		*(DataExtensionRecord*)getNextPacket() = DataExtensionRecord(InstructionBundleHeader, pc >> 32);
	}

	template <> inline void TraceSource::TraceInstructionHeader(uint32_t pc, uint8_t isa_mode)
	{
		auto *header = (InstructionHeaderRecord*)getNextPacket();
		*header = InstructionHeaderRecord(isa_mode, pc, 0);
	}
	template <> inline void TraceSource::TraceInstructionHeader(uint64_t pc, uint8_t isa_mode)
	{
		auto *header = (InstructionHeaderRecord*)getNextPacket();
		*header = InstructionHeaderRecord(isa_mode, pc, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(InstructionHeader, pc >> 32);

	}

	template <> inline void TraceSource::TraceInstructionCode(uint32_t ir, uint8_t irq_mode)
	{
		auto *header = (InstructionCodeRecord*)getNextPacket();
		*header = InstructionCodeRecord(irq_mode, ir, 0);
	}
	template <> inline void TraceSource::TraceInstructionCode(uint64_t ir, uint8_t irq_mode)
	{
		auto *header = (InstructionCodeRecord*)getNextPacket();
		*header = InstructionCodeRecord(irq_mode, ir, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(InstructionCode, ir >> 32);

	}

	template<> inline void TraceSource::Trace_Bank_Reg_Read(bool Trace, uint8_t Bank, uint8_t Regnum, float fValue)
	{
		uint32_t *pValue = (uint32_t*)&fValue;
		uint32_t Value = *pValue;

		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		BankRegReadRecord *header = (BankRegReadRecord*)(getNextPacket());
		*header = BankRegReadRecord(Bank, Regnum, Value, 0);
	}
	template<> inline void TraceSource::Trace_Bank_Reg_Read(bool Trace, uint8_t Bank, uint8_t Regnum, double fValue)
	{
		uint64_t *pValue = (uint64_t*)&fValue;
		uint64_t Value = *pValue;

		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		BankRegReadRecord *header = (BankRegReadRecord*)(getNextPacket());
		*header = BankRegReadRecord(Bank, Regnum, Value, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(BankRegRead, Value >> 32);
	}
	template<> inline void TraceSource::Trace_Bank_Reg_Read(bool Trace, uint8_t Bank, uint8_t Regnum, uint64_t Value)
	{
		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		BankRegReadRecord *header = (BankRegReadRecord*)(getNextPacket());
		*header = BankRegReadRecord(Bank, Regnum, Value, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(BankRegRead, Value >> 32);
	}
	template<> inline void TraceSource::Trace_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, float fValue)
	{
		uint32_t *pValue = (uint32_t*)&fValue;
		uint32_t Value = *pValue;

		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		BankRegWriteRecord *header = (BankRegWriteRecord*)(getNextPacket());
		*header = BankRegWriteRecord(Bank, Regnum, Value, 0);
	}
	template<> inline void TraceSource::Trace_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, double fValue)
	{
		uint64_t *pValue = (uint64_t*)&fValue;
		uint64_t Value = *pValue;

		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		BankRegWriteRecord *header = (BankRegWriteRecord*)(getNextPacket());
		*header = BankRegWriteRecord(Bank, Regnum, Value, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(BankRegWrite, Value >> 32);
	}

	template<> inline void TraceSource::Trace_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, uint32_t Value)
	{
		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		BankRegWriteRecord *header = (BankRegWriteRecord*)(getNextPacket());
		*header = BankRegWriteRecord(Bank, Regnum, Value, 0);
	}
	template<> inline void TraceSource::Trace_Bank_Reg_Write(bool Trace, uint8_t Bank, uint8_t Regnum, uint64_t Value)
	{
		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		BankRegWriteRecord *header = (BankRegWriteRecord*)(getNextPacket());
		*header = BankRegWriteRecord(Bank, Regnum, Value, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(BankRegWrite, Value >> 32);
	}


	template <> inline void TraceSource::Trace_Reg_Read(bool Trace, uint8_t Regnum, uint64_t Value)
	{
		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		RegReadRecord *record = (RegReadRecord*)(getNextPacket());
		*record = RegReadRecord(Regnum, Value, 1);

		auto extension = (DataExtensionRecord*)(getNextPacket());
		*extension = DataExtensionRecord(RegRead, Value >> 32);
	}

	template <> inline void TraceSource::Trace_Reg_Read(bool Trace, uint8_t Regnum, uint32_t Value)
	{
		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		RegReadRecord *header = (RegReadRecord*)(getNextPacket());
		*header = RegReadRecord(Regnum, Value, 0);
	}
	template <> inline void TraceSource::Trace_Reg_Read(bool Trace, uint8_t Regnum, uint16_t Value)
	{
		Trace_Reg_Read(Trace, Regnum, (uint32_t)Value);
	}
	template <> inline void TraceSource::Trace_Reg_Read(bool Trace, uint8_t Regnum, uint8_t Value)
	{
		Trace_Reg_Read(Trace, Regnum, (uint32_t)Value);
	}

	template <> inline void TraceSource::Trace_Reg_Write(bool Trace, uint8_t Regnum, uint64_t Value)
	{
		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		RegWriteRecord *record = (RegWriteRecord*)(getNextPacket());
		*record = RegWriteRecord(Regnum, Value, 1);

		auto extension = (DataExtensionRecord*)(getNextPacket());
		*extension = DataExtensionRecord(RegWrite, Value >> 32);
	}
	template <> inline void TraceSource::Trace_Reg_Write(bool Trace, uint8_t Regnum, uint32_t Value)
	{
		if(!IsPacketOpen()) return;
		assert(!IsTerminated() && IsPacketOpen());

		RegWriteRecord *record = (RegWriteRecord*)(getNextPacket());
		*record = RegWriteRecord(Regnum, Value, 0);
	}
	template <> inline void TraceSource::Trace_Reg_Write(bool Trace, uint8_t Regnum, uint16_t Value)
	{
		Trace_Reg_Write(Trace, Regnum, (uint32_t)Value);
	}
	template <> inline void TraceSource::Trace_Reg_Write(bool Trace, uint8_t Regnum, uint8_t Value)
	{
		Trace_Reg_Write(Trace, Regnum, (uint32_t)Value);
	}

	template<> inline void TraceSource::TraceMemReadAddr(uint64_t Addr, uint32_t Width)
	{
		auto *record = (MemReadAddrRecord*)getNextPacket();
		*record = MemReadAddrRecord(Width, Addr, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(MemReadAddr, Addr >> 32);
	}
	template<> inline void TraceSource::TraceMemReadAddr(uint32_t Addr, uint32_t Width)
	{
		auto *record = (MemReadAddrRecord*)getNextPacket();
		*record = MemReadAddrRecord(Width, Addr, 0);
	}


	template<> inline void TraceSource::TraceMemReadData(uint8_t Data, uint32_t Width)
	{
		auto *record = (MemReadDataRecord*)getNextPacket();
		*record = MemReadDataRecord(Width, Data, 0);
	}
	template<> inline void TraceSource::TraceMemReadData(uint16_t Data, uint32_t Width)
	{
		auto *record = (MemReadDataRecord*)getNextPacket();
		*record = MemReadDataRecord(Width, Data, 0);
	}
	template<> inline void TraceSource::TraceMemReadData(uint32_t Data, uint32_t Width)
	{
		auto *record = (MemReadDataRecord*)getNextPacket();
		*record = MemReadDataRecord(Width, Data, 0);
	}
	template<> inline void TraceSource::TraceMemReadData(uint64_t Data, uint32_t Width)
	{
		auto *record = (MemReadDataRecord*)getNextPacket();
		*record = MemReadDataRecord(Width, Data, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(MemReadData, Data >> 32);
	}

	template<> inline void TraceSource::TraceMemWriteAddr(uint32_t Addr, uint32_t Width)
	{
		auto *record = (MemWriteAddrRecord*)getNextPacket();
		*record = MemWriteAddrRecord(Addr, Width, 0);
	}
	template<> inline void TraceSource::TraceMemWriteAddr(uint64_t Addr, uint32_t Width)
	{
		auto *record = (MemWriteAddrRecord*)getNextPacket();
		*record = MemWriteAddrRecord(Addr, Width, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(MemWriteAddr, Addr >> 32);
	}

	template<> inline void TraceSource::TraceMemWriteData(uint32_t Data, uint32_t Width)
	{
		auto *record = (MemWriteDataRecord*)getNextPacket();
		*record = MemWriteDataRecord(Data, Width, 0);
	}
	template<> inline void TraceSource::TraceMemWriteData(uint64_t Data, uint32_t Width)
	{
		auto *record = (MemWriteDataRecord*)getNextPacket();
		*record = MemWriteDataRecord(Data, Width, 1);

		auto *extension = (DataExtensionRecord*)getNextPacket();
		*extension = DataExtensionRecord(MemWriteData, Data >> 32);
	}

}

#endif
