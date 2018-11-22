/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMAliasAnalysis.h"
#include "define.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/DataLayout.h>

#include <llvm/IR/Verifier.h>

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/DependenceAnalysis.h>
#include <llvm/Analysis/MemoryDependenceAnalysis.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TypeBasedAliasAnalysis.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/AliasAnalysisEvaluator.h>

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Vectorize.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/PromoteMemToReg.h>

using namespace archsim::translate::translate_llvm;

PointerInformationProvider::PointerInformationProvider(llvm::Function* fn) : function_(fn)
{

}

llvm::Value* PointerInformationProvider::GetMemBase() const
{
	return function_->getParent()->getGlobalVariable("contiguous_mem_base");
}

llvm::Value* PointerInformationProvider::GetRegBase() const
{
	return &*function_->arg_begin();
}

llvm::Value* PointerInformationProvider::GetStateBase() const
{
	return &*(function_->arg_begin()+1);
}

bool TryRecover_RegAccess(const llvm::Value *v, int size, std::vector<int> &aaai)
{
	const llvm::Instruction *insn = llvm::dyn_cast<const llvm::Instruction>(v);

	bool is_reg_access = false;
	uint64_t offset = 0;

	const llvm::Argument *arg = llvm::dyn_cast<const llvm::Argument>(v);
	if(arg != nullptr) {
		const llvm::Value *reg_file_ptr = &*arg->getParent()->arg_begin();
		if(arg == reg_file_ptr) {
			is_reg_access = true;
			offset = 0;
		}
	}

	if(insn != nullptr) {
		const llvm::Value *reg_file_ptr = &*insn->getParent()->getParent()->arg_begin();
		llvm::DataLayout dl = insn->getParent()->getParent()->getParent()->getDataLayout();
		llvm::APInt accumulated_offset(64, 0, false);
		if(v->stripAndAccumulateInBoundsConstantOffsets(dl, accumulated_offset) == reg_file_ptr) {
			// definitely a register access. Now try and figure out offsets + size

			is_reg_access = true;
			offset = accumulated_offset.getZExtValue();
		} else if(v->stripInBoundsOffsets() == reg_file_ptr) {
			UNIMPLEMENTED;
		}
	}

	if(is_reg_access) {
		aaai = {TAG_REG_ACCESS, offset, size};
		return true;
	}
	return false;

	if(const llvm::BitCastInst *bc = llvm::dyn_cast<const llvm::BitCastInst>(v)) {
		const llvm::Value *reg_file_ptr = &*bc->getParent()->getParent()->arg_begin();
		if(bc->getOperand(0) == reg_file_ptr) {
			int addend = 0;
			int size = bc->getType()->getPointerElementType()->getScalarSizeInBits()/8;
			aaai = {TAG_REG_ACCESS, addend, addend + size, size};
			return true;
		} else {
			return false;
		}
	}

	if(const llvm::IntToPtrInst *inst = llvm::dyn_cast<const llvm::IntToPtrInst>(v)) {
		const llvm::Value *reg_file_ptr = &*inst->getParent()->getParent()->arg_begin();

		auto op0 = inst->getOperand(0);

		// either we have an add of a ptr to int and a constant, or we have a ptrtoint (with an addend of 0)
		int addend = 0;
		bool is_constant = true;
		llvm::Instruction *ptrtoint = nullptr;

		const llvm::BinaryOperator *add_inst = llvm::dyn_cast<const llvm::BinaryOperator>(op0);
		if(add_inst != nullptr && add_inst->getOpcode() == llvm::BinaryOperator::Add) {
			auto add_op0 = add_inst->getOperand(0);
			auto add_op1 = add_inst->getOperand(1);

			// add op0 should be a ptrtoint
			if(!llvm::isa<llvm::PtrToIntInst>(add_op0)) {
				return false;
			}

			ptrtoint = (llvm::PtrToIntInst*)add_op0;

			if(const llvm::ConstantInt *constant_val = llvm::dyn_cast<const llvm::ConstantInt>(add_op1)) {
				addend = constant_val->getValue().getZExtValue();
			} else {
				is_constant = false;
			}
		} else {
			ptrtoint = (llvm::Instruction*)op0;
		}
		auto ptrtoint_op = ptrtoint->getOperand(0);

		if(ptrtoint_op == reg_file_ptr) {
			int size = inst->getType()->getPointerElementType()->getScalarSizeInBits() / 8;

			if(is_constant) {
				aaai = {(int)TAG_REG_ACCESS, addend, addend + size, size};
				return true;
			} else {
				aaai = {(int)TAG_REG_ACCESS};
				return true;
			}
		}
	}
	return false;
}

bool PointerInformationProvider::TryRecover_MemAccess(const llvm::Value *v, std::vector<int>& aaai) const
{
	auto mem_base = GetMemBase();

	v = v->stripPointerCasts();

	if(v == mem_base) {
		aaai = {TAG_MEM_ACCESS};
		return true;
	}

	if(llvm::isa<llvm::Instruction>(v)) {
		auto stripped_v = v->stripInBoundsOffsets();

		if(llvm::isa<llvm::LoadInst>(stripped_v) && ((llvm::LoadInst*)stripped_v)->getPointerOperand() == mem_base) {
			aaai = {TAG_MEM_ACCESS};
			return true;
		}
	}

	// a memory access is either:
	// 1. an instruction inttoptr of a value >= 0x80000000
	// 1b. a constantexpr inttoptr of a value >= 0x80000000
	// 2. a gep with a base of >= 0x80000000
	// 3. a pointercast of (2)

	if(llvm::isa<llvm::IntToPtrInst>(v)) {
		llvm::IntToPtrInst *inst = (llvm::IntToPtrInst*)v;
		llvm::ConstantInt *value = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0));
		if(value != nullptr && value->getZExtValue() >= 0x80000000) {
			aaai = {TAG_MEM_ACCESS};
			return true;
		}
	} else if(llvm::isa<llvm::ConstantExpr>(v)) {
		// possibly 1a
		llvm::ConstantExpr *expr = (llvm::ConstantExpr*)v;

		if(expr->getOpcode() == llvm::Instruction::IntToPtr) {
			auto op0 = expr->getOperand(0);
			auto const_op0 = llvm::dyn_cast<llvm::ConstantInt>(op0);
			if(const_op0 != nullptr && const_op0->getZExtValue() >= 0x80000000) {
				aaai = {TAG_MEM_ACCESS};
				return true;
			}
		}

	} else if(llvm::isa<llvm::GetElementPtrInst>(v)) {
		// return recovered data from base of gep
		llvm::GetElementPtrInst *gepinst = (llvm::GetElementPtrInst*)v;
		return TryRecover_MemAccess(gepinst->getOperand(0), aaai);
	} else if(llvm::isa<llvm::CastInst>(v)) {
		// return recovered data from base of bitcast
		llvm::CastInst *inst = (llvm::CastInst*)v;
		return TryRecover_MemAccess(inst->getOperand(0), aaai);
	}

	return false;
}

bool PointerInformationProvider::TryRecover_StateBlock(const llvm::Value *v, std::vector<int> &aaai) const
{
	auto stripped_v = v->stripInBoundsOffsets();

	if(stripped_v == GetStateBase()) {
		aaai = {TAG_CPU_STATE};
		return true;
	}
	return false;

}

bool MergePhiValues(const llvm::PHINode *phi, std::set<llvm::Value*> &values)
{
	for(auto i = 0; i < phi->getNumIncomingValues(); ++i) {
		auto value = phi->getIncomingValue(i);

		if(llvm::isa<llvm::PHINode>(value)) {
			if(!values.count(value)) {
				values.insert(value);
				MergePhiValues((llvm::PHINode*)value, values);
			}
		}


	}

	return true;
}

std::vector<int> MergeAAAI(std::vector<int> &a, std::vector<int> &b)
{
	if(a.empty() || b.empty()) {
		return {};
	}

	if(a.at(0) == b.at(0)) {

		if(a == b) {
			return a;
		} else {
			return {a.at(0)};
		}

	} else {
		return {};
	}
}

bool PointerInformationProvider::TryRecover_FromPhi(const llvm::Value *v, int size, std::vector<int> &aaai) const
{
	if(llvm::isa<llvm::PHINode>(v)) {
		std::set<llvm::Value*> phi_values;
		MergePhiValues((llvm::PHINode*)v, phi_values);

		std::vector<int> base_aaai;
		for(auto i : phi_values) {
			if(!llvm::isa<llvm::PHINode>(i)) {
				GetPointerInfo(i, size, base_aaai);
				break;
			}
		}

		for(auto i : phi_values) {
			if(llvm::isa<llvm::PHINode>(i)) {
				continue;
			}

			GetPointerInfo(i, size, aaai);
			base_aaai = MergeAAAI(base_aaai, aaai);
		}

		aaai = base_aaai;
		return true;
	}

	return false;
}

bool PointerInformationProvider::TryRecover_FromSelect(const llvm::Value *v, int size, std::vector<int> &aaai) const
{
	if(llvm::isa<llvm::SelectInst>(v)) {
		llvm::SelectInst *inst = (llvm::SelectInst*)v;
		std::vector<int> true_aaai, false_aaai;

		if(GetPointerInfo(inst->getTrueValue(), size, true_aaai) && GetPointerInfo(inst->getFalseValue(), size, false_aaai) && true_aaai == false_aaai) {
			aaai = true_aaai;
			return true;
		}

	}
	return false;
}

bool PointerInformationProvider::TryRecover_Chain(const llvm::Value *v, int size, std::vector<int> &output_aaai) const
{
	auto stripped_v = v->stripInBoundsOffsets();

	if(llvm::isa<llvm::LoadInst>(stripped_v)) {
		llvm::Value *v_base = ((llvm::LoadInst*)stripped_v)->getPointerOperand();
		v_base = v_base->stripInBoundsOffsets();

		if(v_base == GetStateBase()) {
			output_aaai = {TAG_CPU_STATE};
			return true;
		}
	}

	return false;
}

bool TryRecover_Dispatch(const llvm::Value *v, int size, std::vector<int> &output_aaai)
{
	auto stripped_v = v->stripInBoundsOffsets();

	// TODO: make this check stronger
	if(stripped_v->getName().str() == "jump_table") {
		return true;
	}

	return false;
}

bool TryRecover_Alloca(const llvm::Value *v, PointerInformationProvider::PointerInfo &pi)
{
	if(llvm::isa<llvm::AllocaInst>(v)) {
		pi = {TAG_LOCAL};
		return true;
	} else {
		return false;
	}
}

bool PointerInformationProvider::GetPointerInfo(llvm::Value* v, std::vector<int>& output_aaai) const
{
	if(!v->getType()->isPointerTy()) {
		UNEXPECTED;
	}

	return GetPointerInfo(v, v->getType()->getPointerElementType()->getPrimitiveSizeInBits()/8, output_aaai);
}
bool PointerInformationProvider::GetPointerInfo(const llvm::Value* v, std::vector<int>& output_aaai) const
{
	if(!v->getType()->isPointerTy()) {
		UNEXPECTED;
	}

	return GetPointerInfo(v, v->getType()->getPointerElementType()->getPrimitiveSizeInBits()/8, output_aaai);
}

void PointerInformationProvider::DecodeMetadata(const llvm::Instruction* i, PointerInfo& pi) const
{
	auto metadata = i->getMetadata("archsim_pip");

	if(metadata == nullptr) {
		UNEXPECTED;
	}

	pi.resize(metadata->getNumOperands());
	for(int i = 0; i < pi.size(); ++i) {
		auto mdoperand = &metadata->getOperand(i);
		auto metadata = mdoperand->get();
		auto mdconstant = ((llvm::ConstantAsMetadata*)metadata)->getValue();
		auto constant = (llvm::ConstantInt*)mdconstant;

		pi.at(i) = constant->getZExtValue();
	}
}

void PointerInformationProvider::EncodeMetadata(llvm::Instruction* insn, const PointerInfo& pi) const
{
	std::vector<llvm::Metadata*> metadata;
	for(int i : pi) {
		metadata.push_back(llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(llvm::IntegerType::getInt64Ty(insn->getContext()), i)));
	}

	auto mdnode = llvm::MDNode::get(insn->getContext(), metadata);
	insn->setMetadata("archsim_pip", mdnode);
}

bool PointerInformationProvider::GetPointerInfo(llvm::Value* v, int size_in_bytes, std::vector<int>& output_aaai) const
{
	if(llvm::isa<llvm::Instruction>(v)) {
		llvm::Instruction *insn = (llvm::Instruction*)v;
		if(insn->getMetadata("archsim_pip") != nullptr) {
			DecodeMetadata(insn, output_aaai);
			return true;
		}
	}

	bool success = true;
	if(TryRecover_FromPhi(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_FromSelect(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_RegAccess(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_MemAccess(v, output_aaai)) {

	} else if(TryRecover_StateBlock(v, output_aaai)) {

	} else if(TryRecover_Chain(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_Dispatch(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_Alloca(v, output_aaai)) {

	} else {
		success = false;
	}

	if(success) {
		if(llvm::isa<llvm::Instruction>(v)) {
			EncodeMetadata((llvm::Instruction*)v, output_aaai);
		}
	}

	return success;
}
bool PointerInformationProvider::GetPointerInfo(const llvm::Value* v, int size_in_bytes, std::vector<int>& output_aaai) const
{
	if(llvm::isa<llvm::Instruction>(v)) {
		llvm::Instruction *insn = (llvm::Instruction*)v;
		if(insn->getMetadata("archsim_pip") != nullptr) {
			DecodeMetadata(insn, output_aaai);
			return true;
		}
	}

	bool success = true;
	if(TryRecover_FromPhi(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_FromSelect(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_RegAccess(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_MemAccess(v, output_aaai)) {

	} else if(TryRecover_StateBlock(v, output_aaai)) {

	} else if(TryRecover_Alloca(v, output_aaai)) {

	} else if(TryRecover_Dispatch(v, size_in_bytes, output_aaai)) {

	} else if(TryRecover_Chain(v, size_in_bytes, output_aaai)) {

	} else {
		success = false;
	}

	return success;
}
