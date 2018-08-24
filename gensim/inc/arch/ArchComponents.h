/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   ArchComponents.h
 * Author: s0803652
 *
 * Created on 27 September 2011, 12:21
 */

#ifndef _ARCHCOMPONENTS_H
#define _ARCHCOMPONENTS_H

#include "genC/ir/IRType.h"

#include <string>

#include <stdint.h>

namespace gensim
{
	namespace arch
	{

		class MemoryDescription
		{
		public:
			std::string Name;
			uint32_t size_bytes;

			MemoryDescription(char *_name, uint32_t _size_bytes);
			virtual ~MemoryDescription();

		private:
		};


		class RegBankDescription
		{
		public:
			std::string Name;
			uint32_t count;
			genc::IRType Type;

			RegBankDescription(char *_name, uint32_t _count, uint32_t _bitwidth) : Name(_name), count(_count), Type(gensim::genc::IRType::GetIntType(_bitwidth)) {};

			RegBankDescription(char *_name, uint32_t _count, genc::IRType type) : Name(_name), count(_count), Type(type) {};
			virtual ~RegBankDescription();

		private:
		};

		class VectorRegBankDescription
		{
		public:
			std::string Name;
			uint32_t count;
			genc::IRType Type;
			uint32_t regwidth;

			// TODO: Constructor with 4 arguments causes issues
			VectorRegBankDescription(char *_name, uint32_t _count, uint32_t _bitwidth, uint32_t _regwidth) : Name(_name), count(_count), Type(gensim::genc::IRType::GetIntType(_bitwidth)), regwidth(_regwidth) {};

			VectorRegBankDescription(char *_name, uint32_t _count, genc::IRType type, uint32_t _regwidth) : Name(_name), count(_count), Type(type), regwidth(_regwidth) {};

			virtual ~VectorRegBankDescription();

		private:
		};

		class RegDescription
		{
		public:
			std::string Name;
			genc::IRType Type;

			RegDescription(char *name, genc::IRType type) : Name(name), Type(type) {}
		};
	}
}
#endif /* _ARCHCOMPONENTS_H */
