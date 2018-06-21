/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/RecordFile.h"

using namespace libtrace;

RecordIterator RecordFile::begin()
{
	return RecordIterator(this, 0);
}
RecordIterator RecordFile::end()
{
	return RecordIterator(this, _count);
}
