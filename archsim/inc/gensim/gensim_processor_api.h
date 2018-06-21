/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

// allow a prefix to allow use of macros out of the context of the cpu object
#ifndef PREFIX
#define PREFIX
#endif

#define registers() (reg_state)

#ifndef __ROTATE_FUNS
#define __ROTATE_FUNS
static inline uint32_t __ror32 (uint32_t x, uint32_t n)
{
	return (x<<n) | (x>>(32-n));
}
#endif

#ifndef __BITCAST_FUNS
#define __BITCAST_FUNS
static inline float bitcast_u32_float(uint32_t x)
{
	union {
		float f;
		uint32_t u;
	};
	u = x;
	return f;
}
static inline uint32_t bitcast_float_u32(float x)
{
	union {
		float f;
		uint32_t u;
	};
	f = x;
	return u;
}
static inline double bitcast_u64_double(uint64_t x)
{
	union {
		double f;
		uint64_t u;
	};
	u = x;
	return f;
}
static inline uint64_t bitcast_double_u64(double x)
{
	union {
		double f;
		uint64_t u;
	};
	f = x;
	return u;
}
#endif

#define float_is_snan(x) ((bitcast_float_u32(x) & 0x7fc00000) == 0x7f800000)
#define float_is_qnan(x) ((bitcast_float_u32(x) & 0x7fc00000) == 0x7fc00000)

#define double_is_snan(x) ((bitcast_double_u64(x) & 0x7ffc000000000000ULL) == 0x7ff8000000000000ULL)
#define double_is_qnan(x) ((bitcast_double_u64(x) & 0x7ffc000000000000ULL) == 0x7ffc000000000000ULL)

#define double_sqrt(x) (sqrt((double)x))
#define float_sqrt(x) (sqrt((float)x))

#define double_abs(x) (fabs((double)x))
#define float_abs(x) (fabs((float)x))

#ifndef __GENC_FUNS
#define __GENC_FUNS

extern "C" uint32_t genc_adc_flags(uint32_t lhs, uint32_t rhs, uint32_t carry_in);

#endif
