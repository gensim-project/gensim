/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSAValue.h"
#include "genC/ssa/metadata/SSAMetadata.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableWriteStatement.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <sstream>
#include <typeinfo>

using namespace gensim::genc::ssa;

SSAValue::SSAValue(SSAContext& context, SSAValueNamespace &ns) : _context(context), name_(ns.GetName())
{

}

SSAValue::~SSAValue()
{
	if (!IsDisposed()) {
		assert(false && "Object must be disposed before it is deleted!");
	}

	for (auto md : _metadata) {
		delete md;
	}

	_metadata.clear();
}

std::string SSAValue::GetName() const
{
	return "v_" + std::to_string(GetValueName());
}

void SSAValue::AddUse(SSAValue* user)
{
	CheckDisposal();
	_uses.push_back(user);
}

const SSAValue::use_list_t& SSAValue::GetUses() const
{
	CheckDisposal();
	return _uses;
}

void SSAValue::RemoveUse(SSAValue* user)
{
	CheckDisposal();

	for (auto i = _uses.begin(); i != _uses.end(); ++i) {
		if (*i == user) {
			_uses.erase(i);
			return;
		}
	}

	throw std::logic_error("Tried to remove a user which wasn't a user");
}

bool SSAValue::HasDynamicUses() const
{
	for (const auto use : GetUses()) {
		if (auto rd = dynamic_cast<const SSAVariableReadStatement *>(use)) {
			if (rd->Parent->IsFixed() != BLOCK_ALWAYS_CONST) {
				return true;
			}
		} else if (auto wr = dynamic_cast<const SSAVariableKillStatement *>(use)) {
			if (wr->Parent->IsFixed() != BLOCK_ALWAYS_CONST) {
				return true;
			}
		}
	}
	
	return false;
}


void SSAValue::AddMetadata(SSAMetadata* metadata)
{
	if (metadata->_owner != nullptr)
		throw std::logic_error("Metadata node already owned by SSA value");

	if (metadata->_owner == this)
		throw std::logic_error("Metadata node already owned by this SSA value");

	metadata->_owner = this;
	_metadata.push_back(metadata);
}

void SSAValue::RemoveMetadata(SSAMetadata* metadata)
{
	if (metadata->_owner != this)
		throw std::logic_error("Metadata node not owned by this SSA value");

	bool erased = false;
	for (auto MS = _metadata.begin(), ME = _metadata.end(); MS != ME; MS++) {
		if (*MS == metadata) {
			_metadata.erase(MS);
			erased = true;
			break;
		}
	}

	if (!erased) {
		throw std::logic_error("The metadata was not found in the list (which is weird, because apparently this object owns it)");
	}

	metadata->_owner = nullptr;
}

const SSAValue::metadata_list_t& SSAValue::GetMetadata() const
{
	CheckDisposal();
	return _metadata;
}

void SSAValue::PerformDispose()
{
	if (!_uses.empty()) {
		std::stringstream refstr;

		refstr << "Attempted to dispose of " << std::string(typeid(*this).name()) << " with references:" << std::endl;
		for (const auto& use : _uses) {
			refstr << use->ToString() << std::endl;
		}

		throw std::logic_error(refstr.str().c_str());
	}

	Unlink();
}

void SSAValue::Dump() const
{
	Dump(std::cerr);
}

void SSAValue::Dump(std::ostream& output) const
{
	output << ToString() << std::endl;
}


std::string SSAValue::ToString() const
{
	return "<SSA VALUE>";
}
