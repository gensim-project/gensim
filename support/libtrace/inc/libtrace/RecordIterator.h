#ifndef RECORDITERATOR_H
#define RECORDITERATOR_H

#include <cstdint>
#include "RecordTypes.h"
#include "TraceRecordStream.h"

namespace libtrace {

class TraceRecord;
class RecordFile;
class RecordIterator;

class RecordIterator
{
public:
	RecordIterator(RecordBufferInterface *buffer, uint64_t start_idx) : buffer_(buffer), _idx(start_idx) {}

	friend bool operator==(const RecordIterator &a, const RecordIterator &b);

	TraceRecord operator*();
	
	// Preincrement operator
	RecordIterator operator++() { _idx++; return *this; }
	
	// Postincrement operator
	RecordIterator operator++(int) { auto temp = RecordIterator(buffer_, _idx); _idx++; return temp; }
	
	uint64_t index() { return _idx; }
	
	bool operator==(const libtrace::RecordIterator &other);
	bool operator!=(const libtrace::RecordIterator &other);
private:
	uint64_t _idx;
	RecordBufferInterface *buffer_;
};

}

#endif
