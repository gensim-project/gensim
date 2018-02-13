#include "libtrace/RecordFile.h"
#include "libtrace/RecordIterator.h"

using namespace libtrace;

TraceRecord RecordIterator::operator*() { Record r = buffer_->Get(_idx); auto i = *(TraceRecord*)&r; return i; }


bool RecordIterator::operator==(const RecordIterator &other) {
	return buffer_ == other.buffer_ && _idx == other._idx;
}
bool RecordIterator::operator!=(const RecordIterator &other) {
	return !(*this == other);
}

