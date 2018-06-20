/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "libtrace/TraceFilter.h"
#include "libtrace/RecordTypes.h"

using namespace libtrace;

IndexTraceFilter::IndexTraceFilter(TraceSink* next_target) : filtered_sink_(next_target) {
}

void IndexTraceFilter::SinkPackets(const TraceRecord* start, const TraceRecord* end)
{
	for(auto i = start; i < end; ++i) {
		addPacket(i);
	}
}

void IndexTraceFilter::addPacket(const TraceRecord* r)
{
	if(buffer_.empty()) {
		// reserve a slot for the index
		buffer_.push_back(IndexRecord(0, 0));
	}
	
	if(r->GetType() == InstructionHeader) {
		instructions_since_last_index_++;
	}
	buffer_.push_back(*r);
	
	checkBufferSize();
}

void IndexTraceFilter::checkBufferSize()
{
	if(buffer_.size() > max_buffered_records_ || instructions_since_last_index_ > max_buffered_instructions_) {
		Flush();
	}
}

void IndexTraceFilter::Flush()
{
	buffer_[0] = IndexRecord(buffer_.size(), instructions_since_last_index_);
	instructions_since_last_index_ = 0;
	
	filtered_sink_->SinkPackets(buffer_.data(), buffer_.data() + buffer_.size());
	filtered_sink_->Flush();
	
	buffer_.clear();
}
