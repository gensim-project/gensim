/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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


std::ostream &operator<<(std::ostream &str, const libtrace::RecordReader::DataReader &reader) {
	// just handle u32 and u64 for now
	if(reader.GetSize() == 4) {
		str << "0x" << std::hex << std::setw(8) << std::setfill('0') << reader.AsU32();
	} else if(reader.GetSize() == 8) {
		str << "0x" << std::hex << std::setw(16) << std::setfill('0') << reader.AsU64();
	} else {
		str << "(cannot decode value)";
	}
	
	return str;
}