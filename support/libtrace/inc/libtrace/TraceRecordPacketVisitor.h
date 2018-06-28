/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   TraceRecordPacketVisitor.h
 * Author: harry
 *
 * Created on 14 November 2017, 13:26
 */

#ifndef TRACERECORDPACKETVISITOR_H
#define TRACERECORDPACKETVISITOR_H

#include "TraceRecordPacket.h"
#include "RecordReader.h"

namespace libtrace
{
	class TraceRecordPacketVisitor
	{
	public:
		typedef std::vector<DataExtensionRecord> extension_list_t;

		void Visit(const TraceRecordPacket &packet);

		virtual void VisitInstructionHeader(const InstructionHeaderReader &record) = 0;
		virtual void VisitInstructionCode(const InstructionCodeReader &record) = 0;

		virtual void VisitRegRead(const RegReadReader &record) = 0;
		virtual void VisitRegWrite(const RegWriteReader &record) = 0;
		virtual void VisitBankRegRead(const BankRegReadReader &record) = 0;
		virtual void VisitBankRegWrite(const BankRegWriteReader &record) = 0;

		virtual void VisitMemReadAddr(const MemReadAddrReader &record) = 0;
		virtual void VisitMemReadData(const MemReadDataReader &record) = 0;
		virtual void VisitMemWriteAddr(const MemWriteAddrReader &record) = 0;
		virtual void VisitMemWriteData(const MemWriteDataReader &record) = 0;
	};
}

#endif /* TRACERECORDPACKETVISITOR_H */

