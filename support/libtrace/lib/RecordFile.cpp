#include "libtrace/RecordFile.h"

using namespace libtrace;

RecordIterator RecordFile::begin() { return RecordIterator(this, 0); }
RecordIterator RecordFile::end() { return RecordIterator(this, _count);
}
