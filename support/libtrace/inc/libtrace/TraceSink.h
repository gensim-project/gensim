/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
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

namespace libtrace
{

	class ArchInterface;

	class TraceSink
	{
	public:
		TraceSink();
		virtual ~TraceSink() = default;

		virtual int Open() = 0;
		virtual void SinkPackets(int id, const TraceRecord *start, const TraceRecord *end) = 0;
		virtual void Flush() = 0;
	};

	class BinaryFileTraceSink : public TraceSink
	{
	public:
		BinaryFileTraceSink(const std::string &pattern);
		~BinaryFileTraceSink();

		int Open() override;
		void SinkPackets(int id, const TraceRecord* start, const TraceRecord* end) override;
		void Flush() override;

	private:
		std::vector<FILE*> files_;
		std::string pattern_;
	};

}


#endif /* TRACEMANAGER_H_ */
