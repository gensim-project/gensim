//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description: Globally used constants and definitions.
//
// =====================================================================

#ifndef INC_GLOBALS_H_
#define INC_GLOBALS_H_

#include <stdint.h>

namespace archsim
{
// ---------------------------------------------------------------------------
// Constants
//
	const int B = 1;	      // Byte
	const int KB = 1024;	  // Kilobyte
	const int MB = KB * KB;       // Megabyte
	const int GB = KB * KB * KB;  // Gigabyte

	const int kBitsPerByte = 8;
	const int kPointerSize = sizeof(void*);

	typedef uint8_t byte;
	typedef byte* Address;

	const uint32_t kInvalidPcAddress = 0x1;  // 0x1 is used to denote an illegal PC
}

#endif  // INC_GLOBALS_H_
