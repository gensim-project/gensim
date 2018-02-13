#ifndef RECORDS_H
#define RECORDS_H

#include <cstdint>

namespace libtrace {

	enum TraceRecordType
	{
		Unknown,
		
		InstructionHeader,
		InstructionCode,
		RegRead,
		RegWrite,
		BankRegRead,
		BankRegWrite,
		MemReadAddr,
		MemReadData,
		MemWriteAddr,
		MemWriteData,
		
		InstructionBundleHeader,
		
		DataExtension
	};
		
	struct Record
	{
	public:
		Record(uint32_t header=0, uint32_t data=0) : header(header), data(data) {}
		
		uint32_t GetHeader() const { return header; }
		uint32_t GetData() const { return data; }
	private:
		uint32_t header, data;
	};

	struct TraceRecord : public Record
	{
	public:
		TraceRecord(TraceRecordType type, uint16_t data16, uint32_t data32, uint8_t extension_count) : Record((((uint32_t)type) << 24) | (((uint32_t)extension_count) << 16) | data16, data32) {}
		TraceRecord() : TraceRecord(Unknown, 0, 0, 0) {}
		TraceRecord(const TraceRecord &tr) : Record(tr.GetHeader(), tr.GetData()) {}
		TraceRecord(const Record &r) : Record(r.GetHeader(), r.GetData()) {}
		
		TraceRecordType GetType() const { return (TraceRecordType)(GetHeader() >> 24); }
		uint8_t GetExtensionCount() const { return (TraceRecordType)(GetHeader() >> 16); }
		
		uint16_t GetData16() const { return GetHeader() & 0xffff; }
		uint32_t GetData32() const { return GetData(); }
	};

	struct InstructionHeaderRecord : public TraceRecord
	{
	public:
		InstructionHeaderRecord(uint8_t isa_mode, uint32_t pc, uint8_t extensions) : TraceRecord(InstructionHeader, isa_mode, pc, extensions) {}
		
		uint8_t GetIsaMode() const { return GetData16(); }
		uint32_t GetPC() const { return GetData32(); }
	};

	struct InstructionCodeRecord : public TraceRecord
	{
	public:
		InstructionCodeRecord(uint16_t irq_mode, uint32_t ir, uint8_t extensions) : TraceRecord(InstructionCode, irq_mode, ir, extensions) {}
		
		uint16_t GetIRQMode() const { return GetData16(); }
		uint32_t GetIR() const { return GetData32(); }
	};

	struct RegReadRecord : public TraceRecord
	{
	public:
		RegReadRecord(uint16_t regnum, uint32_t data, uint8_t extensions) : TraceRecord(RegRead, regnum, data, extensions) {}
		
		uint16_t GetRegNum() const { return GetData16(); }
		uint32_t GetData() const { return GetData32(); }
	};

	struct RegWriteRecord : public TraceRecord
	{
	public:
		RegWriteRecord(uint16_t regnum, uint32_t data, uint8_t extensions) : TraceRecord(RegWrite, regnum, data, extensions) {}
		
		uint16_t GetRegNum() const { return GetData16(); }
		uint32_t GetData() const { return GetData32(); }
	};

	struct BankRegReadRecord : public TraceRecord
	{
	public:
		BankRegReadRecord(uint8_t bank, uint8_t regnum, uint32_t data, uint8_t extensions) : TraceRecord(BankRegRead, ((uint16_t)bank << 8) | regnum, data, extensions) {}
		
		uint8_t GetBank() const { return GetData16() >> 8; }
		uint8_t GetRegNum() const { return GetData16() & 0xff; }
		uint32_t GetData() const { return GetData32(); }
	};

	struct BankRegWriteRecord : public TraceRecord
	{
	public:
		BankRegWriteRecord(uint8_t bank, uint8_t regnum, uint32_t data, uint8_t extensions) : TraceRecord(BankRegWrite, ((uint16_t)bank << 8) | regnum, data, extensions) {}
		
		uint8_t GetBank() const { return GetData16() >> 8; }
		uint8_t GetRegNum() const { return GetData16() & 0xff; }
		uint32_t GetData() const { return GetData32(); }
	};

	struct MemReadAddrRecord : public TraceRecord
	{
	public:
		MemReadAddrRecord(uint8_t width, uint32_t address, uint8_t extensions) : TraceRecord(MemReadAddr, width, address, extensions) {}
		
		uint8_t GetWidth() const { return GetData16(); }
		uint32_t GetAddress() const { return GetData32(); }
	};

	struct MemReadDataRecord : public TraceRecord
	{
	public:
		MemReadDataRecord(uint8_t width, uint32_t address, uint8_t extensions) : TraceRecord(MemReadData, width, address, extensions) {}
		
		uint8_t GetWidth() const { return GetData16(); }
		uint32_t GetData() const { return GetData32() & ((1ULL << (GetWidth()*8))-1); }
	};

	struct MemWriteAddrRecord : public TraceRecord
	{
	public:
		MemWriteAddrRecord(uint8_t width, uint32_t address, uint8_t extensions) : TraceRecord(MemWriteAddr, width, address, extensions) {}
		
		uint8_t GetWidth() const { return GetData16(); }
		uint32_t GetAddress() const { return GetData32(); }
	};

	struct MemWriteDataRecord : public TraceRecord
	{
	public:
		MemWriteDataRecord(uint8_t width, uint32_t address, uint8_t extensions) : TraceRecord(MemWriteData, width, address, extensions) {}
		
		uint8_t GetWidth() const { return GetData16(); }
		uint32_t GetData() const { return GetData32() & ((1ULL << (GetWidth()*8))-1); }
	};

	struct DataExtensionRecord : public TraceRecord
	{
	public:
		DataExtensionRecord(uint16_t prevtype, uint32_t data) : TraceRecord(DataExtension, prevtype, data, 0) {}
		
		uint32_t GetData() const { return GetData32(); }
	};
	
	struct InstructionBundleHeaderRecord : public TraceRecord 
	{
	public:
		InstructionBundleHeaderRecord(uint32_t low_start_pc, uint8_t extensions) : TraceRecord(InstructionBundleHeader, 0, low_start_pc, extensions) {}
		
		uint32_t GetLowPC() const { return GetData32(); }
	};
}

#endif
