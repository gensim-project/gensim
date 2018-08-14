/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   TraceRecordPacket.h
 * Author: harry
 *
 * Created on 14 November 2017, 11:23
 */

#ifndef TRACERECORDPACKET_H
#define TRACERECORDPACKET_H

#include "RecordTypes.h"

#include <vector>

namespace libtrace
{
	class TraceRecordPacketVisitor;
	class TraceRecordPacket
	{
	public:
		TraceRecordPacket(const TraceRecord &record, std::vector<DataExtensionRecord> extensions = std::vector<DataExtensionRecord>()) : record_(record), extensions_(extensions) {}

		const TraceRecord &GetRecord() const
		{
			return record_;
		}
		const std::vector<DataExtensionRecord> &GetExtensions() const
		{
			return extensions_;
		}

		void Visited(TraceRecordPacketVisitor *visitor) const;

	private:
		TraceRecord record_;
		std::vector<DataExtensionRecord> extensions_;
	};
}

#endif /* TRACERECORDPACKET_H */

