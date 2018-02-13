/*
 * TraceManager.h
 *
 *  Created on: 8 Aug 2014
 *      Author: harry
 */

#ifndef TRACEMANAGER_H_
#define TRACEMANAGER_H_

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
#include <fstream>

#include "RecordTypes.h"
#include "TraceRecordPacket.h"
#include "TraceRecordStream.h"

namespace libtrace {

class ArchInterface;

	class TraceSink
	{
	public:
		TraceSink();

		virtual void SinkPackets(const TraceRecord *start, const TraceRecord *end) = 0;
		virtual void Flush() = 0;
	};

	class BinaryFileTraceSink : public TraceSink
	{
	public:
		BinaryFileTraceSink(FILE *outfile);
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
		TextFileTraceSink(FILE *outfile, ArchInterface *interface);
		~TextFileTraceSink();

		void SinkPackets(const TraceRecord* start, const TraceRecord* end) override;
		void Flush() override;

	private:
		void WritePacket(const TraceRecordPacket &pkt);

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
		ArchInterface *interface_;

		uint32_t isa_mode_;
		uint32_t pc_;
		
		RecordStreamOutputInterface *record_output_stream_;
		PacketStreamInterface *packet_input_stream_;
	};
}


#endif /* TRACEMANAGER_H_ */
