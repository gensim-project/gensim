/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/RecordFile.h"
#include "libtrace/RecordIterator.h"

using namespace libtrace;

TraceRecord RecordIterator::operator*()
{
	Record r;
	bool success = buffer_->Get(_idx, r);
	assert(success);
	auto i = *(TraceRecord*)&r;
	return i;
}


bool RecordIterator::operator==(const RecordIterator &other)
{
	return buffer_ == other.buffer_ && _idx == other._idx;
}
bool RecordIterator::operator!=(const RecordIterator &other)
{
	return !(*this == other);
}

