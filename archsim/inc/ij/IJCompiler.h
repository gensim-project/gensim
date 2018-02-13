/*
 * File:   IJCompiler.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 17:04
 */

#ifndef IJCOMPILER_H
#define	IJCOMPILER_H

#include "define.h"

namespace archsim
{
	namespace ij
	{
		class IJIR;

		template<typename TPtr = uint8_t>
		class OutputBuffer
		{
		public:
			OutputBuffer(TPtr *start) : start_ptr(start), cur_ptr(start) { }

			inline TPtr *E(TPtr v)
			{
				TPtr *elem_start_ptr = cur_ptr;
				*cur_ptr++ = v;
				return elem_start_ptr;
			}

			template<typename TOtherPtr>
			inline TPtr *EO(TOtherPtr v)
			{
				TOtherPtr *ptr = (TOtherPtr *)cur_ptr;
				*ptr = v;
				cur_ptr += sizeof(TOtherPtr);
				return (TPtr *)ptr;
			}

			inline TPtr *E8(uint8_t v)
			{
				return EO<uint8_t>(v);
			}

			inline TPtr *E16(uint16_t v)
			{
				return EO<uint16_t>(v);
			}

			inline TPtr *E32(uint32_t v)
			{
				return EO<uint32_t>(v);
			}

			inline TPtr *E64(uint64_t v)
			{
				return EO<uint64_t>(v);
			}

			inline TPtr *GetStart() const
			{
				return start_ptr;
			}
			inline TPtr *GetCurrent() const
			{
				return cur_ptr;
			}
			inline uint32_t GetSize() const
			{
				return (uint32_t)((unsigned long)cur_ptr - (unsigned long)start_ptr);
			}
		private:
			TPtr *start_ptr;
			TPtr *cur_ptr;
		};

		class IJCompiler
		{
		public:
			virtual uint32_t Compile(IJIR& ir, uint8_t *buffer) = 0;
		};
	}
}

#endif	/* IJCOMPILER_H */

