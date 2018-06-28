/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef BYTEBYBYTEDECODEGENERATOR_H_
#define BYTEBYBYTEDECODEGENERATOR_H_

#include "DecodeGenerator.h"

namespace gensim
{
	namespace generator
	{

// Byte by byte decoding:
// Load the first byte of the instruction and attempt to decode it. If we find an instruction, we're done. Otherwise, continue to load one byte at a time and move through the decode tree
// until an instruction is detected, or if we reach the length of the longest instruction format and have still not decoded an instruction

		class ByteByByteDecodeGenerator : public DecodeGenerator
		{
		public:
			ByteByByteDecodeGenerator(GenerationManager &man) : DecodeGenerator(man, "decode_bbb") {}

			virtual bool GenerateDecodeHeader(std::stringstream &str) const;
			virtual bool GenerateDecodeSource(std::stringstream &str) const;

			virtual bool GenerateDecodeTree(DecodeNode &tree, std::stringstream &stream, int &i) const;
		};
	}
}

#endif /* BYTEBYBYTEDECODEGENERATOR_H_ */
