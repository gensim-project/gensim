#include "define.h"
#include "genC/Intrinsics.h"
#include "genC/Parser.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRSignature.h"
#include "genC/ir/IR.h"
#include "genC/ssa/SSABuilder.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

static std::map<std::string, SSAIntrinsicStatement::IntrinsicType> __intrinsic_type_mapping;

static ssa::SSAStatement *DefaultIntrinsicEmitter(const IRIntrinsicAction *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr, const IRType &wordtype)
{
	std::vector<SSAStatement *> arg_statements;

	auto params = intrinsic->GetSignature().GetParams();
	if (params.size() != call->Args.size()) throw std::logic_error("Argument/Parameter count mismatch when emitting intrinsic '" + intrinsic->GetSignature().GetName() + "'");

	int arg_index = 0;
	for (const auto& arg : call->Args) {
		SSAStatement *arg_statement = arg->EmitSSAForm(bldr);

		auto param = params.at(arg_index++);
		if (arg_statement->GetType() != param.GetType()) {
			const auto& dn = arg_statement->GetDiag();
			arg_statement = new SSACastStatement(&bldr.GetBlock(), param.GetType(), arg_statement);
			arg_statement->SetDiag(dn);
		}

		arg_statements.push_back(arg_statement);
	}

	if (__intrinsic_type_mapping.find(call->TargetName) == __intrinsic_type_mapping.end()) throw std::logic_error("Default intrinsic mapping for '" + call->TargetName + "' does not exist.");
	SSAIntrinsicStatement::IntrinsicType intrinsic_type = __intrinsic_type_mapping.at(call->TargetName);

	SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), intrinsic_type);
	stmt->SetDiag(call->Diag());

	for (const auto& arg : arg_statements) {
		stmt->AddArg(arg);
	}

	return stmt;
}

static ssa::SSAStatement *MemoryIntrinsicEmitter(const IRIntrinsicAction *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr, const IRType &wordtype)
{
	bool is_memory_write, is_user_access;
	uint8_t data_width;
	IRType data_type;

	if (call->TargetName == "mem_read_8") {
		is_memory_write = false;
		is_user_access = false;
		data_width = 1;
		data_type = IRTypes::UInt8;
	} else if (call->TargetName == "mem_read_16") {
		is_memory_write = false;
		is_user_access = false;
		data_width = 2;
		data_type = IRTypes::UInt16;
	} else if (call->TargetName == "mem_read_32") {
		is_memory_write = false;
		is_user_access = false;
		data_width = 4;
		data_type = IRTypes::UInt32;
	} else if (call->TargetName == "mem_read_64") {
		is_memory_write = false;
		is_user_access = false;
		data_width = 8;
		data_type = IRTypes::UInt64;
	} else if (call->TargetName == "mem_write_8") {
		is_memory_write = true;
		is_user_access = false;
		data_width = 1;
		data_type = IRTypes::UInt8;
	} else if (call->TargetName == "mem_write_16") {
		is_memory_write = true;
		is_user_access = false;
		data_width = 2;
		data_type = IRTypes::UInt16;
	} else if (call->TargetName == "mem_write_32") {
		is_memory_write = true;
		is_user_access = false;
		data_width = 4;
		data_type = IRTypes::UInt32;
	} else if (call->TargetName == "mem_write_64") {
		is_memory_write = true;
		is_user_access = false;
		data_width = 8;
		data_type = IRTypes::UInt64;
	} else {
		throw std::logic_error("Unsupported memory intrinsic: " + call->TargetName);
	}

	auto mem_interface_read = dynamic_cast<IRVariableExpression*>(call->Args[0]);
	if(mem_interface_read == nullptr) {
		throw std::logic_error("First argument of mem intrinsic must be a constant memory interface ID");
	}
	
	auto mem_interface_id = call->GetScope().GetContainingAction().Context.GetConstant(mem_interface_read->Symbol->GetLocalName()).second;
	
	SSAStatement *addr = call->Args[1]->EmitSSAForm(bldr);
	if (addr->GetType() != wordtype) {
		const auto& dn = addr->GetDiag();
		addr = new SSACastStatement(&bldr.GetBlock(), wordtype, addr);
		addr->SetDiag(dn);
	}

	const gensim::arch::MemoryInterfaceDescription *interface = bldr.Context.GetArchDescription().GetMemoryInterfaces().GetByID(mem_interface_id);
	if(interface == nullptr) {
		throw std::logic_error("could not find an interface with id " + std::to_string(mem_interface_id));
	}
	
	if (is_memory_write) {
		SSAStatement *value = call->Args[2]->EmitSSAForm(bldr);
		if (value->GetType() != data_type) {
			const auto& dn = value->GetDiag();
			value = new SSACastStatement(&bldr.GetBlock(), data_type, value);
			value->SetDiag(dn);
		}

		return &SSAMemoryWriteStatement::CreateWrite(&bldr.GetBlock(), addr, value, data_width, interface);
	} else {
		IRVariableExpression *v = dynamic_cast<IRVariableExpression *> (call->Args[2]);
		if (!v) throw std::logic_error("Value argument to memory read intrinsic was not an IRVariableExpression");

		return &SSAMemoryReadStatement::CreateRead(&bldr.GetBlock(), addr, bldr.GetSymbol(v->Symbol), data_width, false, interface);
	}
}

static ssa::SSAStatement *DeviceIntrinsicEmitter(const IRIntrinsicAction *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr, const IRType &wordtype)
{
	SSAStatement *dev_id = call->Args[0]->EmitSSAForm(bldr);
	if (dev_id->GetType() != IRTypes::UInt32) {
		const auto& dn = dev_id->GetDiag();
		dev_id = new SSACastStatement(&bldr.GetBlock(), IRTypes::UInt32, dev_id);
		dev_id->SetDiag(dn);
	}

	if (call->TargetName == "probe_device") {
		SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), SSAIntrinsicStatement::SSAIntrinsic_ProbeDevice);
		stmt->SetDiag(call->Diag());
		stmt->AddArg(dev_id);

		return stmt;
	} else if (call->TargetName == "read_device") {
		SSAStatement *addr = call->Args[1]->EmitSSAForm(bldr);
		if (addr->GetType() != IRTypes::UInt32) {
			const auto& dn = addr->GetDiag();
			addr = new SSACastStatement(&bldr.GetBlock(), IRTypes::UInt32, addr);
			addr->SetDiag(dn);
		}

		IRVariableExpression *data = dynamic_cast<IRVariableExpression*>(call->Args[2]);
		if (data == nullptr) throw std::logic_error("Value argument to device read intrinsic was not an IRVariableExpression");

		SSASymbol *target = bldr.GetSymbol(data->Symbol);
		SSADeviceReadStatement *stmt = new SSADeviceReadStatement(&bldr.GetBlock(), dev_id, addr, target);
		stmt->SetDiag(call->Diag());

		return stmt;
	} else if (call->TargetName == "write_device") {
		SSAStatement *addr = call->Args[1]->EmitSSAForm(bldr);
		SSAStatement *data = call->Args[2]->EmitSSAForm(bldr);

		if (addr->GetType() != IRTypes::UInt32) {
			const auto& dn = addr->GetDiag();
			addr = new SSACastStatement(&bldr.GetBlock(), IRTypes::UInt32, addr);
			addr->SetDiag(dn);
		}

		if (data->GetType() != wordtype) {
			const auto& dn = data->GetDiag();
			data = new SSACastStatement(&bldr.GetBlock(), wordtype, data);
			data->SetDiag(dn);
		}

		SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), SSAIntrinsicStatement::SSAIntrinsic_WriteDevice);
		stmt->AddArg(dev_id);
		stmt->AddArg(addr);
		stmt->AddArg(data);

		return stmt;
	} else {
		throw std::logic_error("Unsupported device intrinsic: " + call->TargetName);
	}
}

static ssa::SSAStatement *BitcastIntrinsicEmitter(const IRIntrinsicAction *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr, const IRType &wordtype)
{
	IRType from, to;
	SSACastStatement::CastType type;
	SSACastStatement::CastOption option = SSACastStatement::Option_None;

	if (call->TargetName == "bitcast_u32_float") {
		from = IRTypes::UInt32;
		to = IRTypes::Float;
		type = SSACastStatement::Cast_Reinterpret;
	} else if (call->TargetName == "bitcast_u64_double") {
		from = IRTypes::UInt64;
		to = IRTypes::Double;
		type = SSACastStatement::Cast_Reinterpret;
	} else if (call->TargetName == "bitcast_float_u32") {
		from = IRTypes::Float;
		to = IRTypes::UInt32;
		type = SSACastStatement::Cast_Reinterpret;
	} else if (call->TargetName == "bitcast_double_u64") {
		from = IRTypes::Double;
		to = IRTypes::UInt64;
		type = SSACastStatement::Cast_Reinterpret;
	} else if (call->TargetName == "__builtin_cast_float_to_u32_truncate") {
		from = IRTypes::Float;
		to = IRTypes::UInt32;
		type = SSACastStatement::Cast_Convert;
		option = SSACastStatement::Option_RoundTowardZero;
	} else if (call->TargetName == "__builtin_cast_double_to_u64_truncate") {
		from = IRTypes::Double;
		to = IRTypes::UInt64;
		type = SSACastStatement::Cast_Convert;
		option = SSACastStatement::Option_RoundTowardZero;
	} else if (call->TargetName == "bitcast_u64_v8u8") {
		from = IRTypes::UInt64;
		to = IRTypes::UInt8;
		to.VectorWidth = 8;
		type = SSACastStatement::Cast_Reinterpret;
	} else {
		throw std::logic_error("Unsupported intrinsic bitcast '" + call->TargetName + "'");
	}

	SSAStatement *arg = call->Args[0]->EmitSSAForm(bldr);
	auto stmt = new SSACastStatement(&bldr.GetBlock(), to, arg, type);
	stmt->SetOption(option);
	stmt->SetDiag(call->Diag());

	return stmt;
}

void GenCContext::AddIntrinsic(const std::string& name, const IRType& retty, const IRSignature::param_type_list_t& ptl, IntrinsicEmitterFn emitter, SSAIntrinsicStatement::IntrinsicType type)
{
	if (IntrinsicTable.count(name) != 0) {
		throw std::logic_error("Intrinsic with the name '" + name + "' is already registered.");
	}

	IRSignature sig(name, retty, ptl);
	IntrinsicTable[name] = std::make_pair(new IRIntrinsicAction(sig, *this), emitter);
	__intrinsic_type_mapping[name] = type;
}

static const IRType &WordType(const arch::ArchDescription *arch)
{
	// PC Intrinsics: TODO specialise this
	switch(arch->wordsize) {
		case 32:
			return IRTypes::UInt32;
			break;
		case 64:
			return IRTypes::UInt64;
			break;
		default:
			UNIMPLEMENTED;
	}
}

void GenCContext::LoadIntrinsics()
{
	const IRType &wordtype = WordType(&Arch);
	AddIntrinsic("read_pc", wordtype, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_ReadPc);
	AddIntrinsic("write_pc", IRTypes::Void, {IRParam("addr", wordtype)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_WritePc);

	// Memory Intrinsics: TODO specialise this
	AddIntrinsic("mem_read_8", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt8))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_read_16", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt16))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_read_32", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt32))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_read_64", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt64))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_8", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt8)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_16", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt16)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_32", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt32)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_64", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt64)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);

//	AddIntrinsic("mem_read_8_user", IRTypes::Void, {IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt8))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
//	AddIntrinsic("mem_read_16_user", IRTypes::Void, {IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt16))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
//	AddIntrinsic("mem_read_32_user", IRTypes::Void, {IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt32))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
//	AddIntrinsic("mem_read_64_user", IRTypes::Void, {IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt64))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
//	AddIntrinsic("mem_write_8_user", IRTypes::Void, {IRParam("addr", wordtype), IRParam("value", IRTypes::UInt8)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
//	AddIntrinsic("mem_write_16_user", IRTypes::Void, {IRParam("addr", wordtype), IRParam("value", IRTypes::UInt16)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
//	AddIntrinsic("mem_write_32_user", IRTypes::Void, {IRParam("addr", wordtype), IRParam("value", IRTypes::UInt32)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
//	AddIntrinsic("mem_write_64_user", IRTypes::Void, {IRParam("addr", wordtype), IRParam("value", IRTypes::UInt64)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);

	// Device Intrinsics: TODO specialise this
	AddIntrinsic("probe_device", IRTypes::UInt8, {IRParam("device", IRTypes::UInt32)}, DeviceIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("read_device", IRTypes::UInt8, {IRParam("device", IRTypes::UInt32), IRParam("addr", IRTypes::UInt32), IRParam("data", IRType::Ref(wordtype))}, DeviceIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("write_device", IRTypes::UInt8, {IRParam("device", IRTypes::UInt32), IRParam("addr", IRTypes::UInt32), IRParam("data", wordtype)}, DeviceIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);

	// Trap Intrinsic: TODO make this a control-flow statement
	AddIntrinsic("trap", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("halt_cpu", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_HaltCpu);

	// IRQ/Privilege support
	AddIntrinsic("push_interrupt", IRTypes::Void, {IRParam("level", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_PushInterrupt);
	AddIntrinsic("pop_interrupt", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_PopInterrupt);
	AddIntrinsic("pend_interrupt", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_PendIRQ);

	AddIntrinsic("trigger_irq", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_TriggerIRQ);
	AddIntrinsic("enter_kernel_mode", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_EnterKernelMode);
	AddIntrinsic("enter_user_mode", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_EnterUserMode);
	AddIntrinsic("take_exception", IRTypes::Void, {IRParam("category", IRTypes::UInt32), IRParam("data", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_TakeException);
	AddIntrinsic("set_cpu_mode", IRTypes::Void, {IRParam("mode", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_SetCpuMode);
	AddIntrinsic("get_cpu_mode", IRTypes::UInt8, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode);

	// *** Built-in Helpers *** //
	AddIntrinsic("__builtin_set_feature", IRTypes::Void, {IRParam("feature", IRTypes::UInt32), IRParam("level", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_SetFeature);
	AddIntrinsic("__builtin_get_feature", IRTypes::UInt32, {IRParam("feature", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_GetFeature);

	AddIntrinsic("__builtin_bswap32", IRTypes::UInt32, {IRParam("value", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_BSwap32);
	AddIntrinsic("__builtin_bswap64", IRTypes::UInt64, {IRParam("value", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_BSwap64);

	AddIntrinsic("__builtin_clz", IRTypes::UInt32, {IRParam("value", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Clz32);
	AddIntrinsic("__builtin_clz64", IRTypes::UInt64, {IRParam("value", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Clz64);
	AddIntrinsic("__builtin_popcount", IRTypes::UInt32, {IRParam("value", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Popcount32);

	AddIntrinsic("__builtin_update_zn_flags", IRTypes::Void, {IRParam("value", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_UpdateZN32);
	AddIntrinsic("__builtin_update_zn_flags64", IRTypes::Void, {IRParam("value", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_UpdateZN64);

	// Carry + Flags Optimisation: TODO New GenSim flag-capturing operator stuff
	AddIntrinsic("__builtin_adc", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt32), IRParam("rhs", IRTypes::UInt32), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Adc);
	AddIntrinsic("__builtin_adc_flags", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt32), IRParam("rhs", IRTypes::UInt32), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_AdcWithFlags);
	AddIntrinsic("__builtin_sbc", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt32), IRParam("rhs", IRTypes::UInt32), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Sbc);
	AddIntrinsic("__builtin_sbc_flags", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt32), IRParam("rhs", IRTypes::UInt32), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_SbcWithFlags);
	AddIntrinsic("__builtin_adc64", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Adc64);
	AddIntrinsic("__builtin_adc64_flags", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Adc64WithFlags);
	AddIntrinsic("__builtin_sbc64", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Sbc64);
	AddIntrinsic("__builtin_sbc64_flags", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Sbc64WithFlags);

	// Wide multiplication: TODO support 128-bit types
	AddIntrinsic("__builtin_umull", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_UMULL);
	AddIntrinsic("__builtin_umulh", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_UMULH);
	AddIntrinsic("__builtin_smull", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_SMULL);
	AddIntrinsic("__builtin_smulh", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_SMULH);

	// Floating point helpers
	AddIntrinsic("float_sqrt", IRTypes::Float, {IRParam("value", IRTypes::Float)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_FloatSqrt);
	AddIntrinsic("double_sqrt", IRTypes::Double, {IRParam("value", IRTypes::Double)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_DoubleSqrt);

	AddIntrinsic("__builtin_fabs32", IRTypes::Float, {IRParam("value", IRTypes::Float)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_FloatAbs);
	AddIntrinsic("__builtin_fabs64", IRTypes::Double, {IRParam("value", IRTypes::Double)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_DoubleAbs);

	AddIntrinsic("__builtin_set_fp_flush", IRTypes::Void, {IRParam("mode", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_FPSetFlush);
	AddIntrinsic("__builtin_get_fp_flush", IRTypes::UInt32, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_FPGetFlush);
	AddIntrinsic("__builtin_set_fp_rounding", IRTypes::Void, {IRParam("mode", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_FPSetRounding);
	AddIntrinsic("__builtin_get_fp_rounding", IRTypes::UInt32, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_FPGetRounding);

	AddIntrinsic("__builtin_fma32", IRTypes::Float, {IRParam("a", IRTypes::Float), IRParam("b", IRTypes::Float), IRParam("c", IRTypes::Float)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_FMA32);
	AddIntrinsic("__builtin_fma64", IRTypes::Double, {IRParam("a", IRTypes::Double), IRParam("b", IRTypes::Double), IRParam("c", IRTypes::Double)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_FMA64);

	AddIntrinsic("bitcast_u32_float", IRTypes::Float, {IRParam("value", IRTypes::UInt32)}, BitcastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("bitcast_u64_double", IRTypes::Double, {IRParam("value", IRTypes::UInt64)}, BitcastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("bitcast_float_u32", IRTypes::UInt32, {IRParam("value", IRTypes::Float)}, BitcastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("bitcast_double_u64", IRTypes::UInt64, {IRParam("value", IRTypes::Double)}, BitcastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);

	auto v8u8 = IRTypes::UInt8;
	v8u8.VectorWidth = 8;
	AddIntrinsic("bitcast_u64_v8u8", v8u8, {IRParam("value", IRTypes::UInt64)}, BitcastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);

	AddIntrinsic("__builtin_cast_float_to_u32_truncate", IRTypes::UInt32, {IRParam("value", IRTypes::Float)}, BitcastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("__builtin_cast_double_to_u64_truncate", IRTypes::UInt64, {IRParam("value", IRTypes::Double)}, BitcastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
}

SSAStatement *IRCallExpression::EmitIntrinsicCall(SSABuilder &bldr, const gensim::arch::ArchDescription &Arch) const
{
	auto emitter = this->Target->Context.GetIntrinsicEmitter(Target->GetSignature().GetName());
	if (emitter == nullptr) {
		throw std::logic_error("Intrinsic emitter for '" + Target->GetSignature().GetName() + "' not available");
	}

	return emitter((IRIntrinsicAction *)this->Target, this, bldr, IRType::GetIntType(Arch.wordsize));
}
