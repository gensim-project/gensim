/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

static ssa::SSAStatement *DefaultIntrinsicEmitter(const IntrinsicDescriptor *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr)
{
	std::vector<SSAStatement *> arg_statements;

	auto signature = intrinsic->GetSignature(call);

	auto params = signature.GetParams();
	if (params.size() != call->Args.size()) throw std::logic_error("Argument/Parameter count mismatch when emitting intrinsic '" + intrinsic->GetName() + "'");

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

	SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), *intrinsic, signature.GetType());
	stmt->SetDiag(call->Diag());

	for (const auto& arg : arg_statements) {
		stmt->AddArg(arg);
	}

	return stmt;
}

static ssa::SSAStatement *MemoryIntrinsicEmitter(const IntrinsicDescriptor *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr)
{
	const IRType &ArchWordType = WordType(&intrinsic->GetArch());

	bool is_memory_write;
	uint8_t data_width;
	IRType data_type;

	switch (intrinsic->GetID()) {
		case IntrinsicID::MR8:
			is_memory_write = false;
			data_width = 1;
			data_type = IRTypes::UInt8;
			break;
		case IntrinsicID::MR16:
			is_memory_write = false;
			data_width = 2;
			data_type = IRTypes::UInt16;
			break;
		case IntrinsicID::MR32:
			is_memory_write = false;
			data_width = 4;
			data_type = IRTypes::UInt32;
			break;
		case IntrinsicID::MR64:
			is_memory_write = false;
			data_width = 8;
			data_type = IRTypes::UInt64;
			break;
		case IntrinsicID::MR128:
			is_memory_write = false;
			data_width = 16;
			data_type = IRTypes::UInt128;
			break;
		case IntrinsicID::MW8:
			is_memory_write = true;
			data_width = 1;
			data_type = IRTypes::UInt8;
			break;
		case IntrinsicID::MW16:
			is_memory_write = true;
			data_width = 2;
			data_type = IRTypes::UInt16;
			break;
		case IntrinsicID::MW32:
			is_memory_write = true;
			data_width = 4;
			data_type = IRTypes::UInt32;
			break;
		case IntrinsicID::MW64:
			is_memory_write = true;
			data_width = 8;
			data_type = IRTypes::UInt64;
			break;
		case IntrinsicID::MW128:
			is_memory_write = true;
			data_width = 16;
			data_type = IRTypes::UInt128;
			break;

		default:
			UNEXPECTED;
	}

	auto mem_interface_read = dynamic_cast<IRVariableExpression*>(call->Args[0]);
	if(mem_interface_read == nullptr) {
		throw std::logic_error("First argument of mem intrinsic must be a constant memory interface ID");
	}

	auto mem_interface_id = call->GetScope().GetContainingAction().Context.GetConstant(mem_interface_read->Symbol->GetLocalName()).second.Int();

	SSAStatement *addr = call->Args[1]->EmitSSAForm(bldr);
	if (addr->GetType() != ArchWordType) {
		const auto& dn = addr->GetDiag();
		addr = new SSACastStatement(&bldr.GetBlock(), ArchWordType, addr);
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

static ssa::SSAStatement *EmitReadDevice(const IntrinsicDescriptor *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr, const IRType &datatype)
{
	SSAStatement *dev_id = call->Args[0]->EmitSSAForm(bldr);
	SSAStatement *addr = call->Args[1]->EmitSSAForm(bldr);

	if (dev_id->GetType() != IRTypes::UInt32) {
		const auto& dn = dev_id->GetDiag();
		dev_id = new SSACastStatement(&bldr.GetBlock(), IRTypes::UInt32, dev_id);
		dev_id->SetDiag(dn);
	}

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
}

static ssa::SSAStatement *EmitWriteDevice(const IntrinsicDescriptor *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr, const IRType &datatype)
{
	SSAStatement *dev_id = call->Args[0]->EmitSSAForm(bldr);
	SSAStatement *addr = call->Args[1]->EmitSSAForm(bldr);
	SSAStatement *data = call->Args[2]->EmitSSAForm(bldr);

	if (dev_id->GetType() != IRTypes::UInt32) {
		const auto& dn = dev_id->GetDiag();
		dev_id = new SSACastStatement(&bldr.GetBlock(), IRTypes::UInt32, dev_id);
		dev_id->SetDiag(dn);
	}

	if (addr->GetType() != IRTypes::UInt32) {
		const auto& dn = addr->GetDiag();
		addr = new SSACastStatement(&bldr.GetBlock(), IRTypes::UInt32, addr);
		addr->SetDiag(dn);
	}

	if (data->GetType() != datatype) {
		const auto& dn = data->GetDiag();
		data = new SSACastStatement(&bldr.GetBlock(), datatype, data);
		data->SetDiag(dn);
	}

	SSAIntrinsicStatement *stmt = new SSAIntrinsicStatement(&bldr.GetBlock(), *intrinsic, intrinsic->GetSignature(call).GetType());
	stmt->AddArg(dev_id);
	stmt->AddArg(addr);
	stmt->AddArg(data);

	return stmt;
}

static ssa::SSAStatement *DeviceIntrinsicEmitter(const IntrinsicDescriptor *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr)
{
	switch (intrinsic->GetID()) {
		case IntrinsicID::ReadDevice32:
			return EmitReadDevice(intrinsic, call, bldr, IRTypes::UInt32);
		case IntrinsicID::ReadDevice64:
			return EmitReadDevice(intrinsic, call, bldr, IRTypes::UInt64);
		case IntrinsicID::WriteDevice32:
			return EmitWriteDevice(intrinsic, call, bldr, IRTypes::UInt32);
		case IntrinsicID::WriteDevice64:
			return EmitWriteDevice(intrinsic, call, bldr, IRTypes::UInt64);
		default:
			UNEXPECTED;
	}
}

static ssa::SSAStatement *CastIntrinsicEmitter(const IntrinsicDescriptor *intrinsic, const IRCallExpression *call, ssa::SSABuilder &bldr)
{
	IRType from, to;
	SSACastStatement::CastType type;
	SSACastStatement::CastOption option = SSACastStatement::Option_None;

	switch (intrinsic->GetID()) {
		case IntrinsicID::CastFloatToU32Truncate:
			from = IRTypes::Float;
			to = IRTypes::UInt32;
			type = SSACastStatement::Cast_Convert;
			option = SSACastStatement::Option_RoundTowardZero;
			break;

		case IntrinsicID::CastDoubleToU64Truncate:
			from = IRTypes::Double;
			to = IRTypes::UInt64;
			type = SSACastStatement::Cast_Convert;
			option = SSACastStatement::Option_RoundTowardZero;
			break;

		default:
			throw std::logic_error("Unsupported intrinsic cast '" + intrinsic->GetName() + "'");
	}

	SSAStatement *arg = call->Args[0]->EmitSSAForm(bldr);
	auto stmt = new SSACastStatement(&bldr.GetBlock(), to, arg, type);
	stmt->SetOption(option);
	stmt->SetDiag(call->Diag());

	return stmt;
}

static IntrinsicDescriptor::SignatureFactory Signature(IRType returnType, std::initializer_list<IRType> paramTypes = {})
{
	std::vector<IRParam> params;
	for (const auto& paramType : paramTypes) {
		params.push_back(IRParam("__param", paramType));
	}

	return [returnType, params](const IntrinsicDescriptor *intrinsic, const IRCallExpression *call) {
		return IRSignature(intrinsic->GetName(), returnType, params);
	};
}

static bool NeverFixed(const SSAIntrinsicStatement *intrinsicStmt, const IntrinsicDescriptor *intrinsic)
{
	return false;
}

static bool AlwaysFixed(const SSAIntrinsicStatement *intrinsicStmt, const IntrinsicDescriptor *intrinsic)
{
	return true;
}

static bool AutoFixedness(const SSAIntrinsicStatement *intrinsicStmt, const IntrinsicDescriptor *intrinsic)
{
	bool fixed = true;
	for (int i = 0; i < intrinsicStmt->ArgCount(); i++) {
		fixed &= intrinsicStmt->Args(i)->IsFixed();
	}

	return fixed;
}

IntrinsicDescriptor::IntrinsicDescriptor(const std::string &name, IntrinsicID id, const gensim::arch::ArchDescription &arch, const SignatureFactory &factory, const SSAEmitter &emitter, const FixednessResolver &resolver)
	: name_(name), id_(id), arch_(arch), resolver_(resolver), sig_fact_(factory), emitter_(emitter) 
{
	
}

bool IntrinsicManager::AddIntrinsic(const std::string& name, IntrinsicID id, const IntrinsicDescriptor::SignatureFactory &signatureFactory, const IntrinsicDescriptor::SSAEmitter &emitter, const IntrinsicDescriptor::FixednessResolver &fixednessResolver)
{
	if (name_to_descriptor_.count(name) != 0) {
		throw std::logic_error("Intrinsic with the name '" + name + "' is already registered by name.");
	}

	if (id_to_descriptor_.count(id) != 0) {
		throw std::logic_error("Intrinsic with the name '" + name + "' is already registered by id.");
	}


	IntrinsicDescriptor *descriptor = new IntrinsicDescriptor(name, id, GetArch(), signatureFactory, emitter, fixednessResolver);

	name_to_descriptor_[name] = descriptor;
	id_to_descriptor_[id] = descriptor;

	intrinsics_.push_back(descriptor);

	return true;
}

bool IntrinsicManager::LoadIntrinsics() 
{
	const IRType &ArchWordType = WordType(&GetArch());

#define Intrinsic(name, id, sig, emitter, fixedness) AddIntrinsic(name, IntrinsicID::id, sig, emitter, fixedness);
#include "genC/Intrinsics.def"
#undef Intrinsic

	return true;

	/*const IRType &wordtype = WordType(&Arch);
	AddIntrinsic("read_pc", wordtype, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_ReadPc);
	AddIntrinsic("write_pc", IRTypes::Void, {IRParam("addr", wordtype)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_WritePc);

	// Memory Intrinsics: TODO specialise this
	AddIntrinsic("mem_read_8", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt8))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_read_16", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt16))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_read_32", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt32))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_read_64", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt64))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_read_128", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRType::Ref(IRTypes::UInt128))}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_8", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt8)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_16", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt16)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_32", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt32)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_64", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt64)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("mem_write_128", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("addr", wordtype), IRParam("value", IRTypes::UInt128)}, MemoryIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);

	AddIntrinsic("mem_lock", IRTypes::Void, {IRParam("interface", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_MemLock);
	AddIntrinsic("mem_unlock", IRTypes::Void, {IRParam("interface", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_MemUnlock);

	AddIntrinsic("mem_monitor_acquire", IRTypes::Void, {IRParam("interface", IRTypes::UInt32), IRParam("address", wordtype)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_MemMonitorAcquire);
	AddIntrinsic("mem_monitor_write_8", IRTypes::UInt8, {IRParam("interface", IRTypes::UInt32), IRParam("address", wordtype), IRParam("value", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_MemMonitorWrite8);
	AddIntrinsic("mem_monitor_write_16", IRTypes::UInt8, {IRParam("interface", IRTypes::UInt32), IRParam("address", wordtype), IRParam("value", IRTypes::UInt16)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_MemMonitorWrite16);
	AddIntrinsic("mem_monitor_write_32", IRTypes::UInt8, {IRParam("interface", IRTypes::UInt32), IRParam("address", wordtype), IRParam("value", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_MemMonitorWrite32);
	AddIntrinsic("mem_monitor_write_64", IRTypes::UInt8, {IRParam("interface", IRTypes::UInt32), IRParam("address", wordtype), IRParam("value", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_MemMonitorWrite64);

	// Device Intrinsics: TODO specialise this
	AddIntrinsic("probe_device", IRTypes::UInt8, {IRParam("device", IRTypes::UInt32)}, DeviceIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("read_device", IRTypes::UInt8, {IRParam("device", IRTypes::UInt32), IRParam("addr", IRTypes::UInt32), IRParam("data", IRType::Ref(wordtype))}, DeviceIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("write_device", IRTypes::UInt8, {IRParam("device", IRTypes::UInt32), IRParam("addr", IRTypes::UInt32), IRParam("data", wordtype)}, DeviceIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("read_device64", IRTypes::UInt8, {IRParam("device", IRTypes::UInt32), IRParam("addr", IRTypes::UInt32), IRParam("data", IRType::Ref(IRTypes::UInt64))}, DeviceIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("write_device64", IRTypes::UInt8, {IRParam("device", IRTypes::UInt32), IRParam("addr", IRTypes::UInt32), IRParam("data", IRTypes::UInt64)}, DeviceIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);

	// Trap Intrinsic: TODO make this a control-flow statement
	AddIntrinsic("trap", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("halt_cpu", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_HaltCpu);

	// IRQ/Privilege support
	AddIntrinsic("push_interrupt", IRTypes::Void, {IRParam("level", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_PushInterrupt);
	AddIntrinsic("pop_interrupt", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_PopInterrupt);
	AddIntrinsic("pend_interrupt", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_PendIRQ);

	AddIntrinsic("trigger_irq", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_TriggerIRQ);

	// Todo: Deprecate enter_kernel_mode and enter_user_mode
	AddIntrinsic("enter_kernel_mode", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_EnterKernelMode);
	AddIntrinsic("enter_user_mode", IRTypes::Void, {}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_EnterUserMode);
	AddIntrinsic("__builtin_set_execution_ring", IRTypes::Void, {IRParam("ring", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_SetExecutionRing);
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
	AddIntrinsic("__builtin_ctz32", IRTypes::UInt32, {IRParam("value", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Ctz32);
	AddIntrinsic("__builtin_ctz64", IRTypes::UInt64, {IRParam("value", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Ctz64);
	AddIntrinsic("__builtin_popcount", IRTypes::UInt32, {IRParam("value", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Popcount32);

	AddIntrinsic("__builtin_update_zn_flags", IRTypes::Void, {IRParam("value", IRTypes::UInt32)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_UpdateZN32);
	AddIntrinsic("__builtin_update_zn_flags64", IRTypes::Void, {IRParam("value", IRTypes::UInt64)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_UpdateZN64);

	// Carry + Flags Optimisation: TODO New GenSim flag-capturing operator stuff
	AddIntrinsic("__builtin_adc", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt32), IRParam("rhs", IRTypes::UInt32), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Adc);
	AddIntrinsic("__builtin_adc8_flags", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt8), IRParam("rhs", IRTypes::UInt8), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Adc8WithFlags);
	AddIntrinsic("__builtin_adc16_flags", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt16), IRParam("rhs", IRTypes::UInt16), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Adc16WithFlags);
	AddIntrinsic("__builtin_adc_flags", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt32), IRParam("rhs", IRTypes::UInt32), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_AdcWithFlags);
	AddIntrinsic("__builtin_sbc", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt32), IRParam("rhs", IRTypes::UInt32), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Sbc);
	AddIntrinsic("__builtin_sbc_flags", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt32), IRParam("rhs", IRTypes::UInt32), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_SbcWithFlags);
	AddIntrinsic("__builtin_sbc8_flags", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt8), IRParam("rhs", IRTypes::UInt8), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Sbc8WithFlags);
	AddIntrinsic("__builtin_sbc16_flags", IRTypes::UInt32, {IRParam("lhs", IRTypes::UInt16), IRParam("rhs", IRTypes::UInt16), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Sbc16WithFlags);
	AddIntrinsic("__builtin_sbc64_flags", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Sbc64WithFlags);
	AddIntrinsic("__builtin_adc64", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Adc64);
	AddIntrinsic("__builtin_adc64_flags", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Adc64WithFlags);
	AddIntrinsic("__builtin_sbc64", IRTypes::UInt64, {IRParam("lhs", IRTypes::UInt64), IRParam("rhs", IRTypes::UInt64), IRParam("carry_in", IRTypes::UInt8)}, DefaultIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Sbc64);

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

	AddIntrinsic("__builtin_cast_float_to_u32_truncate", IRTypes::UInt32, {IRParam("value", IRTypes::Float)}, CastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);
	AddIntrinsic("__builtin_cast_double_to_u64_truncate", IRTypes::UInt64, {IRParam("value", IRTypes::Double)}, CastIntrinsicEmitter, SSAIntrinsicStatement::SSAIntrinsic_Trap);*/
}

SSAStatement *IRCallExpression::EmitIntrinsicCall(SSABuilder &bldr, const gensim::arch::ArchDescription &Arch) const
{
	IRIntrinsicAction *action = dynamic_cast<IRIntrinsicAction *>(Target);
	if (action == nullptr) {
		UNEXPECTED;
	}

	return action->Emit(this, bldr);
}
