/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"

using namespace gensim::genc;

IRSignature::IRSignature(const std::string& name, const IRType& return_type, const param_type_list_t& params)
	: name_(name),
	  return_type_(return_type),
	  parameters_(params)
{

}

IRAction::IRAction(GenCContext& context)
	: body(nullptr), Context(context), _scope(nullptr), emitted_ssa_(nullptr)
{
}

IRScope *IRAction::GetFunctionScope()
{
	if (_scope != NULL) return _scope;

	_scope = &IRScope::CreateFunctionScope(*this);
	return _scope;
}

IRCallableAction::IRCallableAction(GenCContext& context)
	: IRAction(context)
{
}

IRExecuteAction::IRExecuteAction(const std::string& name, GenCContext &context, const IRType instructionType)
	: IRAction(context), signature_(IRSignature(name, IRTypes::Void,
{
	IRParam("inst", instructionType)
}))
{
	GetFunctionScope()->InsertSymbol("inst", instructionType, Symbol_Parameter);
}

IRHelperAction::IRHelperAction(const IRSignature& signature, HelperScope scope, GenCContext &context)
	: IRCallableAction(context), _scope(scope), signature_(signature)
{
	for (const auto p : signature.GetParams()) {
		GetFunctionScope()->InsertSymbol(p.GetName(), p.GetType(), Symbol_Parameter);
	}
}

IRIntrinsicAction::IRIntrinsicAction(const std::string& name, IntrinsicID id, SignatureFactory factory, SSAEmitter ssaEmitter, FixednessResolver fixednessResolver, GenCContext& context)
	: IRCallableAction(context), name_(name), id_(id), factory_(factory), emitter_(ssaEmitter), resolver_(fixednessResolver)
{
}
