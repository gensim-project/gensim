/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMOptimiser.h"
#include "translate/llvm/LLVMAliasAnalysis.h"

#include "util/SimOptions.h"
#include "util/LogContext.h"

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

#include "translate/llvm/LLVMRegisterOptimisationPass.h"

UseLogContext(LogTranslate);
DeclareChildLogContext(LogAliasAnalysis, LogTranslate, "AliasAnalysis");

#define PERMISSIVE

#define CONSTVAL(a) (llvm::mdconst::extract<llvm::ConstantInt>(a)->getZExtValue())
#define ISCONSTVAL(a) (a->getValueID() == ::llvm::Instruction::ConstantIntVal)

using namespace archsim::translate::translate_llvm;


static bool IsInstr(const ::llvm::Value *v)
{
	return (v->getValueID() >= ::llvm::Value::InstructionVal);
}

static bool IsIntToPtr(const ::llvm::Value *v)
{
	return (v->getValueID() == ::llvm::Value::InstructionVal + ::llvm::Instruction::IntToPtr);
}

static bool IsAlloca(const ::llvm::Value *v)
{
	return (v->getValueID() == ::llvm::Value::InstructionVal + ::llvm::Instruction::Alloca);
}

static bool IsGEP(const ::llvm::Value *v)
{
	return (v->getValueID() == ::llvm::Value::InstructionVal + ::llvm::Instruction::GetElementPtr);
}

static bool IsConstExpr(const ::llvm::Value *v)
{
	return (v->getValueID() == ::llvm::Value::ConstantExprVal);
}

static bool IsConstVal(const ::llvm::MDOperand &v)
{
	return llvm::mdconst::hasa<llvm::ConstantInt>(v);
}

char ArchSimAA::ID = 0;

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

bool TryRecover_MemAccess(const llvm::Value *v, std::vector<int>& aaai)
{
	if(llvm::isa<llvm::Instruction>(v)) {
		auto mem_base = ((llvm::Instruction*)v)->getParent()->getParent()->getParent()->getGlobalVariable("contiguous_mem_base");
		if(v->stripInBoundsOffsets() == mem_base) {
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

bool TryRecover_StateBlock(const llvm::Value *v, std::vector<int> &aaai)
{
	// state block access is anything indexed off of arg 1 (%1)
	if(llvm::isa<llvm::Argument>(v)) {
		auto arg = (llvm::Argument*)v;
		auto state_block_ptr = &*(arg->getParent()->arg_begin() + 1);

		if(arg == state_block_ptr) {
			aaai = {TAG_CPU_STATE};
			return true;
		}
	}

	// try thread ptr
	const llvm::BitCastInst *bitcast = llvm::dyn_cast<const llvm::BitCastInst>(v);
	if(bitcast != nullptr) {
		auto state_block_ptr = &*(bitcast->getParent()->getParent()->arg_begin() + 1);

		if(bitcast->getOperand(0) == state_block_ptr) {
			aaai = {TAG_CPU_STATE};
			return true;
		}
	}

	return false;
}

bool RecoverAAAI(const llvm::Value *v, int size, std::vector<int> &aaai);

bool MergePhiValues(const llvm::PHINode *phi, std::set<llvm::Value*> &values)
{
	for(auto i = 0; i < phi->getNumIncomingValues(); ++i) {
		auto value = phi->getIncomingValue(i);

		if(llvm::isa<llvm::PHINode>(value)) {
			if(!values.count(value)) {
				MergePhiValues((llvm::PHINode*)value, values);
			}
		}

		values.insert(value);
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

bool TryRecover_FromPhi(const llvm::Value *v, int size, std::vector<int> &aaai)
{
	// Don't try to recover alias information from a phi node just now. Lots to consider e.g. loops, merging information, etc.

	if(llvm::isa<llvm::PHINode>(v)) {
		std::set<llvm::Value*> phi_values;
		MergePhiValues((llvm::PHINode*)v, phi_values);

		std::vector<int> base_aaai;
		RecoverAAAI(*phi_values.begin(), size, base_aaai);

		for(auto i : phi_values) {
			if(llvm::isa<llvm::PHINode>(i)) {
				continue;
			}

			RecoverAAAI(i, size, aaai);
			base_aaai = MergeAAAI(base_aaai, aaai);
		}

		aaai = base_aaai;
		return true;
	}

	return false;
}

bool TryRecover_FromSelect(const llvm::Value *v, int size, std::vector<int> &aaai)
{
	if(llvm::isa<llvm::SelectInst>(v)) {
		llvm::SelectInst *inst = (llvm::SelectInst*)v;
		std::vector<int> true_aaai, false_aaai;

		if(RecoverAAAI(inst->getTrueValue(), size, true_aaai) && RecoverAAAI(inst->getFalseValue(), size, false_aaai) && true_aaai == false_aaai) {
			aaai = true_aaai;
			return true;
		}

	}
	return false;
}

bool TryRecover_Chain(const llvm::Value *v, int size, std::vector<int> &output_aaai)
{
	return false;
}

// attempt to figure out what v actually is
bool RecoverAAAI(const llvm::Value *v, int size, std::vector<int> &output_aaai)
{
	if(TryRecover_FromPhi(v, size, output_aaai)) {
		return true;
	}
	if(TryRecover_FromSelect(v, size, output_aaai)) {
		return true;
	}

	if(TryRecover_RegAccess(v, size, output_aaai)) {
		return true;
	}
	if(TryRecover_MemAccess(v, output_aaai)) {
		return true;
	}
	if(TryRecover_StateBlock(v, output_aaai)) {
		return true;
	}
	if(TryRecover_Chain(v, size, output_aaai)) {
		return true;
	}

	return false;
}

bool GetArchsimAliasAnalysisInfo(const llvm::Value *v, int size, std::vector<int>& out)
{
	out.clear();
	llvm::MDNode *mdnode = nullptr;

	if(const llvm::Instruction *inst = llvm::dyn_cast<const llvm::Instruction>(v)) {
		mdnode = inst->getMetadata("aaai");
	} else if(const llvm::GlobalVariable *gv = llvm::dyn_cast<const llvm::GlobalVariable>(v)) {
		mdnode = gv->getMetadata("aaai");
	}

	if(mdnode) {
		for(int i = 0; i < mdnode->getNumOperands(); ++i) {
			auto &op = mdnode->getOperand(i);

			llvm::ConstantAsMetadata *constant = llvm::dyn_cast<llvm::ConstantAsMetadata>(op);
			if(!constant) {
				throw std::logic_error("unexpected metadata node");
			}

			llvm::ConstantInt *ci = llvm::dyn_cast<llvm::ConstantInt>(constant->getValue());
			if(!ci) {
				throw std::logic_error("unexpected metadata constant");
			}

			out.push_back(ci->getZExtValue());
		}
		return true;
	}

	return RecoverAAAI(v, size, out);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

llvm::AliasResult ArchSimAA::alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2)
{
	// First heuristic - really easy.  Taken from SCEV-AA
	if (L1.Size == 0 || L2.Size == 0) return llvm::NoAlias;

	// Second heuristic - obvious.
	if (L1.Ptr == L2.Ptr) return llvm::MustAlias;

	llvm::AliasResult rc = do_alias(L1, L2);

	if (archsim::options::JitDebugAA) {
		const llvm::Value *v1 = L1.Ptr, *v2 = L2.Ptr;

		bool print = false;

//		fprintf(stderr, "ALIAS: ");

		switch(rc) {
			case llvm::MayAlias:
//				fprintf(stderr, "MAY");
				print = true;
				break;
			case llvm::NoAlias:
//				fprintf(stderr, "NO");
				break;
			case llvm::MustAlias:
//				fprintf(stderr, "MUST");
				break;
			default:
//				fprintf(stderr, "???");
				print = true;
				break;
		}

//		fprintf(stderr, "\n");

		if(print) {

			std::vector<int> v1aa, v2aa;
			auto v1hasaa = GetArchsimAliasAnalysisInfo(v1, L1.Size.getValue(), v1aa);
			auto v2hasaa = GetArchsimAliasAnalysisInfo(v2, L2.Size.getValue(), v2aa);

			// it's OK if two memory pointers might alias.
			if(v1hasaa && v2hasaa && v1aa[0] == TAG_MEM_ACCESS && v2aa[0] == TAG_MEM_ACCESS) {
				return rc;
			}

			fprintf(stderr, "ALIAS PROBLEM:\n");

			fprintf(stderr, "%u, %u\n", L1.Size, L2.Size);

			if(v1hasaa && v1aa[0] == TAG_JT_ELEMENT) {
				fprintf(stderr, "(JT)\n");
			} else {
				std::string str;
				llvm::raw_string_ostream stream(str);
				v1->print(stream, false);
				fprintf(stderr, "%s\n", str.c_str());
			}

			if(v2hasaa && v2aa[0] == TAG_JT_ELEMENT) {
				fprintf(stderr, "(JT)\n");
			} else {
				std::string str;
				llvm::raw_string_ostream stream(str);
				v2->print(stream, false);
				fprintf(stderr, "%s\n", str.c_str());
			}

			if(v1hasaa) {
				fprintf(stderr, "V1AA: ");
				for(auto i : v1aa) {
					fprintf(stderr, "%u ", i);
				}
				fprintf(stderr, "\n");
			}
			if(v2hasaa) {
				fprintf(stderr, "V2AA: ");
				for(auto i : v2aa) {
					fprintf(stderr, "%u ", i);
				}
				fprintf(stderr, "\n");
			}

			fprintf(stderr, "\n");
		}

	}

	return rc;
}

llvm::AliasResult ArchSimAA::do_alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2)
{
	const llvm::Value *v1 = L1.Ptr, *v2 = L2.Ptr;

	std::vector<int> metadata_1, metadata_2;

	// Retrieve the ArcSim Alias-Analysis Information metadata node.
	int size1 = L1.Size.hasValue() ? L1.Size.getValue() : -1;
	int size2 = L2.Size.hasValue() ? L2.Size.getValue() : -1;

	GetArchsimAliasAnalysisInfo(v1, size1, metadata_1);
	GetArchsimAliasAnalysisInfo(v2, size2, metadata_2);

	// We must have AAAI for BOTH instructions to proceed.
	if (metadata_1.size() && metadata_2.size()) {

		uint64_t tag_1 = metadata_1.at(0);
		uint64_t tag_2 = metadata_2.at(0);

		if (tag_1 != tag_2) {
			// Tagged memory operations that have different types cannot alias.  I guarantee it.
			return llvm::NoAlias;
		} else {
			switch (tag_1) {
				case TAG_REG_ACCESS: {
					// The new register model produces alias information specifying pairs of
					// extents which are accessed.

					if(metadata_1.size() != 3 || metadata_2.size() != 3) {
						return llvm::MayAlias;
					}

					auto base_1 = metadata_1.at(1);
					auto base_2 = metadata_2.at(1);

					auto size_1 = metadata_1.at(2);
					auto size_2 = metadata_2.at(2);

					// if a size is unknown then the access may alias
					if(size_1 == -1 || size_2 == -1) {
						return llvm::MayAlias;
					}

					UNIMPLEMENTED;

					break;
				}
				case TAG_MEM_ACCESS:  // MEM
					// two accesses to memory, of course, might alias
					return llvm::MayAlias;
				case TAG_CPU_STATE:
				// A tagged CPU state is always a GEP %cpustate, 0, X - so check the third
				// operand for equivalency and return the appropriate result.
//						if (CONSTVAL(i1->getOperand(2)) == CONSTVAL(i2->getOperand(2))) {
//							return llvm::MustAlias;
//						} else {
//							return llvm::NoAlias;
//						}
				case TAG_JT_ELEMENT:
					break;
				case TAG_REGION_CHAIN_TABLE:
					return llvm::MayAlias;
				case TAG_SPARSE_PAGE_MAP:
					return llvm::MayAlias;
				case TAG_METRIC:
//						if (IsIntToPtr(i1) && IsIntToPtr(i2)) {
//							if (CONSTVAL(i1->getOperand(0)) == CONSTVAL(i2->getOperand(0)))
//								return llvm::MustAlias;
//							else
//								return llvm::NoAlias;
//						} else {
//							return llvm::MayAlias;
//						}
				case TAG_MMAP_ENTRY:
					return llvm::MayAlias;

				default:
					assert(!"Invalid AA metadata");
					break;
			}
		}
	}


	return llvm::MayAlias;
}


ArchSimAA::ArchSimAA() : llvm::AAResultBase<ArchSimAA>()
{
}


//void ArchSimAA::getAnalysisUsage(llvm::AnalysisUsage& AU) const
//{
//	AU.setPreservesAll();
//}


LLVMOptimiser::LLVMOptimiser() : isInitialised(false), my_aa_(nullptr)
{

}

LLVMOptimiser::~LLVMOptimiser()
{
}

bool LLVMOptimiser::Initialise(const ::llvm::DataLayout &datalayout)
{
	if(isInitialised) {
		return true;
	}
	llvm::PassManagerBuilder pmp;
	pmp.OptLevel = archsim::options::JitOptLevel.GetValue();

	if(my_aa_ == nullptr) {
		my_aa_ = new ArchSimAA();
	}

	if(!archsim::options::JitDisableAA) {
		pm.add(llvm::createExternalAAWrapperPass([&](llvm::Pass &pass, llvm::Function &function, llvm::AAResults &results) {
			results.addAAResult(*my_aa_);
		}));
	}

	pmp.populateModulePassManager(pm);

	isInitialised = true;

	return true;
}

bool LLVMOptimiser::AddPass(::llvm::Pass *pass)
{
	pm.add(pass);

	if (!archsim::options::JitDisableAA) {
		if(my_aa_ == nullptr) {
			my_aa_ = new ArchSimAA();
		}
		pm.add(llvm::createExternalAAWrapperPass([&](llvm::Pass &pass, llvm::Function &function, llvm::AAResults &results) {
			results.addAAResult(*my_aa_);
		}));
	}
	return true;
}

bool LLVMOptimiser::Optimise(::llvm::Module* module, const ::llvm::DataLayout &data_layout)
{
	if(archsim::options::Debug && ::llvm::verifyModule(*module, &::llvm::outs())) assert(false);

	if(!isInitialised)Initialise(data_layout);
	pm.run(*module);

	return true;
}
