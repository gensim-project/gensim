/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   RecordReader.cpp
 * Author: harry
 *
 * Created on 13 November 2017, 16:20
 */

#include "libtrace/RecordReader.h"
#include <iostream>
#include <iomanip>

using namespace libtrace;


std::ostream &operator<<(std::ostream &str, const libtrace::RecordReader::DataReader &reader)
{
	str << "[" << reader.GetExtensionCount() << "]";
	str << "0x";
	for(int32_t i = reader.GetExtensionCount()-1; i >= 0; --i) {
		str << std::hex << std::setw(8) << std::setfill('0') << reader.GetExtension(i);
	}
	str << std::hex << std::setw(8) << std::setfill('0') << reader.AsU32();

	return str;
}
