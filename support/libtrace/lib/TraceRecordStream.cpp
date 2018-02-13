/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "libtrace/TraceRecordStream.h"

#include <cassert>
#include <cstdlib>

using namespace libtrace;

RecordBufferStreamAdaptor::RecordBufferStreamAdaptor(RecordBufferInterface* buffer) : buffer_(buffer), index_(0)
{

}

RecordBufferStreamAdaptor::~RecordBufferStreamAdaptor()
{

}

Record RecordBufferStreamAdaptor::Get()
{
	return buffer_->Get(index_++);
}

Record RecordBufferStreamAdaptor::Peek()
{
	return buffer_->Get(index_);
}

void RecordBufferStreamAdaptor::Skip(size_t i)
{
	index_ += i;
}


bool RecordBufferStreamAdaptor::Good()
{
	return index_ < buffer_->Size();
}

TracePacketStreamAdaptor::TracePacketStreamAdaptor(RecordStreamInputInterface* input_stream) : input_stream_(input_stream), packet_ready_(false), packet_(TraceRecordPacket(TraceRecord(),{}))
{

}

TracePacketStreamAdaptor::~TracePacketStreamAdaptor()
{

}

TraceRecordPacket TracePacketStreamAdaptor::Get()
{
	if(!packet_ready_) {
		PreparePacket();
	}
	packet_ready_ = false;
	return packet_;
}

bool TracePacketStreamAdaptor::Good()
{
	if(!packet_ready_) {
		return PreparePacket();
	}
	return true;
}

TraceRecordPacket TracePacketStreamAdaptor::Peek()
{
	if(!packet_ready_) {
		PreparePacket();
	}
	return packet_;
}

bool TracePacketStreamAdaptor::PreparePacket()
{
	if(!input_stream_->Good()) {
		return false;
	}
	Record rpacket_head = input_stream_->Get();
	TraceRecord packet_head = *(TraceRecord*)&rpacket_head;
	assert(packet_head.GetType() != DataExtension);
	
	std::vector<DataExtensionRecord> extensions;
	for(int i = 0; i < packet_head.GetExtensionCount(); ++i) {
		Record record = input_stream_->Get();
		DataExtensionRecord trecord = *(DataExtensionRecord*)&record;
		extensions.push_back(trecord);
	}
	
	packet_ = TraceRecordPacket(packet_head, extensions);
	packet_ready_ = true;
	return true;
}

