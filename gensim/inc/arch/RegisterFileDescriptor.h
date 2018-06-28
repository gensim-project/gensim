/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * RegisterFileDescriptor.h
 *
 *  Created on: 18 Sep 2015
 *      Author: harry
 */

#ifndef INC_ARCH_REGISTERFILEDESCRIPTOR_H_
#define INC_ARCH_REGISTERFILEDESCRIPTOR_H_

struct RegisterSlotDescriptorStruct {
	uint8_t type; // ALWAYS 1
	uint8_t width_in_bytes;
	uint8_t offset_in_bytes;
	uint8_t name_length_in_bytes;
	uint8_t tag_length_in_bytes;
	uint8_t names[];
};

struct RegisterBankDescriptorStruct {
	uint8_t type; //ALWAYS 2
	uint8_t width_in_bytes;
	uint8_t stride_in_bytes;
	uint8_t offset_in_bytes;
	uint8_t count;
	uint8_t name_length_in_bytes;
	uint8_t name[];
};

struct RegisterSpaceDescriptorStruct {
	uint8_t type; //ALWAYS 3
	uint8_t children_count;
};

struct RegisterFileDescriptorStruct {
	uint8_t type; //ALWAYS 4
	uint8_t children_count;
};



#endif /* INC_ARCH_REGISTERFILEDESCRIPTOR_H_ */
