/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "libtrace/TraceRecordPacketVisitor.h"

using namespace libtrace;

void TraceRecordPacketVisitor::Visit(const TraceRecordPacket& packet)
{
	packet.Visited(this);
}
