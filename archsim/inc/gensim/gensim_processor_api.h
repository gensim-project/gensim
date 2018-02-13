#undef mem_write_64

#undef read_register
#undef read_register_nt
#undef write_register
#undef write_register_nt
#undef read_register_bank
#undef read_register_bank_nt
#undef write_register_bank
#undef write_register_bank_nt
#undef read_vector_register_bank
#undef read_vector_register_bank_nt
#undef write_vector_register_bank
#undef write_vector_register_bank_nt

#undef trace_rb_write
#undef trace_rb_read

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

#undef GENSIM_TRACE
#ifdef GENSIM_TRACE

#define mem_write_64(ADDR, DATA) do { GetMemoryModel().Write64((ADDR), (DATA)); GetTraceManager()->Trace_Mem_Write(true, (ADDR), (DATA)); } while (0)

#define read_register(REG) (GetTraceManager()->Trace_Reg_Read(true, TRACE_ID_##REG, (PREFIX reg_state.REG)), (PREFIX reg_state.REG))
#define read_register_nt(REG) (GetTraceManager()->Trace_Reg_Read(false, TRACE_ID_##REG, (PREFIX reg_state.REG)), (PREFIX reg_state.REG))
#define write_register(REG, VALUE) do { GetTraceManager()->Trace_Reg_Write(true, TRACE_ID_##REG, (VALUE)); (PREFIX reg_state.REG) = (VALUE); } while (0)
#define write_register_nt(REG, VALUE) do { GetTraceManager()->Trace_Reg_Write(false, TRACE_ID_##REG, (VALUE)); (PREFIX reg_state.REG) = (VALUE); } while (0)

#define read_register_bank(REG, REGNUM) (GetTraceManager()->Trace_Bank_Reg_Read(true, TRACE_ID_##REG, REGNUM, (PREFIX reg_state.REG)[REGNUM]), ((PREFIX reg_state.REG)[REGNUM]))
#define read_register_bank_nt(REG, REGNUM) (GetTraceManager()->Trace_Bank_Reg_Read(false, TRACE_ID_##REG, REGNUM, (PREFIX reg_state.REG)[REGNUM]), ((PREFIX reg_state.REG)[REGNUM]))

#define write_register_bank(REG, REGNUM, VALUE) do { GetTraceManager()->Trace_Bank_Reg_Write(true, TRACE_ID_##REG, REGNUM, (VALUE)); (PREFIX reg_state.REG)[REGNUM] = (VALUE); } while (0)
#define write_register_bank_nt(REG, REGNUM, VALUE) do { GetTraceManager()->Trace_Bank_Reg_Write(false, TRACE_ID_##REG, REGNUM, (VALUE)); (PREFIX reg_state.REG)[REGNUM] = (VALUE); } while (0)

#define read_vector_register_bank(REG, REGNUM, REGINDEX) ((PREFIX reg_state.REG)[REGNUM][REGINDEX])
#define read_vector_register_bank_nt(REG, REGNUM, REGINDEX) ((PREFIX reg_state.REG)[REGNUM][REGINDEX])
#define write_vector_register_bank(REG, REGNUM, REGINDEX, VALUE) (PREFIX reg_state.REG)[REGNUM][REGINDEX] = (VALUE)
#define write_vector_register_bank_nt(REG, REGNUM, REGINDEX, VALUE) (PREFIX reg_state.REG)[REGNUM][REGINDEX] = (VALUE)

#else

#define mem_write_64(ADDR, DATA) GetMemoryModel().Write64((ADDR), (DATA))

#define read_register(REG) (PREFIX read_register_##REG())
#define write_register(REG, VALUE) (PREFIX write_register_##REG(VALUE))

#ifdef GENSIM_PC_CHECK
#ifndef GENSIM_PC_REG
#error "No PC register defined"
#endif

#define read_register_bank(REG, REGNUM) (assert(REGNUM != GENSIM_PC_REG), (PREFIX reg_state.REG)[REGNUM])
#define read_register_bank_nt(REG, REGNUM) (assert(REGNUM != GENSIM_PC_REG), (PREFIX reg_state.REG)[REGNUM])

#else

#define read_register_bank(REG, REGNUM) ((PREFIX read_register_bank_##REG(REGNUM)))

#endif

#define write_register_bank(REG, REGNUM, VALUE) ((PREFIX write_register_bank_##REG(REGNUM, VALUE)))
#define read_vector_register_bank(REG, REGNUM, REGINDEX) ((PREFIX reg_state.REG)[REGNUM][REGINDEX])
#define write_vector_register_bank(REG, REGNUM, REGINDEX, VALUE) (PREFIX reg_state.REG)[REGNUM][REGINDEX] = (VALUE)

#endif
