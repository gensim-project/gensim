#if 0
#include "define.h"
#include "genC/Parser.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSABuilder.h"
#include "genC/ssa/SSAContext.h"

#include "genC/ssa/statement/SSAStatements.h"

#include "genC/ir/IR.h"
#include "genC/ir/IRAction.h"
#include "isa/ISADescription.h"
#include "arch/ArchDescription.h"

#include <math.h>

#define MEMORY_ADDRESS_TYPE IRTypes::UInt64
#define PC_TYPE IRTypes::UInt64

namespace gensim
{
	namespace genc
	{

#if 0
		void GenCContext::LoadDefaultIntrinsics()
		{
			std::vector<IRParam> params;

			// Load memory read intrinsics
			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt8)));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_read_8", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt8)));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_read_8_user", IRTypes::UInt32, params), PublicScope, *this, true);

				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt8)));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_read_8s", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt16)));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_read_16", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{

				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt16)));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_read_16s", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt32)));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_read_32", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt32)));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_read_32_user", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt64)));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_read_64", IRTypes::UInt64, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			// Load memory write intrinsics
			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRTypes::UInt8));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_write_8", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRTypes::UInt8));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_write_8_user", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRTypes::UInt16));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_write_16", IRTypes::UInt32, params), PublicScope, *this, true);

				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_write_32", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_write_32_user", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("address", MEMORY_ADDRESS_TYPE));
				params.push_back(IRParam("data", IRTypes::UInt64));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("mem_write_64", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.clear();
				params.push_back(IRParam("address", PC_TYPE));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("write_pc", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.clear();
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("read_pc", PC_TYPE, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("enter_kernel_mode", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}
			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("enter_user_mode", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}
			{
				params.push_back(IRParam("mode", IRTypes::UInt8));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("set_cpu_mode", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("get_cpu_mode", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("category", IRTypes::UInt32));
				params.push_back(IRParam("data", IRTypes::UInt32));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("take_exception", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("int_level", IRTypes::UInt32));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("push_interrupt", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("pop_interrupt", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.clear();
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("halt_cpu", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.clear();
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("trap", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("index", IRTypes::UInt32));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("get_limm_32", IRTypes::UInt32, params), PublicScope, *this, true);

				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Float));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("float_is_qnan", IRTypes::UInt8, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Float));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("float_is_snan", IRTypes::UInt8, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Double));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("double_is_qnan", IRTypes::UInt8, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Double));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("double_is_snan", IRTypes::UInt8, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Double));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("double_sqrt", IRTypes::Double, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Float));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("float_sqrt", IRTypes::Float, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Double));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("double_abs", IRTypes::Double, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Float));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("float_abs", IRTypes::Float, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::UInt32));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("bitcast_u32_float", IRTypes::Float, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Float));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("bitcast_float_u32", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::UInt64));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("bitcast_u64_double", IRTypes::Double, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::Double));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("bitcast_double_u64", IRTypes::UInt64, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("numerator", IRTypes::Double));
				params.push_back(IRParam("denominator", IRTypes::Double));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("fmod", IRTypes::Double, params), PublicScope, *this, true);

				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::UInt32));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_popcount", IRTypes::UInt32, params), PublicScope, *this, true);

				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("val", IRTypes::UInt32));
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_clz", IRTypes::UInt32, params), PublicScope, *this, true);

				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("device", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("probe_device", IRTypes::UInt8, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;
				params.clear();
			}

			{
				params.push_back(IRParam("device", IRTypes::UInt32));
				params.push_back(IRParam("address", IRTypes::UInt32));
				params.push_back(IRParam("data", IRType::Ref(IRTypes::UInt32)));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("read_device", IRTypes::UInt8, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("device", IRTypes::UInt32));
				params.push_back(IRParam("address", IRTypes::UInt32));
				params.push_back(IRParam("data", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("write_device", IRTypes::UInt8, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("pend_interrupt", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("flush", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}
			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("flush_itlb", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}
			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("flush_dtlb", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}
			{
				params.push_back(IRParam("address", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("flush_itlb_entry", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}
			{
				params.push_back(IRParam("address", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("flush_dtlb_entry", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("lhs", IRTypes::UInt32));
				params.push_back(IRParam("rhs", IRTypes::UInt32));
				params.push_back(IRParam("carry_in", IRTypes::UInt8));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_adc_flags", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("lhs", IRTypes::UInt32));
				params.push_back(IRParam("rhs", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_ror32", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("lhs", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_zn_flags", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("rm", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_set_fp_rounding", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_get_fp_rounding", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}
			{
				params.push_back(IRParam("rm", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_set_fp_flush", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("value", IRTypes::Double));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_cast_double_to_u64_truncate", IRTypes::UInt64, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("value", IRTypes::Float));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_cast_float_to_u32_truncate", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_get_fp_flush", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("feature", IRTypes::UInt32));
				params.push_back(IRParam("level", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_set_feature", IRTypes::Void, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}

			{
				params.push_back(IRParam("feature", IRTypes::UInt32));

				IRHelperAction *intrinsic = new IRHelperAction(IRSignature("__builtin_get_feature", IRTypes::UInt32, params), PublicScope, *this, true);
				HelperTable[intrinsic->GetSignature().GetName()] = intrinsic;

				params.clear();
			}
		}
#endif

		using namespace ssa;

		SSAStatement *EmitMemoryRead8(const IRCallExpression &call, SSABuilder &bldr)
		{
			IRVariableExpression *v = dynamic_cast<IRVariableExpression *>(call.Args[1]);
			assert(v);
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), &call, addr, bldr.GetSymbol(v->Symbol), 1, false);
		}

		SSAStatement *EmitMemoryRead8User(const IRCallExpression &call, SSABuilder &bldr)
		{
			IRVariableExpression *v = dynamic_cast<IRVariableExpression *>(call.Args[1]);
			assert(v);
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), &call, addr, bldr.GetSymbol(v->Symbol), 1, false, true);
		}

		SSAStatement *EmitMemoryRead8s(const IRCallExpression &call, SSABuilder &bldr)
		{
			IRVariableExpression *v = dynamic_cast<IRVariableExpression *>(call.Args[1]);
			assert(v);
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), &call, addr, bldr.GetSymbol(v->Symbol), 1, true);
		}

		SSAStatement *EmitMemoryRead16(const IRCallExpression &call, SSABuilder &bldr)
		{
			IRVariableExpression *v = dynamic_cast<IRVariableExpression *>(call.Args[1]);
			assert(v);
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), &call, addr, bldr.GetSymbol(v->Symbol), 2, false);
		}

		SSAStatement *EmitMemoryRead16s(const IRCallExpression &call, SSABuilder &bldr)
		{
			IRVariableExpression *v = dynamic_cast<IRVariableExpression *>(call.Args[1]);
			assert(v);
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), &call, addr, bldr.GetSymbol(v->Symbol), 2, true);
		}

		SSAStatement *EmitMemoryRead32(const IRCallExpression &call, SSABuilder &bldr)
		{
			IRVariableExpression *v = dynamic_cast<IRVariableExpression *>(call.Args[1]);
			assert(v);
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), &call, addr, bldr.GetSymbol(v->Symbol), 4, false);
		}

		SSAStatement *EmitMemoryRead32User(const IRCallExpression &call, SSABuilder &bldr)
		{
			IRVariableExpression *v = dynamic_cast<IRVariableExpression *>(call.Args[1]);
			assert(v);
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), &call, addr, bldr.GetSymbol(v->Symbol), 4, false, true);
		}

		SSAStatement *EmitMemoryRead64(const IRCallExpression &call, SSABuilder &bldr)
		{
			IRVariableExpression *v = dynamic_cast<IRVariableExpression *>(call.Args[1]);
			assert(v);
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), &call, addr, bldr.GetSymbol(v->Symbol), 8, false);
		}

		SSAStatement *EmitMemoryWrite8(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *value = call.Args[1]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			if (value->GetType() != IRTypes::UInt8) value = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt8, value);
			return &SSAMemoryWriteStatement::CreateWrite(&bldr.GetBlock(), &call, addr, value, 1);
		}

		SSAStatement *EmitMemoryWrite8User(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *value = call.Args[1]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			if (value->GetType() != IRTypes::UInt8) value = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt8, value);
			return &SSAMemoryWriteStatement::CreateWrite(&bldr.GetBlock(), &call, addr, value, 1, true);
		}

		SSAStatement *EmitMemoryWrite16(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *value = call.Args[1]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			if (value->GetType() != IRTypes::UInt16) value = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt16, value);
			return &SSAMemoryWriteStatement::CreateWrite(&bldr.GetBlock(), &call, addr, value, 2);
		}

		SSAStatement *EmitMemoryWrite32(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *value = call.Args[1]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			if (value->GetType() != IRTypes::UInt32) value = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, value);
			return &SSAMemoryWriteStatement::CreateWrite(&bldr.GetBlock(), &call, addr, value, 4);
		}

		SSAStatement *EmitMemoryWrite32User(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *value = call.Args[1]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			if (value->GetType() != IRTypes::UInt32) value = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, value);
			return &SSAMemoryWriteStatement::CreateWrite(&bldr.GetBlock(), &call, addr, value, 4, true);
		}

		SSAStatement *EmitMemoryWrite64(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *value = call.Args[1]->EmitSSAForm(bldr);
			if (addr->GetType() != MEMORY_ADDRESS_TYPE) addr = new SSACastStatement(&bldr.GetBlock(), &call, MEMORY_ADDRESS_TYPE, addr);
			if (value->GetType() != IRTypes::UInt64) value = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt64, value);
			return &SSAMemoryWriteStatement::CreateWrite(&bldr.GetBlock(), &call, addr, value, 8);
		}

		SSAStatement *EmitHaltCpu(const IRCallExpression &call, SSABuilder &bldr)
		{
			return new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_HaltCpu);
		}

		SSAStatement *EmitReadPc(const IRCallExpression &call, SSABuilder &bldr)
		{
			return new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_ReadPc);
		}

		SSAStatement *EmitWritePc(const IRCallExpression &call, SSABuilder &bldr)
		{
			assert(call.Args.size() == 1);

			SSAStatement *arg0 = call.Args[0]->EmitSSAForm(bldr);
			if (arg0->GetType() != PC_TYPE) arg0 = new SSACastStatement(&bldr.GetBlock(), &call, PC_TYPE, arg0);

			SSAIntrinsicStatement *intrinsic = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_WritePc);
			intrinsic->AddArg(arg0);

			return intrinsic;
		}

		SSAStatement *EmitSetCpuMode(const IRCallExpression &call, SSABuilder &bldr)
		{
			assert(call.Args.size() == 1);

			SSAStatement *arg0 = call.Args[0]->EmitSSAForm(bldr);
			if (arg0->GetType() != IRTypes::UInt8) arg0 = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt8, arg0);

			SSAIntrinsicStatement *intrinsic = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_SetCpuMode);
			intrinsic->AddArg(arg0);

			return intrinsic;
		}

		SSAStatement *EmitGetCpuMode(const IRCallExpression &call, SSABuilder &bldr)
		{
			assert(call.Args.size() == 0);

			SSAIntrinsicStatement *intrinsic = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode);

			return intrinsic;
		}

		SSAStatement *EmitEnterUserMode(const IRCallExpression &call, SSABuilder &bldr)
		{
			assert(call.Args.size() == 0);

			SSAIntrinsicStatement *intrinsic = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_EnterUserMode);

			return intrinsic;
		}

		SSAStatement *EmitEnterKernelMode(const IRCallExpression &call, SSABuilder &bldr)
		{
			assert(call.Args.size() == 0);

			SSAIntrinsicStatement *intrinsic = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_EnterKernelMode);

			return intrinsic;
		}

		SSAStatement *EmitTakeException(const IRCallExpression &call, SSABuilder &bldr)
		{
			assert(call.Args.size() == 2);

			SSAStatement *arg0 = call.Args[0]->EmitSSAForm(bldr);
			if (arg0->GetType() != IRTypes::UInt32) arg0 = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, arg0);

			SSAStatement *arg1 = call.Args[1]->EmitSSAForm(bldr);
			if (arg1->GetType() != IRTypes::UInt32) arg1 = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, arg1);

			SSAIntrinsicStatement *intrinsic = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_TakeException);
			intrinsic->AddArg(arg0);
			intrinsic->AddArg(arg1);

			return intrinsic;
		}

		SSAStatement *EmitPushInterrupt(const IRCallExpression &call, SSABuilder &bldr)
		{
			assert(call.Args.size() == 1);
			return new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_PushInterrupt);
		}

		SSAStatement *EmitPopInterrupt(const IRCallExpression &call, SSABuilder &bldr)
		{
			assert(call.Args.size() == 0);
			return new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_PopInterrupt);
		}

		SSAStatement *EmitTrap(const IRCallExpression &call, SSABuilder &bldr)
		{
			return new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_Trap);
		}

		SSAStatement *EmitGetLimm(const IRCallExpression &call, SSABuilder &bldr)
		{
			UNIMPLEMENTED;
//	const IRExecuteAction *action = dynamic_cast<const IRExecuteAction *>(&call.GetScope().GetContainingAction());
//	SSAConstantStatement *constant = dynamic_cast<SSAConstantStatement *>(call.Args[0]->EmitSSAForm(bldr));
//	assert(constant && "Limm operations must have constant bank indices");
//	return new SSAReadStructMemberStatement(&bldr.GetBlock(), &call, bldr.GetSymbol(call.GetScope().GetSymbol("inst")), "LimmPtr", constant->Constant & 0xffffffff);
		}

		SSAStatement *EmitFloatIsSnan(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FloatIsSnan);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitFloatIsQnan(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FloatIsQnan);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitDoubleIsSnan(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_DoubleIsSnan);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitDoubleIsQnan(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_DoubleIsQnan);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitDoubleSqrt(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_DoubleSqrt);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitFloatSqrt(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FloatSqrt);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitDoubleAbs(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_DoubleAbs);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitFloatAbs(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FloatAbs);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitBitcastU32Float(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSACastStatement *stmt = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::Float, arg, SSACastStatement::Cast_Reinterpret);
			return stmt;
		}

		SSAStatement *EmitBitcastFloatU32(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSACastStatement *stmt = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, arg, SSACastStatement::Cast_Reinterpret);
			return stmt;
		}

		SSAStatement *EmitBitcastU64Double(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSACastStatement *stmt = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::Double, arg, SSACastStatement::Cast_Reinterpret);
			return stmt;
		}

		SSAStatement *EmitBitcastDoubleU64(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSACastStatement *stmt = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt64, arg, SSACastStatement::Cast_Reinterpret);
			return stmt;
		}

		SSAStatement *EmitFMod(const IRCallExpression &call, SSABuilder &bldr)
		{
			return new SSABinaryArithmeticStatement(&bldr.GetBlock(), &call, call.Args[0]->EmitSSAForm(bldr), call.Args[1]->EmitSSAForm(bldr), BinaryOperator::Modulo);
		}

		SSAStatement *EmitPopcount32(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_Popcount32);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement *EmitClz32(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *arg = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_Clz32);
			stmt->AddArg(arg);
			return stmt;
		}

		SSAStatement* EmitProbeDevice(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *dev_id = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_ProbeDevice);
			stmt->AddArg(dev_id);

			return stmt;
		}

		SSAStatement* EmitReadDevice(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *dev_id = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *addr = call.Args[1]->EmitSSAForm(bldr);

			if(dev_id->GetType() != IRTypes::UInt32) dev_id = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, dev_id);
			if(addr->GetType() != IRTypes::UInt32) addr = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, addr);

			IRVariableExpression *data = dynamic_cast<IRVariableExpression*>(call.Args[2]);
			assert(data);

			SSASymbol *target = bldr.GetSymbol(data->Symbol);

			SSADeviceReadStatement *stmt = new SSADeviceReadStatement(&bldr.GetBlock(), &call, dev_id, addr, target);

			return stmt;
		}

		SSAStatement* EmitWriteDevice(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *dev_id = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *addr = call.Args[1]->EmitSSAForm(bldr);
			SSAStatement *data = call.Args[2]->EmitSSAForm(bldr);

			if(dev_id->GetType() != IRTypes::UInt32) dev_id = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, dev_id);
			if(addr->GetType() != IRTypes::UInt32) addr = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, addr);
			if(data->GetType() != IRTypes::UInt32) data = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, data);

			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_WriteDevice);
			stmt->AddArg(dev_id);
			stmt->AddArg(addr);
			stmt->AddArg(data);

			return stmt;
		}

		SSAStatement *EmitPendInterrupt(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_PendIRQ);
			return stmt;
		}

		SSAStatement *EmitFlush(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_Flush);
			return stmt;
		}

		SSAStatement *EmitFlushITlb(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FlushITlb);
			return stmt;
		}

		SSAStatement *EmitFlushDTlb(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FlushDTlb);
			return stmt;
		}

		SSAStatement *EmitFlushITlbEntry(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FlushITlbEntry);
			stmt->AddArg(addr);
			return stmt;
		}

		SSAStatement *EmitFlushDTlbEntry(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *addr = call.Args[0]->EmitSSAForm(bldr);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FlushDTlbEntry);
			stmt->AddArg(addr);
			return stmt;
		}

		SSAStatement *EmitAdcWFlags(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *lhs   = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *rhs   = call.Args[1]->EmitSSAForm(bldr);
			SSAStatement *flags = call.Args[2]->EmitSSAForm(bldr);

			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_AdcWithFlags);

			stmt->AddArg(lhs);
			stmt->AddArg(rhs);
			stmt->AddArg(flags);

			return stmt;
		}

		SSAStatement *EmitRor32(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *lhs   = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *rhs   = call.Args[1]->EmitSSAForm(bldr);

			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_RotateRight);

			stmt->AddArg(lhs);
			stmt->AddArg(rhs);

			return stmt;
		}

		SSAStatement *EmitZNFlags(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *lhs   = call.Args[0]->EmitSSAForm(bldr);

			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_UpdateZN);

			stmt->AddArg(lhs);

			return stmt;
		}


		SSAStatement *EmitSetFPRounding(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *mode   = call.Args[0]->EmitSSAForm(bldr);

			if(mode->GetType() != IRTypes::UInt32) mode = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, mode);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FPSetRounding);

			stmt->AddArg(mode);

			return stmt;
		}
		SSAStatement *EmitGetFPRounding(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FPGetRounding);
			return stmt;
		}
		SSAStatement *EmitSetFPFlushMode(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *mode   = call.Args[0]->EmitSSAForm(bldr);

			if(mode->GetType() != IRTypes::UInt32) mode = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, mode);
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FPSetFlush);

			stmt->AddArg(mode);

			return stmt;
		}
		SSAStatement *EmitGetFPFlushMode(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_FPGetFlush);
			return stmt;
		}

		SSAStatement *EmitSetFeature(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *feature = call.Args[0]->EmitSSAForm(bldr);
			SSAStatement *level   = call.Args[1]->EmitSSAForm(bldr);

			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_SetFeature);

			stmt->AddArg(feature);
			stmt->AddArg(level);

			return stmt;
		}

		SSAStatement *EmitGetFeature(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *feature = call.Args[0]->EmitSSAForm(bldr);

			SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), &call, SSAIntrinsicStatement::SSAIntrinsic_GetFeature);

			stmt->AddArg(feature);

			return stmt;
		}

		SSAStatement *EmitCastDoubleToU64Truncate(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *value = call.Args[0]->EmitSSAForm(bldr);

			SSACastStatement *stmt = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt64, value, SSACastStatement::Cast_FloatTruncate);

			assert(stmt->Type == SSACastStatement::Cast_FloatTruncate);
			return stmt;
		}
		SSAStatement *EmitCastFloatToU32Truncate(const IRCallExpression &call, SSABuilder &bldr)
		{
			SSAStatement *value = call.Args[0]->EmitSSAForm(bldr);

			SSACastStatement *stmt = new SSACastStatement(&bldr.GetBlock(), &call, IRTypes::UInt32, value, SSACastStatement::Cast_FloatTruncate);

			assert(stmt->Type == SSACastStatement::Cast_FloatTruncate);
			return stmt;
		}


		SSAStatement *IRCallExpression::EmitIntrinsicCall(SSABuilder &bldr, const gensim::arch::ArchDescription &Arch) const
		{
			auto emitter = this->Target->Context.GetIntrinsicEmitter(Target->GetSignature().GetName());
			if (emitter == nullptr) {
				throw std::logic_error("Intrinsic emitter for '" + Target->GetSignature().GetName() + "' not available");
			}

			return emitter((IRIntrinsicAction *)this->Target, this, bldr, Arch);

#if 0
			if (Target->GetSignature().GetName() == "mem_read_8") return EmitMemoryRead8(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_read_8_user") return EmitMemoryRead8User(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_read_8s") return EmitMemoryRead8s(*this, bldr);

			if (Target->GetSignature().GetName() == "mem_read_16") return EmitMemoryRead16(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_read_16s") return EmitMemoryRead16s(*this, bldr);

			if (Target->GetSignature().GetName() == "mem_read_32") return EmitMemoryRead32(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_read_32_user") return EmitMemoryRead32User(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_read_64") return EmitMemoryRead64(*this, bldr);

			if (Target->GetSignature().GetName() == "mem_write_8") return EmitMemoryWrite8(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_write_8_user") return EmitMemoryWrite8User(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_write_16") return EmitMemoryWrite16(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_write_32") return EmitMemoryWrite32(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_write_32_user") return EmitMemoryWrite32User(*this, bldr);
			if (Target->GetSignature().GetName() == "mem_write_64") return EmitMemoryWrite64(*this, bldr);

			if (Target->GetSignature().GetName() == "halt_cpu") return EmitHaltCpu(*this, bldr);

			if (Target->GetSignature().GetName() == "read_pc") return EmitReadPc(*this, bldr);
			if (Target->GetSignature().GetName() == "write_pc") return EmitWritePc(*this, bldr);
			if (Target->GetSignature().GetName() == "set_cpu_mode") return EmitSetCpuMode(*this, bldr);
			if (Target->GetSignature().GetName() == "get_cpu_mode") return EmitGetCpuMode(*this, bldr);

			if (Target->GetSignature().GetName() == "enter_user_mode") return EmitEnterUserMode(*this, bldr);
			if (Target->GetSignature().GetName() == "enter_kernel_mode") return EmitEnterKernelMode(*this, bldr);

			if (Target->GetSignature().GetName() == "trap") return EmitTrap(*this, bldr);

			if (Target->GetSignature().GetName() == "get_limm_32") return EmitGetLimm(*this, bldr);


			if (Target->GetSignature().GetName() == "__builtin_popcount") return EmitPopcount32(*this, bldr);
			if (Target->GetSignature().GetName() == "__builtin_clz") return EmitClz32(*this, bldr);

			if(Target->GetSignature().GetName() == "float_is_snan") return EmitFloatIsSnan(*this, bldr);
			if(Target->GetSignature().GetName() == "float_is_qnan") return EmitFloatIsQnan(*this, bldr);

			if(Target->GetSignature().GetName() == "double_is_snan") return EmitDoubleIsSnan(*this, bldr);
			if(Target->GetSignature().GetName() == "double_is_qnan") return EmitDoubleIsQnan(*this, bldr);

			if(Target->GetSignature().GetName() == "double_sqrt") return EmitDoubleSqrt(*this, bldr);
			if(Target->GetSignature().GetName() == "float_sqrt") return EmitFloatSqrt(*this, bldr);

			if(Target->GetSignature().GetName() == "double_abs") return EmitDoubleAbs(*this, bldr);
			if(Target->GetSignature().GetName() == "float_abs") return EmitFloatAbs(*this, bldr);

			if(Target->GetSignature().GetName() == "bitcast_u32_float") return EmitBitcastU32Float(*this, bldr);
			if(Target->GetSignature().GetName() == "bitcast_float_u32") return EmitBitcastFloatU32(*this, bldr);
			if(Target->GetSignature().GetName() == "bitcast_u64_double") return EmitBitcastU64Double(*this, bldr);
			if(Target->GetSignature().GetName() == "bitcast_double_u64") return EmitBitcastDoubleU64(*this, bldr);

			if (Target->GetSignature().GetName() == "fmod") return EmitFMod(*this, bldr);

			if (Target->GetSignature().GetName() == "take_exception") return EmitTakeException(*this, bldr);

			if(Target->GetSignature().GetName() == "push_interrupt") return EmitPushInterrupt(*this, bldr);
			if(Target->GetSignature().GetName() == "pop_interrupt") return EmitPopInterrupt(*this, bldr);

			if(Target->GetSignature().GetName() == "probe_device") return EmitProbeDevice(*this, bldr);
			if(Target->GetSignature().GetName() == "read_device") return EmitReadDevice(*this, bldr);
			if(Target->GetSignature().GetName() == "write_device") return EmitWriteDevice(*this, bldr);

			if(Target->GetSignature().GetName() == "pend_interrupt") return EmitPendInterrupt(*this, bldr);

			if(Target->GetSignature().GetName() == "flush") return EmitFlush(*this, bldr);
			if(Target->GetSignature().GetName() == "flush_itlb") return EmitFlushITlb(*this, bldr);
			if(Target->GetSignature().GetName() == "flush_dtlb") return EmitFlushDTlb(*this, bldr);
			if(Target->GetSignature().GetName() == "flush_itlb_entry") return EmitFlushITlbEntry(*this, bldr);
			if(Target->GetSignature().GetName() == "flush_dtlb_entry") return EmitFlushDTlbEntry(*this, bldr);

			if(Target->GetSignature().GetName() == "__builtin_adc_flags") return EmitAdcWFlags(*this, bldr);
			if(Target->GetSignature().GetName() == "__builtin_ror32") return EmitRor32(*this, bldr);
			if(Target->GetSignature().GetName() == "__builtin_zn_flags") return EmitZNFlags(*this, bldr);

			if(Target->GetSignature().GetName() == "__builtin_set_fp_rounding") return EmitSetFPRounding(*this, bldr);
			if(Target->GetSignature().GetName() == "__builtin_get_fp_rounding") return EmitGetFPRounding(*this, bldr);

			if(Target->GetSignature().GetName() == "__builtin_set_fp_flush") return EmitSetFPFlushMode(*this, bldr);
			if(Target->GetSignature().GetName() == "__builtin_get_fp_flush") return EmitGetFPFlushMode(*this, bldr);

			if(Target->GetSignature().GetName() == "__builtin_get_feature") return EmitGetFeature(*this, bldr);
			if(Target->GetSignature().GetName() == "__builtin_set_feature") return EmitSetFeature(*this, bldr);

			if(Target->GetSignature().GetName() == "__builtin_cast_double_to_u64_truncate") return EmitCastDoubleToU64Truncate(*this, bldr);
			if(Target->GetSignature().GetName() == "__builtin_cast_float_to_u32_truncate") return EmitCastFloatToU32Truncate(*this, bldr);

			assert(false && "Unrecognized intrinsic");
			return NULL;
#endif
		}
	}
}
#endif