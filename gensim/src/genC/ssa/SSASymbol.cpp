#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/statement/SSADeviceReadStatement.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"

#include <list>
#include <typeinfo>
#include <iostream>

using namespace gensim;
using namespace gensim::genc;
using namespace gensim::genc::ssa;

SSASymbol::SSASymbol(SSAContext& context, std::string prettyname, const SSAType& type, SymbolType stype, SSASymbol* referenceTo)
	: SSAValue(context, context.GetValueNamespace()),
	  SType(stype),
	  Uses(_uses),
	  pretty_name_(prettyname),
	  ReferenceTo(referenceTo),
	  Type(type)
{
	std::string formatted_prettyname = pretty_name_;
	for(auto &c : formatted_prettyname) {
		if(c == ' ' ) c = '_';
	}
	name_ = "sym_" + std::to_string(GetValueName()) + "_" + std::to_string(SType) + "_" + formatted_prettyname;

	if(ReferenceTo != nullptr) {
		ReferenceTo->AddUse(this);
	}
}

std::string SSASymbol::GetName() const
{
	return name_;
}

void SSASymbol::SetName(const std::string& newname)
{
	name_ = newname;
}


bool SSASymbol::IsFullyFixed() const
{
	for (std::list<SSAVariableKillStatement *>::const_iterator ci = _defs.begin(); ci != _defs.end(); ci++) {
		if (!(*ci)->IsFixed()) {
			return false;
		}
		if ((*ci)->Parent->IsFixed() != BLOCK_ALWAYS_CONST) {
			return false;
		}
	}
	return true;
}

void SSASymbol::Unlink()
{
	if(ReferenceTo != nullptr) {
		ReferenceTo->RemoveUse(this);
	}
}

std::string SSASymbol::ToString() const
{
	return GetPrettyName();
}
