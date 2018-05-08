/*
 * LowerTypes.h
 *
 *  Created on: 3 Dec 2015
 *      Author: harry
 */

#ifndef LowerType
#error LowerType must be defined!
#else
LowerType(Scm)
LowerType(SetCpuFeature)
LowerType(IntCheck)
LowerType(Trap)
LowerType(CMov)
LowerType(Mov)
LowerType(UDiv)
LowerType(SDiv)
LowerType(IMul)
LowerType(UMul)
LowerType(Mod)
LowerType(ReadReg)
LowerType(Dispatch)
LowerType(Ret)
LowerType(AddSub)
LowerType(WriteReg)
LowerType(Jmp)
LowerType(Branch)
LowerType(Call)
LowerType(LoadPC)
LowerType(IncPC)
LowerType(WriteDevice)
LowerType(ReadDevice)
LowerType(ProbeDevice)
LowerType(Clz)
LowerType(Bitwise)
LowerType(Trunc)
LowerType(Extend)
LowerType(Compare)
LowerType(CompareSigned)
LowerType(Shift)
LowerType(FlushTlb)
LowerType(FlushTlbEntry)
LowerType(AdcFlags)
LowerType(ZNFlags)
LowerType(TakeException)
LowerType(Verify)
LowerType(Count)

LowerType(ReadMemGeneric)
LowerType(WriteMemGeneric)
LowerType(ReadUserMemGeneric)
LowerType(WriteUserMemGeneric)

LowerType(ReadMemCache)
LowerType(WriteMemCache)
LowerType(ReadUserMemCache)
LowerType(WriteUserMemCache)

LowerTypeTS(ReadMemUser)
LowerTypeTS(WriteMemUser)

LowerType(FMul)
LowerType(FDiv)
LowerType(FAdd)
LowerType(FSub)
LowerType(FCmp)
LowerType(FSqrt)

LowerType(FCvt_SI_To_F)
LowerType(FCvt_F_To_SI)
LowerType(FCvt_UI_To_F)
LowerType(FCvt_F_To_UI)
LowerType(FCvt_S_To_D)
LowerType(FCvt_D_To_S)

LowerType(FCtrl_SetRound)
LowerType(FCtrl_GetRound)
LowerType(FCtrl_SetFlush)
LowerType(FCtrl_GetFlush)

LowerType(VAddF)
LowerType(VAddI)
LowerType(VSubF)
LowerType(VSubI)
LowerType(VMulF)
LowerType(VMulI)

#endif
