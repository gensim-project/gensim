//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Thread-safe Counter classes responsible for maintaining and  instantiating
// all kinds of profiling counters in a generic way.
//
// There are two types of Counters, 32-bit counters (i.e. Counter), and
// 64-bit counters (i.e. Counter64).
//
// =====================================================================

#ifndef INC_UTIL_COUNTER_H_
#define INC_UTIL_COUNTER_H_

#include <string>

#if CONFIG_LLVM
#include <llvm/IR/IRBuilder.h>
#endif

namespace archsim
{
	namespace translate
	{
		namespace llvm
		{
			class LLVMTranslationContext;
		}
	}
	namespace util
	{
		class Counter
		{
		public:
			explicit Counter();

			uint32_t* get_ptr()
			{
				return &count_;
			}

			inline uint32_t get_value() const
			{
				return count_;
			}

			void set_value(uint32_t count)
			{
				count_ = count;
			}

			inline void inc()
			{
				++count_;
			}

			inline void inc(uint32_t incr)
			{
				count_ += incr;
			}

			inline void dec()
			{
				--count_;
			}

			inline void dec(uint32_t decr)
			{
				count_ -= decr;
			}

			// Reset to 0
			void clear()
			{
				count_ = 0;
			}

		private:
			uint32_t count_;
		};

// -------------------------------------------------------------------------
// Counter class encapsulating common operations on 64 bit counters
//
		class Counter64
		{
		public:
			explicit Counter64();

			inline uint64_t *get_ptr()
			{
				return &count_;
			}

			inline uint64_t get_value() const
			{
				return count_;
			}

			void set_value(uint64_t count)
			{
				count_ = count;
			}

			inline void inc()
			{
				++count_;
			}

			inline void inc(uint64_t incr)
			{
				count_ += incr;
			}

			inline void dec()
			{
				--count_;
			}

			inline void dec(uint64_t decr)
			{
				count_ -= decr;
			}

			// Reset to 0
			void clear()
			{
				count_ = 0;
			}

			inline Counter64& operator++(int)
			{
				inc();
				return *this;
			}

#if CONFIG_LLVM
			void emit_inc(archsim::translate::llvm::LLVMTranslationContext& ctx, llvm::IRBuilder<>& bldr);
#endif

		private:
			uint64_t count_;
		};

	}
}  // namespace archsim::util

#endif  // INC_UTIL_COUNTER_H_
