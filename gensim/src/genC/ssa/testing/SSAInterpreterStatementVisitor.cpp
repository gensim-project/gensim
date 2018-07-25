/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/testing/SSAInterpreterStatementVisitor.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "define.h"
#include "arch/ArchDescription.h"

#include "genC/ssa/SSAContext.h"

#include <cmath>

using namespace gensim::genc;
using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::testing;

SSAInterpreterStatementVisitor::SSAInterpreterStatementVisitor(VMActionState& vmstate, MachineStateInterface& machstate, const arch::ArchDescription *arch) : _vmstate(vmstate), _machine_state(machstate), _arch(arch)
{

}

SSAInterpreterStatementVisitor::~SSAInterpreterStatementVisitor()
{

}

void SSAInterpreterStatementVisitor::VisitStatement(SSAStatement& stmt)
{
	UNIMPLEMENTED;
	_vmstate.SetResult(Interpret_Error);
}

void SSAInterpreterStatementVisitor::VisitJumpStatement(SSAJumpStatement& stmt)
{
	_vmstate.SetNextBlock(stmt.Target());
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitRaiseStatement(SSARaiseStatement& stmt)
{
	throw std::logic_error("invoking raise statements in the interpreter is not supported");
}

void SSAInterpreterStatementVisitor::VisitReturnStatement(SSAReturnStatement& stmt)
{
	if(stmt.Value() != nullptr) {
		_vmstate.SetReturnValue(_vmstate.GetStatementValue(stmt.Value()));
	}

	_vmstate.SetNextBlock(nullptr);
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitCastStatement(SSACastStatement& stmt)
{
	// fetch the value of the original expression
	if(stmt.GetType().VectorWidth != 1) {
		UNIMPLEMENTED;
	}

	IRConstant base_value = _vmstate.GetStatementValue(stmt.Expr());

	IRConstant value = IRType::Cast(base_value, stmt.Expr()->GetType(), stmt.GetType());

	_vmstate.SetStatementValue(&stmt, value);
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitConstantStatement(SSAConstantStatement& stmt)
{
	_vmstate.SetStatementValue(&stmt, stmt.Constant);
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitIfStatement(SSAIfStatement& stmt)
{
	IRConstant value = _vmstate.GetStatementValue(stmt.Expr());
	if(value) {
		_vmstate.SetNextBlock(stmt.TrueTarget());
	} else {
		_vmstate.SetNextBlock(stmt.FalseTarget());
	}
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitCallStatement(SSACallStatement& stmt)
{
	SSAFormAction *callee = dynamic_cast<SSAFormAction *>(stmt.Target());
	if (callee == nullptr) {
		throw std::logic_error("Cannot invoke call statement for non-SSAFormAction call");
	}

	SSAInterpreter subinterpreter (_arch, _machine_state);
	subinterpreter.SetTracing(false);

	std::vector<IRConstant> param_values;
	for(unsigned i = 0; i < stmt.ArgCount(); ++i) {
		SSAValue *param = stmt.Arg(i);
		if(dynamic_cast<SSAStatement*>(param)) {
			param_values.push_back(_vmstate.GetStatementValue((SSAStatement*)stmt.Arg(i)));
		} else if(dynamic_cast<SSASymbol*>(param)) {
			param_values.push_back(_vmstate.GetSymbolValue((SSASymbol*)stmt.Arg(i)));
		}
	}

	auto result = subinterpreter.ExecuteAction(callee, param_values);

	// need to get back reference values
	for(unsigned i = 0; i < stmt.ArgCount(); ++i) {
		SSAValue *param = stmt.Arg(i);
		if(dynamic_cast<SSASymbol*>(param)) {
			_vmstate.SetSymbolValue((SSASymbol*)param, result.ReferenceParameterValues.at(i));
		}
	}

	if(result.Result == Interpret_Normal && callee->GetPrototype().ReturnType() != IRTypes::Void) {
		_vmstate.SetStatementValue(&stmt, result.ReturnValue);
	}

	_vmstate.SetResult(result.Result);
}

void SSAInterpreterStatementVisitor::VisitBinaryArithmeticStatement(SSABinaryArithmeticStatement& stmt)
{
	IRConstant lhs = _vmstate.GetStatementValue(stmt.LHS());
	IRConstant rhs = _vmstate.GetStatementValue(stmt.RHS());

	IRConstant result;

	switch(stmt.Type) {
			using namespace BinaryOperator;
		case Add:
			result = lhs + rhs;
			break;
		case Subtract:
			result = lhs - rhs;
			break;
		case Multiply:
			result = lhs * rhs;
			break;
		case Divide: {
			if(rhs == IRConstant::Integer(0)) {
				result = IRConstant::Integer(0);
			} else {
				result = lhs / rhs;
			}
			break;
		}
		case Modulo: {
			if(rhs == IRConstant::Integer(0)) {
				result = IRConstant::Integer(0);
			} else {
				result = lhs % rhs;
			}
			break;
		}

		case Bitwise_And:
			result = lhs & rhs;
			break;
		case Bitwise_Or:
			result = lhs | rhs;
			break;
		case Bitwise_XOR:
			result = lhs ^ rhs;
			break;

		case Logical_And:
			result = lhs && rhs;
			break;
		case Logical_Or:
			result = lhs || rhs;
			break;

		case ShiftLeft:
			result = lhs << rhs;
			break;
		case ShiftRight:
			result = lhs >> rhs;
			break;
		case SignedShiftRight:
			result = IRConstant::SSR(lhs, rhs);
			break;

		case RotateLeft:
			switch(stmt.GetType().SizeInBytes()) {
				case 4:
					result = IRConstant::ROL(lhs, rhs, 32);
					break;
				case 8:
					result = IRConstant::ROL(lhs, rhs, 64);
					break;
				default:
					UNIMPLEMENTED;
			}
		case RotateRight:
			switch(stmt.GetType().SizeInBytes()) {
				case 4:
					result = IRConstant::ROR(lhs, rhs, 32);
					break;
				case 8:
					result = IRConstant::ROR(lhs, rhs, 64);
					break;
				default:
					UNIMPLEMENTED;
			}

		case Equality:
			result = IRConstant::Integer(lhs == rhs);
			break;
		case Inequality:
			result = IRConstant::Integer(lhs != rhs);
			break;
		case GreaterThan:
			result = lhs > rhs;
			break;
		case GreaterThanEqual:
			result = lhs >= rhs;
			break;
		case LessThan:
			result = lhs < rhs;
			break;
		case LessThanEqual:
			result = lhs <= rhs;
			break;

		default:
			throw std::logic_error("Unknown binary expression type");
	}

	// cast result to the correct width
	_vmstate.SetStatementValue(&stmt, result);
	_vmstate.SetResult(Interpret_Normal);
}

gensim::genc::IRConstant ReadElement(gensim::genc::ssa::testing::RegisterFileState &rfile, size_t offset, const gensim::genc::IRType &type)
{
	GASSERT(type.VectorWidth == 1);
	using namespace gensim::genc;
	IRConstant value;

	union {
		uint64_t data_i;
		float data_f;
		double data_d;
	};

	rfile.Read(offset, type.SizeInBytes(), (uint8_t*)&data_i);

	switch(type.BaseType.PlainOldDataType) {
		case IRPlainOldDataType::FLOAT:
			value = IRConstant::Float(data_f);
			break;
		case IRPlainOldDataType::DOUBLE:
			value = IRConstant::Double(data_d);
			break;
		default:
			value = IRConstant::Integer(data_i);
			break;
	}
	return value;
}

IRConstant GetDefault(const IRType &type)
{
	switch(type.BaseType.PlainOldDataType) {
		case IRPlainOldDataType::FLOAT:
			return IRConstant::Float(0);
		case IRPlainOldDataType::DOUBLE:
			return IRConstant::Double(0);
		default:
			return IRConstant::Integer(0);
	}
}

void SSAInterpreterStatementVisitor::VisitRegisterStatement(SSARegisterStatement& stmt)
{
	uint64_t data_offset, data_size, data_stride;
	IRType value_type;
	if(stmt.IsBanked) {
		auto &bank = _arch->GetRegFile().GetBankByIdx(stmt.Bank);
		uint64_t regnum = _vmstate.GetStatementValue(stmt.RegNum()).Int();

		data_offset = bank.GetRegFileOffset() + (regnum * bank.GetRegisterStride());
		data_size = bank.GetElementSize();
		data_stride = bank.GetElementStride();
		value_type = bank.GetRegisterIRType();

	} else {
		auto &slot = _arch->GetRegFile().GetSlotByIdx(stmt.Bank);
		data_offset = slot.GetRegFileOffset();
		data_size = slot.GetWidth();
		data_stride = slot.GetWidth();
		value_type = slot.GetIRType();
	}

	if(stmt.IsRead) {
		try {
			if(value_type.VectorWidth > 1) {
				IRConstant vector = IRConstant::Vector(value_type.VectorWidth, GetDefault(value_type.GetElementType()));
				for(unsigned i = 0; i < value_type.VectorWidth; ++i) {
					vector.VPut(i, ReadElement(_machine_state.RegisterFile(), data_offset, value_type.GetElementType()));
					data_offset += value_type.SizeInBytes();
				}
				_vmstate.SetStatementValue(&stmt, vector);
			} else {
				IRConstant value = ReadElement(_machine_state.RegisterFile(), data_offset, value_type);
				_vmstate.SetStatementValue(&stmt, value);
			}

			_vmstate.SetResult(Interpret_Normal);
		} catch(std::exception &e) {
			_vmstate.SetResult(Interpret_Error);
		}
	} else {
		try {
			if(stmt.Value()->GetType().VectorWidth > 1) {
				for(unsigned i = 0; i < stmt.Value()->GetType().VectorWidth; ++i) {
					uint64_t data = _vmstate.GetStatementValue(stmt.Value()).VGet(i).POD();
					_machine_state.RegisterFile().Write(data_offset, data_size, (uint8_t*)&data);
					data_offset += data_stride;
				}
			} else {
				uint64_t data = _vmstate.GetStatementValue(stmt.Value()).POD();
				_machine_state.RegisterFile().Write(data_offset, data_size, (uint8_t*)&data);
			}
			_vmstate.SetResult(Interpret_Normal);
		} catch(std::exception &e) {
			_vmstate.SetResult(Interpret_Error);
		}
	}
}

void SSAInterpreterStatementVisitor::VisitMemoryReadStatement(SSAMemoryReadStatement& stmt)
{
	uint64_t addr = _vmstate.GetStatementValue(stmt.Addr()).Int();
	uint64_t data;

	_machine_state.Memory().Read(addr, stmt.Width, (uint8_t*)&data);
	_vmstate.SetSymbolValue(stmt.Target(), IRConstant::Integer(data));
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitMemoryWriteStatement(SSAMemoryWriteStatement& stmt)
{
	uint64_t addr = _vmstate.GetStatementValue(stmt.Addr()).Int();
	uint64_t data = _vmstate.GetStatementValue(stmt.Value()).Int();

	_machine_state.Memory().Write(addr, stmt.Width, (uint8_t*)&data);
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitVariableReadStatement(SSAVariableReadStatement& stmt)
{
	_vmstate.SetStatementValue(&stmt, _vmstate.GetSymbolValue(stmt.Target()));
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitVariableWriteStatement(SSAVariableWriteStatement& stmt)
{
	_vmstate.SetSymbolValue(stmt.Target(), _vmstate.GetStatementValue(stmt.Expr()));
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitControlFlowStatement(SSAControlFlowStatement& stmt)
{
	UNIMPLEMENTED;
}

void SSAInterpreterStatementVisitor::VisitDeviceReadStatement(SSADeviceReadStatement& stmt)
{
	_vmstate.SetSymbolValue(stmt.Target(), IRConstant::Integer(0));
	_vmstate.SetStatementValue(&stmt, IRConstant::Integer(0));
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitIntrinsicStatement(SSAIntrinsicStatement& stmt)
{
	_vmstate.SetResult(Interpret_Normal);

	switch(stmt.Type) {
		case SSAIntrinsicStatement::SSAIntrinsic_GetFeature: // TODO: Implement properly
		case SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode: // TODO: Implement properly
		case SSAIntrinsicStatement::SSAIntrinsic_ReadPc: // TODO: Implement properly
		case SSAIntrinsicStatement::SSAIntrinsic_FPGetFlush: // TODO: Implement properly
		case SSAIntrinsicStatement::SSAIntrinsic_FPGetRounding: // TODO: Implement properly
		case SSAIntrinsicStatement::SSAIntrinsic_FPSetFlush: // TODO: Implement properly
		case SSAIntrinsicStatement::SSAIntrinsic_FPSetRounding: // TODO: Implement properly
		case SSAIntrinsicStatement::SSAIntrinsic_ProbeDevice: // TODO: Implement properly
			_vmstate.SetStatementValue(&stmt, IRConstant::Integer(0));
			break;

		case SSAIntrinsicStatement::SSAIntrinsic_SetCpuMode:
		case SSAIntrinsicStatement::SSAIntrinsic_TakeException:
		case SSAIntrinsicStatement::SSAIntrinsic_PendIRQ:
		case SSAIntrinsicStatement::SSAIntrinsic_Trap:
		case SSAIntrinsicStatement::SSAIntrinsic_WriteDevice:
		case SSAIntrinsicStatement::SSAIntrinsic_PopInterrupt:
		case SSAIntrinsicStatement::SSAIntrinsic_PushInterrupt:
		case SSAIntrinsicStatement::SSAIntrinsic_EnterKernelMode:
		case SSAIntrinsicStatement::SSAIntrinsic_EnterUserMode:
		case SSAIntrinsicStatement::SSAIntrinsic_SetFeature: // todo: implement
			// do nothing?
			break;

		case SSAIntrinsicStatement::SSAIntrinsic_Popcount32: {
			uint32_t value = _vmstate.GetStatementValue(stmt.Args(0)).Int();
			value = __builtin_popcount(value);
			_vmstate.SetStatementValue(&stmt, IRConstant::Integer(value));
			break;
		}
		case SSAIntrinsicStatement::SSAIntrinsic_DoubleSqrt: {
			double value = _vmstate.GetStatementValue(stmt.Args(0)).Dbl();
			value = std::sqrt(value);
			_vmstate.SetStatementValue(&stmt, IRConstant::Double(value));
			break;
		}
		case SSAIntrinsicStatement::SSAIntrinsic_FloatSqrt: {
			float value = _vmstate.GetStatementValue(stmt.Args(0)).Flt();
			value = std::sqrt(value);
			_vmstate.SetStatementValue(&stmt, IRConstant::Float(value));
			break;
		}

		case SSAIntrinsicStatement::SSAIntrinsic_Clz32: {
			uint32_t input = _vmstate.GetStatementValue(stmt.Args(0)).Int();
			uint32_t result = __builtin_clz(input);
			_vmstate.SetStatementValue(&stmt, IRConstant::Integer(result));
			break;
		}

		case SSAIntrinsicStatement::SSAIntrinsic_HaltCpu:
			_vmstate.SetResult(Interpret_Halt);
			break;
		default: {
			std::ostringstream intrinsic;
			stmt.PrettyPrint(intrinsic);

			throw std::logic_error("Intrinsic not handled in interpreter: " + intrinsic.str());
		}

	}
}

void SSAInterpreterStatementVisitor::VisitPhiStatement(SSAPhiStatement& stmt)
{
	// exactly one phi statement member should be defined
	IRConstant value;
	bool found = false;
	uint64_t found_timestamp = 0;
	for(auto i : stmt.Get()) {
		if(_vmstate.HasStatementValue((SSAStatement*)i)) {
			if(!found || _vmstate.GetStatementTimestamp((SSAStatement*)i) > found_timestamp) {
				found = true;
				value = _vmstate.GetStatementValue((SSAStatement*)i);
				found_timestamp = _vmstate.GetStatementTimestamp((SSAStatement*)i);
			}
		}
	}
	GASSERT(found);

	_vmstate.SetStatementValue(&stmt, value);
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitReadStructMemberStatement(SSAReadStructMemberStatement& stmt)
{
	_vmstate.SetStatementValue(&stmt, _machine_state.Instruction().GetField(stmt.MemberName));
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitSelectStatement(SSASelectStatement& stmt)
{
	IRConstant expr = _vmstate.GetStatementValue(stmt.Cond());
	IRConstant result;

	if(expr) {
		result = _vmstate.GetStatementValue(stmt.TrueVal());
	} else {
		result = _vmstate.GetStatementValue(stmt.FalseVal());
	}

	_vmstate.SetStatementValue(&stmt, result);
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitSwitchStatement(SSASwitchStatement& stmt)
{
	SSABlock *next_block = stmt.Default();
	uint64_t expr = _vmstate.GetStatementValue(stmt.Expr()).Int();

	for(auto &value : stmt.GetValues()) {
		SSAConstantStatement *constant = dynamic_cast<SSAConstantStatement*>(value.first);
		if(constant != nullptr) {
			if(constant->Constant.Int() == expr) {
				next_block = value.second;
			}
		} else {
			throw std::logic_error("");
		}
	}
	_vmstate.SetNextBlock(next_block);
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitUnaryArithmeticStatement(SSAUnaryArithmeticStatement& stmt)
{
	IRConstant input = _vmstate.GetStatementValue(stmt.Expr());
	IRConstant output = -input;

	_vmstate.SetStatementValue(&stmt, output);
	_vmstate.SetResult(Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitVariableKillStatement(SSAVariableKillStatement& stmt)
{
	UNIMPLEMENTED;
}

void SSAInterpreterStatementVisitor::VisitVectorExtractElementStatement(SSAVectorExtractElementStatement& stmt)
{
	auto base = _vmstate.GetStatementValue(stmt.Base());
	auto index = _vmstate.GetStatementValue(stmt.Index());
	auto value = base.VGet(index.Int() % base.VSize());
	_vmstate.SetStatementValue(&stmt, value);
	_vmstate.SetResult(InterpretResult::Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitVectorInsertElementStatement(SSAVectorInsertElementStatement& stmt)
{
	IRConstant vector = _vmstate.GetStatementValue(stmt.Base());
	vector.VPut(_vmstate.GetStatementValue(stmt.Index()).Int() % vector.VSize(), _vmstate.GetStatementValue(stmt.Value()));
	_vmstate.SetStatementValue(&stmt, vector);

	_vmstate.SetResult(InterpretResult::Interpret_Normal);
}

void SSAInterpreterStatementVisitor::VisitBitDepositStatement(SSABitDepositStatement& stmt)
{
	UNIMPLEMENTED;
}

void SSAInterpreterStatementVisitor::VisitBitExtractStatement(SSABitExtractStatement& stmt)
{
	UNIMPLEMENTED;
}
