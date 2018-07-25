/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

ENUM_ENTRY(HaltCpu)
ENUM_ENTRY(ReadPc)
ENUM_ENTRY(WritePc)

ENUM_ENTRY(Popcount32)
ENUM_ENTRY(Clz32)
ENUM_ENTRY(Clz64)
ENUM_ENTRY(BSwap32)
ENUM_ENTRY(BSwap64)

ENUM_ENTRY(SetCpuMode)
ENUM_ENTRY(GetCpuMode)

ENUM_ENTRY(Trap)
ENUM_ENTRY(TakeException)

ENUM_ENTRY(EnterUserMode)
ENUM_ENTRY(EnterKernelMode)

ENUM_ENTRY(ProbeDevice)
ENUM_ENTRY(WriteDevice)

ENUM_ENTRY(SetFeature)
ENUM_ENTRY(GetFeature)

ENUM_ENTRY(PushInterrupt)
ENUM_ENTRY(PopInterrupt)
ENUM_ENTRY(PendIRQ)
ENUM_ENTRY(TriggerIRQ)

ENUM_ENTRY(InvalidateDCache)
ENUM_ENTRY(InvalidateDCacheEntry)
ENUM_ENTRY(InvalidateICache)
ENUM_ENTRY(InvalidateICacheEntry)
ENUM_ENTRY(FlushContextID)
ENUM_ENTRY(SetContextID)

ENUM_ENTRY(FloatIsSnan)
ENUM_ENTRY(FloatIsQnan)

ENUM_ENTRY(DoubleIsSnan)
ENUM_ENTRY(DoubleIsQnan)

ENUM_ENTRY(DoubleSqrt)
ENUM_ENTRY(FloatSqrt)

ENUM_ENTRY(DoubleAbs)
ENUM_ENTRY(FloatAbs)

ENUM_ENTRY(FPGetRounding)
ENUM_ENTRY(FPSetRounding)
ENUM_ENTRY(FPGetFlush)
ENUM_ENTRY(FPSetFlush)

