/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RecordReader.h
 * Author: harry
 *
 * Created on 13 November 2017, 16:24
 */

#ifndef RECORDREADER_H
#define RECORDREADER_H

#include "RecordTypes.h"

#include <vector>
#include <stdexcept>

namespace libtrace {
	class RecordReader {
	public:
		class DataReader {
		public:
			DataReader(const RecordReader &reader) : reader_(reader) {}
			
			size_t GetSize() const { return 4 + reader_.extensions_.size() * 4; }
			uint32_t AsU32() const { return reader_.GetRecord().GetData32(); }
			uint64_t AsU64() const { return (uint64_t)reader_.GetRecord().GetData32() | ((uint64_t)reader_.GetExtensions().at(0).GetData32() << 32); }
			
		private:
			const RecordReader &reader_;
		};
	
	public:
		typedef std::vector<DataExtensionRecord> extension_list_t;
		RecordReader(const TraceRecord &record, TraceRecordType type, extension_list_t extensions = {}) : record_(record), extensions_(extensions) {
			if(record.GetType() != type) {
				throw std::logic_error("");
			}
		}
		
		size_t GetDataSize() { return 4 + extensions_.size() * 4; }
	protected:
		TraceRecord GetRecord() const { return record_; }
		const extension_list_t &GetExtensions() const { return extensions_; }
	private:
		TraceRecord record_;
		extension_list_t extensions_;
	};
	
#define ReaderTemplate(x) public: x ## Reader(const x ## Record &record, const extension_list_t &extensions) : RecordReader(record, x, extensions) {} public: x ## Record GetRecord() const { auto record = RecordReader::GetRecord(); return *(x##Record*)&record; };
#define Data32Template(x) public: DataReader Get##x() const { return DataReader(*this); }
#define Data16Template(x) public: uint16_t Get##x() const { return this->GetRecord().GetData16(); }
#define PassthroughTemplate(x, y) public: x Get##y() const { return this->GetRecord().Get##y(); }
	
	class InstructionHeaderReader : public RecordReader {
		ReaderTemplate(InstructionHeader);
		
		Data32Template(PC);
		Data16Template(IsaMode);
	};
	
	class InstructionCodeReader : public RecordReader {
		ReaderTemplate(InstructionCode);
		Data32Template(Code);
		Data16Template(ExceptionMode);
	};
	
	class RegReadReader : public RecordReader {
		ReaderTemplate(RegRead);
		Data32Template(Value);
		Data16Template(Index);
	};
	
	class RegWriteReader : public RecordReader {
		ReaderTemplate(RegWrite);
		Data32Template(Value);
		Data16Template(Index);
	};
	
	class BankRegReadReader : public RecordReader {
		ReaderTemplate(BankRegRead);
		Data32Template(Value);
		PassthroughTemplate(uint8_t, Bank);
		PassthroughTemplate(uint8_t, RegNum);
	};
	
	class BankRegWriteReader : public RecordReader {
		ReaderTemplate(BankRegWrite);
		Data32Template(Value);
		PassthroughTemplate(uint8_t, Bank);
		PassthroughTemplate(uint8_t, RegNum);
	};
	
	class MemWriteAddrReader : public RecordReader {
		ReaderTemplate(MemWriteAddr);
		Data16Template(Width);
		Data32Template(Address);
	};
	class MemWriteDataReader : public RecordReader {
		ReaderTemplate(MemWriteData);
		Data16Template(Width);
		Data32Template(Data);
	};
	
	class MemReadAddrReader : public RecordReader {
		ReaderTemplate(MemReadAddr);
		Data16Template(Width);
		Data32Template(Address);
	};
	class MemReadDataReader : public RecordReader {
		ReaderTemplate(MemReadData);
		Data16Template(Width);
		Data32Template(Data);
	};
}

std::ostream &operator<<(std::ostream &str, const libtrace::RecordReader::DataReader &reader);

#undef ReaderTemplate
#undef Data16Template
#undef Data32Template
#undef PassthroughTemplate

#endif /* RECORDREADER_H */
