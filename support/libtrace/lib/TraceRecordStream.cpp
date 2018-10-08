/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "libtrace/TraceRecordStream.h"

#include <cassert>
#include <cstdlib>
#include <utility>
#include <vector>

using namespace libtrace;

RecordBufferStreamAdaptor::RecordBufferStreamAdaptor(RecordBufferInterface* buffer) : buffer_(buffer), index_(0)
{

}

RecordBufferStreamAdaptor::~RecordBufferStreamAdaptor()
{

}

Record RecordBufferStreamAdaptor::Get()
{
	Record r;
	bool success = buffer_->Get(index_++, r);
	assert(success);
	return r;
}

Record RecordBufferStreamAdaptor::Peek()
{
	Record r;
	bool success = buffer_->Get(index_, r);
	assert(success);
	return r;
}

void RecordBufferStreamAdaptor::Skip(size_t i)
{
	index_ += i;
}


bool RecordBufferStreamAdaptor::Good()
{
	return index_ < buffer_->Size();
}

TracePacketStreamAdaptor::TracePacketStreamAdaptor(RecordStreamInputInterface* input_stream) : input_stream_(input_stream), packet_ready_(false), packet_(TraceRecordPacket(TraceRecord()))
{

}

TracePacketStreamAdaptor::~TracePacketStreamAdaptor()
{

}

const TraceRecordPacket &TracePacketStreamAdaptor::Get()
{
	if(!packet_ready_) {
		PreparePacket();
	}
	packet_ready_ = false;

	return packet_;
}

bool TracePacketStreamAdaptor::Good()
{
	return input_stream_->Good();
}

const TraceRecordPacket &TracePacketStreamAdaptor::Peek()
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

	int extension_count = packet_head.GetExtensionCount();

	if(extension_count != 0) {
		DataExtensionRecord extension_records[extension_count];

		for(int i = 0; i < extension_count; ++i) {
			if(!input_stream_->Good()) {
				return false;
			}
			Record record = input_stream_->Get();
			extension_records[i] = *(DataExtensionRecord*)&record;
		}

		packet_.Assign(packet_head, extension_records, extension_count);
	} else {
		packet_.Assign(packet_head);
	}

	packet_ready_ = true;
	return true;
}

libtrace::RecordFileInputStream::RecordFileInputStream(std::ifstream& str) : stream_(str), is_ready_record_(false), record_buffer_pointer_(0)
{
}

libtrace::Record libtrace::RecordFileInputStream::Get()
{
	if(!is_ready_record_) {
		makeReadyRecord();
	}

	is_ready_record_ = false;
	return ready_record_;
}

bool libtrace::RecordFileInputStream::Good()
{
	return stream_.good();
}

libtrace::Record libtrace::RecordFileInputStream::Peek()
{
	if(!is_ready_record_) {
		makeReadyRecord();
	}

	return ready_record_;
}

void libtrace::RecordFileInputStream::Skip(size_t i)
{
	is_ready_record_ = false;
	stream_.ignore(i * sizeof(libtrace::Record));
}

void libtrace::RecordFileInputStream::makeReadyRecord()
{
	if(record_buffer_pointer_ == record_buffer_.size()) {
		record_buffer_.resize(1024 * 1024);
		stream_.read((char*)&record_buffer_.at(0), record_buffer_.size() * sizeof(Record));
		record_buffer_.resize(stream_.gcount() / sizeof(Record));
		record_buffer_pointer_ = 0;
	}

	ready_record_ = record_buffer_[record_buffer_pointer_++];

	if(stream_.good()) {
		is_ready_record_ = true;
	}
}

