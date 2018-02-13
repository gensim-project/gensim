#include <cassert>
#include "define.h"

namespace archsim
{
	namespace util
	{

		template <int Start, int Length, typename InType, typename OutType>
		struct Bitfield {
		public:
			OutType Get()
			{
				return UNSIGNED_BITS(_backing_store, Start, Length);
			}
			void Put(OutType val)
			{
				_backing_store ^= Get() << Start;
				_backing_store |= (UNSIGNED_BITS(val, 0, Length)) << Start;
			}

		private:
			InType _backing_store;
		};

		/**
		 * Class to aid decoding of arbitrary length instructions. Abstracts the
		 * instruction word array to a single value and allows reading of
		 * arbitrary portions of that value, with bits numbered from 0 (MSB) to
		 * sizeof(BackingType) * BackingArrayLen (LSB).
		 */
		template <typename BackingType, int BackingArrayLen>
		class BitArray
		{
		private:
			typedef BackingType array_type[BackingArrayLen];

		public:
			template <typename RType, unsigned Start, unsigned Length>
			RType GetBits()
			{
				// first, determine if we can get the value in one operation. This is
				// possible if
				//(Start + Length - 1) % (sizeof(BackingType)) == (Start % sizeof(BackingType))
				//
				// i.e., if the value ends in the same 'chunk' as it starts
				const int backing_type_len = sizeof(BackingType) * 8;
				// printf("%u + %u %% %u == %u %% %u\n",Start, Length, backing_type_len, Start, backing_type_len);
				if (((Start + Length - 1) / backing_type_len) == (Start / backing_type_len)) {
					// in this case, we can immediately return a value
					BackingType chunk = _backing_store[Start / backing_type_len];
					const int hbit = backing_type_len - (Start % backing_type_len) - 1;
					const int lbit = hbit - Length + 1;
					return (RType)(UNSIGNED_BITS(chunk, hbit, lbit));
				} else {
					assert(BackingArrayLen > 1);
					// otherwise, we need to assemble the value from multiple chunks
					RType val = 0;

					unsigned start = Start;
					while (start < Length) {
						unsigned curr_backing_index = Start / backing_type_len;  // figure out which chunk we should start in
						// figure out how far through the current chunk we are
						unsigned chunk_start = Start % backing_type_len;
						unsigned chunk_length = backing_type_len - chunk_start;
						val |= (RType)(UNSIGNED_BITS(_backing_store[curr_backing_index], chunk_start, chunk_start + chunk_length)) << (Length - chunk_start - chunk_length);
						start += chunk_length;
					}

					return val;
				}
			}

			BitArray(array_type &backing_array) : _backing_store(backing_array)
			{
				assert(BackingArrayLen > 0);
			}

		private:
			array_type &_backing_store;
			BitArray();
			BitArray(const BitArray &);
		};
	}
}
