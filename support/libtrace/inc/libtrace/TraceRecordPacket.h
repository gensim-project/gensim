/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   TraceRecordPacket.h
 * Author: harry
 *
 * Created on 14 November 2017, 11:23
 */

#ifndef TRACERECORDPACKET_H
#define TRACERECORDPACKET_H

#include <cassert>
#include <utility>
#include <vector>

#include "RecordTypes.h"

namespace libtrace
{
	class TraceRecordPacketVisitor;
	class TraceRecordPacket
	{
	public:
		TraceRecordPacket(const TraceRecordPacket &other) : record_(other.record_), extension_count_(other.extension_count_), extensions_(other.extensions_) {}

		TraceRecordPacket(const TraceRecord &record) : record_(record) {}
		TraceRecordPacket(const TraceRecord &record, const std::vector<DataExtensionRecord> &extensions) : record_(record)
		{
			Assign(record, extensions);
		}

		TraceRecordPacket &operator=(const TraceRecordPacket &other)
		{
			Assign(other.record_, other.GetExtensions());
			return *this;
		}

		void Assign(const TraceRecord &record)
		{
			record_ = record;
			extension_count_ = 0;
		}
		void Assign(const TraceRecord &record, const DataExtensionRecord *extensions, int extension_count)
		{
			record_ = record;

			extension_count_ = extension_count;
			assert(extension_count_ <= kExtensionCount);
			for(int i = 0; i < extension_count_; ++i) {
				extensions_[i] = extensions[i];
			}
		}
		void Assign(const TraceRecord &record, const std::vector<DataExtensionRecord> &extensions)
		{
			Assign(record, &extensions.at(0), extensions.size());
		}

		const TraceRecord &GetRecord() const
		{
			return record_;
		}
		const std::vector<DataExtensionRecord> GetExtensions() const
		{
			std::vector<DataExtensionRecord> extension;
			for(int i = 0; i < extension_count_; ++i) {
				extension.push_back(extensions_[i]);
			}
			return extension;
		}

		void Visited(TraceRecordPacketVisitor *visitor) const;

	private:
		TraceRecord record_;

		int extension_count_;
		static const int kExtensionCount = 4;
		DataExtensionRecord extensions_[kExtensionCount];
	};
}

#endif /* TRACERECORDPACKET_H */

