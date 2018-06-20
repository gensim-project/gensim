/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   TraceFilter.h
 * Author: harry
 *
 * Created on 04 May 2018, 09:25
 */

#ifndef TRACEFILTER_H
#define TRACEFILTER_H

#include "TraceSink.h"

namespace libtrace
{
	class IndexTraceFilter : public TraceSink
	{
	public:
		IndexTraceFilter(TraceSink *next_target);

		void SinkPackets(const TraceRecord* start, const TraceRecord* end) override;
		void Flush() override;

	private:
		void addPacket(const TraceRecord *r);
		void checkBufferSize();

		TraceSink *filtered_sink_;

		std::vector<TraceRecord> buffer_;
		uint32_t instructions_since_last_index_;

		uint32_t max_buffered_records_;
		uint32_t max_buffered_instructions_;
	};
}

#endif /* TRACEFILTER_H */

