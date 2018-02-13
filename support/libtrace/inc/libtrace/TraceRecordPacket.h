/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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

namespace libtrace {
	class TraceRecordPacketVisitor;
	class TraceRecordPacket {
	public:
		TraceRecordPacket(const TraceRecord &record, std::vector<DataExtensionRecord> extensions = {}) : record_(record), extensions_(extensions) {}
		
		const TraceRecord &GetRecord() const { return record_; }
		const std::vector<DataExtensionRecord> &GetExtensions() const { return extensions_; }
		
		void Visited(TraceRecordPacketVisitor *visitor) const;
		
	private:
		TraceRecord record_;
		std::vector<DataExtensionRecord> extensions_;
	};
}

#endif /* TRACERECORDPACKET_H */

