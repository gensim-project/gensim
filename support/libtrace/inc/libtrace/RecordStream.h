#ifndef RECORDSTREAM_H
#define RECORDSTREAM_H

#include "RecordTypes.h"
#include "RecordIterator.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace libtrace {

class RecordStream
{
public:
	RecordStream(FILE *f) : _file(f), _buffer(0), _buffer_ptr(0), _good(true) { _buffer = new Record[kBufferEntries]; _buffer_ptr = _buffer+kBufferEntries; }
	
	const Record &next() { if(buffer_empty()) refill_buffer(); return *_buffer_ptr++; }
	const Record &peek() { if(buffer_empty()) refill_buffer(); return *_buffer_ptr; }
	bool good() { return _good; }
	
	
private:
	static const uint32_t kBufferEntries = 1 << 10;

	FILE *_file;
	
	Record *_buffer;
	Record *_buffer_ptr;
	bool _good;
	
	bool buffer_empty() { return (_buffer_ptr == (_buffer+kBufferEntries)); }
	void refill_buffer() { _good = (!feof(_file) && (fread(_buffer, sizeof(Record), kBufferEntries, _file) >= 0)); _buffer_ptr = _buffer; }
};

}

#endif
