/*
 * File:   gensim_decode.h
 * Author: s0803652
 *
 * Created on 20 October 2011, 17:30
 */

#ifndef _GENSIM_DECODE_H
#define _GENSIM_DECODE_H

#include <list>
#include <stdint.h>

typedef std::list<std::pair<bool, uint8_t> > dynamic_pred_queue_t;

namespace gensim
{

	class Processor;

#pragma pack(push)
#pragma pack(1)
	class BaseDecode
	{
	public:
		static uint64_t decode_specialisation_hit;

		uint16_t Instr_Code;
		uint8_t Instr_Length;
		uint8_t isa_mode;
		bool Block_Cond;

#define STORE_IR
#define FLAG_PROPERTIES
#ifdef STORE_IR
		inline INSTRUCTION_WORD_TYPE GetIR() const
		{
			return ir;
		}
		inline void SetIR(INSTRUCTION_WORD_TYPE _ir)
		{
			ir = _ir;
		}
#else
		inline INSTRUCTION_WORD_TYPE GetIR() const
		{
			return 0;
		}
		inline void SetIR(INSTRUCTION_WORD_TYPE _ir) {}
#endif

#ifdef ENABLE_LIMM_OPERATIONS
		void *LimmPtr;
		uint8 LimmBytes;
#endif

		BaseDecode()
			: Instr_Code(0),
			  Instr_Length(0),
			  Block_Cond(false)
#ifdef ENABLE_LIMM_OPERATIONS
			,
			  LimmPtr(0),
			  LimmBytes(0)
#endif
			,
			  isa_mode(0) {}

#ifdef FLAG_PROPERTIES
		enum FlagEnum {
			FLAG_END_OF_BLOCK = 0x1,
			FLAG_USES_PC = 0x2,
			FLAG_IS_PREDICATED = 0x4
		};

		inline bool GetEndOfBlock() const
		{
			return ((flags & FLAG_END_OF_BLOCK) != 0);
		}
		inline bool GetUsesPC() const
		{
			return ((flags & FLAG_USES_PC) != 0);
		}
		inline bool GetIsPredicated() const
		{
			return ((flags & FLAG_IS_PREDICATED) != 0);
		}

		inline uint32_t GetPredicateInfo() const
		{
			return _predicate_info;
		}
		inline void SetPredicateInfo(uint32_t status)
		{
			_predicate_info = status;
		}

		inline void SetEndOfBlock()
		{
			flags = (uint8_t)(flags | FLAG_END_OF_BLOCK);
		}
		inline void SetUsesPC()
		{
			flags = (uint8_t)(flags | FLAG_USES_PC);
		}
		inline void SetIsPredicated()
		{
			flags = (uint8_t)(flags | FLAG_IS_PREDICATED);
		}

		inline void ClearEndOfBlock()
		{
			flags = (uint8_t)(flags & ~FLAG_END_OF_BLOCK);
		}
		inline void ClearUsesPC()
		{
			flags = (uint8_t)(flags & ~FLAG_USES_PC);
		}
		inline void ClearIsPredicated()
		{
			flags = (uint8_t)(flags & ~FLAG_IS_PREDICATED);
		}
#else
		inline bool GetEndOfBlock() const
		{
			return IsEndOfBlock;
		}
		inline bool GetUsesPC() const
		{
			return UsesPC;
		}
		inline bool GetIsPredicated() const
		{
			return IsPredicated;
		}

		inline void SetEndOfBlock()
		{
			IsEndOfBlock = 1;
			;
		}
		inline void SetUsesPC()
		{
			UsesPC = 1;
			;
		}
		inline void SetIsPredicated()
		{
			IsPredicated = 1;
		}

		inline void ClearEndOfBlock()
		{
			IsEndOfBlock = 0;
		}
		inline void ClearUsesPC()
		{
			UsesPC = 0;
		}
		inline void ClearIsPredicated()
		{
			IsPredicated = 0;
		}
#endif

#ifdef STORE_IR
		INSTRUCTION_WORD_TYPE ir;
#endif

#ifndef FLAG_PROPERTIES
		bool IsEndOfBlock;
		bool UsesPC;
		bool IsPredicated;
#endif
	private:
		uint32_t _predicate_info;
#ifdef FLAG_PROPERTIES
		uint8_t flags;
#endif
	};
#pragma pack(pop)
}

#endif /* _GENSIM_DECODE_H */
