/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "libtrace/TraceRecordPacketVisitor.h"

using namespace libtrace;

void TraceRecordPacketVisitor::Visit(const TraceRecordPacket& packet)
{
	packet.Visited(this);
}
