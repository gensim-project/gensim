/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

#include <cstring>
#include <deque>
#include <fstream>

namespace libtrace
{

	// Interface for interacting with buffers containing trace records
	class RecordBufferInterface
	{
	public:
		virtual bool Get(size_t i, Record &r) = 0;
		virtual uint64_t Size() = 0;
	};

	class RecordStreamOutputInterface
	{
	public:
		virtual void Put(const Record &record) = 0;
	};

	// Interface for interacting with (potentially infinite) streams of records
	class RecordStreamInputInterface
	{
	public:
		virtual Record Get() = 0;
		virtual Record Peek() = 0;
		virtual bool Good() = 0;
		virtual void Skip(size_t i) = 0;
	};

	class RecordFileInputStream : public RecordStreamInputInterface
	{
	public:
		RecordFileInputStream(std::ifstream& str);

		libtrace::Record Get() override;
		bool Good() override;
		libtrace::Record Peek() override;
		void Skip(size_t i) override;

	private:
		void makeReadyRecord();

		bool is_ready_record_;
		libtrace::Record ready_record_;
		std::ifstream &stream_;

		std::vector<Record> record_buffer_;
		uint32_t record_buffer_pointer_;
	};

	class PacketStreamInterface
	{
	public:
		virtual TraceRecordPacket Get() = 0;
		virtual TraceRecordPacket Peek() = 0;
		virtual bool Good() = 0;
	};

	class RecordStreamInterface : public RecordStreamOutputInterface, public RecordStreamInputInterface
	{
	};

	class RecordBufferStreamAdaptor : public RecordStreamInputInterface
	{
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

	class TracePacketStreamInterface
	{
	public:
		virtual const TraceRecordPacket &Get() = 0;
		virtual const TraceRecordPacket &Peek() = 0;

		virtual bool Good() = 0;
	};

	class TracePacketStreamAdaptor : public TracePacketStreamInterface
	{
	public:
		TracePacketStreamAdaptor(RecordStreamInputInterface *input_stream);
		virtual ~TracePacketStreamAdaptor();

		const TraceRecordPacket &Get() override;
		bool Good() override;
		const TraceRecordPacket &Peek() override;

	private:
		bool packet_ready_;
		TraceRecordPacket packet_;

		bool PreparePacket();

		RecordStreamInputInterface *input_stream_;

		std::vector<DataExtensionRecord> extensions_;
	};
}

#endif /* TRACERECORDSTREAM_H */

