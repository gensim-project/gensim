/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TraceRecordStream.h
 * Author: harry
 *
 * Created on 14 November 2017, 11:12
 */

#ifndef TRACERECORDSTREAM_H
#define TRACERECORDSTREAM_H

#include "libtrace/TraceRecordPacket.h"
#include "libtrace/RecordTypes.h"

#include <deque>
#include <string.h>

namespace libtrace {
	
	// Interface for interacting with buffers containing trace records
	class RecordBufferInterface {
	public:
		virtual Record Get(size_t i) = 0;
		virtual size_t Size() = 0;
	};
	
	class RecordStreamOutputInterface {
	public:
		virtual void Put(const Record &record) = 0;
	};
	
	// Interface for interacting with (potentially infinite) streams of records
	class RecordStreamInputInterface {
	public:
		virtual Record Get() = 0;
		virtual Record Peek() = 0;
		virtual bool Good() = 0;
		virtual void Skip(size_t i) = 0;
	};
	
	class PacketStreamInterface {
	public:
		virtual TraceRecordPacket Get() = 0;
		virtual TraceRecordPacket Peek() = 0;
		virtual bool Good() = 0;
	};
	
	class RecordStreamInterface : public RecordStreamOutputInterface, public RecordStreamInputInterface {
	};
	
	class RecordBufferStreamAdaptor : public RecordStreamInputInterface {
	public:
		RecordBufferStreamAdaptor(RecordBufferInterface *buffer);
		virtual ~RecordBufferStreamAdaptor();
		
		Record Get() override;
		bool Good() override;
		Record Peek() override;
		void Skip(size_t i) override;

	private:
		RecordBufferInterface *buffer_;
		size_t index_;
	};
	
	class TracePacketStreamInterface {
	public:
		virtual TraceRecordPacket Get() = 0;
		virtual TraceRecordPacket Peek() = 0;
		
		virtual bool Good() = 0;
	};
	
	class TracePacketStreamAdaptor : public TracePacketStreamInterface {
	public:
		TracePacketStreamAdaptor(RecordStreamInputInterface *input_stream);
		virtual ~TracePacketStreamAdaptor();
		
		TraceRecordPacket Get() override;
		bool Good() override;
		TraceRecordPacket Peek() override;
		
	private:
		bool packet_ready_;
		TraceRecordPacket packet_;
		
		bool PreparePacket();
		
		RecordStreamInputInterface *input_stream_;
	};
}

#endif /* TRACERECORDSTREAM_H */

