/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/ArchInterface.h"
#include "libtrace/TraceSink.h"
#include "libtrace/TraceSource.h"
#include "libtrace/TraceRecordStream.h"
#include "libtrace/TraceRecordPacketVisitor.h"

#include <sstream>
#include <string>
#include <string.h>


using namespace libtrace;


TraceSink::TraceSink()
{

}

BinaryFileTraceSink::BinaryFileTraceSink(const std::string &pattern) : TraceSink(), pattern_(pattern)
{

}

BinaryFileTraceSink::~BinaryFileTraceSink()
{
	for(auto file : files_) {
		fclose(file);
	}
}

int BinaryFileTraceSink::Open()
{
	int new_id = files_.size();
	std::stringstream str;
	str << pattern_ << new_id;
	FILE *f = fopen(str.str().c_str(), "w");
	files_.push_back(f);
	return new_id;
}

void BinaryFileTraceSink::SinkPackets(int id, const TraceRecord* start, const TraceRecord* end)
{
	FILE *f = files_.at(id);
	fwrite(start, end-start, sizeof(*start), f);
}

void BinaryFileTraceSink::Flush()
{
	// nothing to do
}

