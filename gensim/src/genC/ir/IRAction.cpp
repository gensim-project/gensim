#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"

using namespace gensim::genc;

IRSignature::IRSignature(const std::string& name, const IRType& return_type, const param_type_list_t& params)
	: name_(name),
	  return_type_(return_type),
	  parameters_(params)
{

}

IRAction::IRAction(const IRSignature& signature, GenCContext& context)
	: _signature(signature), body(nullptr), Context(context), _scope(nullptr), emitted_ssa_(nullptr)
{
	for (const auto p : GetSignature().GetParams()) {
		IRType type = p.GetType();
		_params.push_back(GetFunctionScope()->InsertSymbol(p.GetName(), type, Symbol_Parameter));
	}
}

IRScope *IRAction::GetFunctionScope()
{
	if (_scope != NULL) return _scope;

	_scope = &IRScope::CreateFunctionScope(*this);
	return _scope;
}

IRCallableAction::IRCallableAction(const IRSignature& signature, GenCContext& context)
	: IRAction(signature, context)
{
}

IRExecuteAction::IRExecuteAction(const std::string& name, GenCContext &context, const IRType instructionType)
	: IRAction(IRSignature(name, IRTypes::Void,
{
	IRParam("inst", instructionType)
}), context)
{
	InstructionSymbol = _params.front();
}

IRHelperAction::IRHelperAction(const IRSignature& signature, HelperScope scope, GenCContext &context)
	: IRCallableAction(signature, context), _scope(scope)
{
}

IRIntrinsicAction::IRIntrinsicAction(const IRSignature& signature, GenCContext& context)
	: IRCallableAction(signature, context)
{
}

IRExternalAction::IRExternalAction(const IRSignature& signature, GenCContext& context)
	: IRCallableAction(signature, context)
{
}

