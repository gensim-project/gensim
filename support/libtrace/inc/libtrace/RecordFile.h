#ifndef RECORDFILE_H
#define RECORDFILE_H

#include "RecordTypes.h"
#include "RecordIterator.h"
#include "TraceRecordStream.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace libtrace {

	class RecordFile : public RecordBufferInterface
	{
	public:
		RecordFile(FILE *f) : _file(f), _buffer(nullptr) { if(f) fseek(f, 0, SEEK_END); uint64_t size = ftell(f); _count = size / sizeof(Record); }
		
		RecordIterator begin();
		RecordIterator end();
		
		Record Get(size_t i) { if(i >= _count) assert(false); if(!_buffer || _buffer_page != BufferPage(i)) loadBuffer(i); return _buffer[BufferOffset(i)]; }
		uint64_t Size() { return _count; }
		
	private:
		static const uint64_t kBufferBits = 17;
		static const uint64_t kBufferCount = 1 << kBufferBits;
		static const uint64_t kBufferSize = kBufferCount * sizeof(TraceRecord);

		FILE *_file;
		uint64_t _count;
		
		uint64_t BufferPage(uint64_t idx) { return idx >> kBufferBits; }
		uint64_t BufferOffset(uint64_t idx) { return idx % kBufferCount; }
		void loadBuffer(uint64_t idx) {
			if(!_buffer) _buffer = (Record*)malloc(kBufferSize);
			
			_buffer_page = BufferPage(idx);
			
			if(fseek(_file, _buffer_page * kBufferSize, SEEK_SET)) {
				perror("");
				abort();
			}
			if(fread(_buffer, kBufferSize, 1, _file) != 1) {
				if(ferror(_file)) {
					perror("");
					abort();
				}
			}
		}
		
		uint64_t _buffer_page;
		Record *_buffer;
	};
	
}

#endif
