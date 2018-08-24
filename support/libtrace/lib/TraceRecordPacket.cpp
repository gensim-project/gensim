/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "libtrace/RecordTypes.h"
#include "libtrace/TraceRecordPacket.h"
#include "libtrace/TraceRecordPacketVisitor.h"

#include <cassert>

using namespace libtrace;

void TraceRecordPacket::Visited(TraceRecordPacketVisitor* visitor) const
{
	// this is ugly but we definitely don't want to attach vtable ptrs to the individual trace records
	switch(GetRecord().GetType()) {
#define Handle(x) case x: visitor->Visit##x(x##Reader(*(x##Record*)&GetRecord(), GetExtensions())); break;
			Handle(InstructionHeader)
			Handle(InstructionCode)
			Handle(RegRead)
			Handle(RegWrite)
			Handle(BankRegRead)
			Handle(BankRegWrite)

			Handle(MemReadAddr)
			Handle(MemReadData)
			Handle(MemWriteAddr)
			Handle(MemWriteData)

		default:
			assert(!"Unknown record type");
	}
}
