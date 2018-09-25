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

bool TryRecover_RegAccess(const llvm::Value *v, std::vector<int> &aaai)
{
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
	const llvm::IntToPtrInst *i2pinst = llvm::dyn_cast<const llvm::IntToPtrInst>(v);
	if(i2pinst != nullptr) {
		const auto *i2pop = i2pinst->getOperand(0);
		const llvm::BinaryOperator *add_inst = llvm::dyn_cast<const llvm::BinaryOperator>(i2pop);
		if(add_inst != nullptr && add_inst->getOpcode() == llvm::BinaryOperator::Add) {
			const llvm::ConstantInt *op0 = llvm::dyn_cast<const llvm::ConstantInt>(add_inst->getOperand(0));
			const llvm::ConstantInt *op1 = llvm::dyn_cast<const llvm::ConstantInt>(add_inst->getOperand(1));

			if(op0 != nullptr) {
				// look for the contiguous memory base, minus some amount to account for constant folding in llvm
				if(op0->getZExtValue() >= 0x80000000 - 4096) { // this is super unsafe: is there some way we can do better?
					aaai = {TAG_MEM_ACCESS};
					return true;
				}
			}
			if(op1 != nullptr) {
				// look for the contiguous memory base, minus some amount to account for constant folding in llvm
				if(op1->getZExtValue() >= 0x80000000 - 4096) { // this is super unsafe: is there some way we can do better?
					aaai = {TAG_MEM_ACCESS};
					return true;
				}
			}
		}
	}

	const llvm::ConstantExpr *i2pcexpr = llvm::dyn_cast<const llvm::ConstantExpr>(v);
	if(i2pcexpr != nullptr && i2pcexpr->getOpcode() == llvm::Instruction::IntToPtr) {
		auto *i2p_base = i2pcexpr->getOperand(0);
		if(llvm::isa<llvm::ConstantInt>(i2p_base) && ((llvm::ConstantInt*)i2p_base)->getZExtValue() >= 0x80000000) {
			aaai = {TAG_MEM_ACCESS};
			return true;
		}
	}

	return false;
}

bool TryRecover_StateBlock(const llvm::Value *v, std::vector<int> &aaai)
{
	// state block access is anything indexed off of arg 1 (%1)

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

// attempt to figure out what v actually is
bool RecoverAAAI(const llvm::Value *v, std::vector<int> &output_aaai)
{
	if(TryRecover_RegAccess(v, output_aaai)) {
		return true;
	}
	if(TryRecover_MemAccess(v, output_aaai)) {
		return true;
	}
	if(TryRecover_StateBlock(v, output_aaai)) {
		return true;
	}

	return false;
}

bool GetArchsimAliasAnalysisInfo(const llvm::Value *v, std::vector<int>& out)
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

	return RecoverAAAI(v, out);
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
			fprintf(stderr, "ALIAS PROBLEM:\n");

			std::vector<int> v1aa, v2aa;
			auto v1hasaa = GetArchsimAliasAnalysisInfo(v1, v1aa);
			auto v2hasaa = GetArchsimAliasAnalysisInfo(v2, v2aa);

//			v1->dump();
//			v2->dump();

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
		}

	}

	return rc;
}

llvm::AliasResult ArchSimAA::do_alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2)
{
	const llvm::Value *v1 = L1.Ptr, *v2 = L2.Ptr;

	std::vector<int> metadata_1, metadata_2;

	// Retrieve the ArcSim Alias-Analysis Information metadata node.
	GetArchsimAliasAnalysisInfo(v1, metadata_1);
	GetArchsimAliasAnalysisInfo(v2, metadata_2);

	// We must have AAAI for BOTH instructions to proceed.
	if (metadata_1.size() && metadata_2.size()) {

		uint64_t tag_1 = metadata_1.at(0);
		uint64_t tag_2 = metadata_2.at(0);

		if (tag_1 != tag_2) {
			// Tagged memory operations that have different types cannot alias.  I guarantee it.
			return llvm::NoAlias;
		} else {
			AliasAnalysisTag tag = (AliasAnalysisTag)metadata_1.at(0);
			switch (tag) {
				case TAG_REG_ACCESS: {
					// The new register model produces alias information specifying the
					// minimum start and maximum end offsets of the access into the register
					// file, as well as the size of the access such that (addr + size < max)

					// If the number of operands is low, this indicates an automatically
					// generated register access which we should treat as MayAlias in all
					// cases.
					if(metadata_1.size() == 1 || metadata_2.size() == 1) return llvm::MayAlias;

					auto &min1_val = metadata_1.at(1);
					auto &max1_val = metadata_1.at(2);

					auto &size1_val = metadata_1.at(3);

					auto &min2_val = metadata_2.at(1);
					auto &max2_val = metadata_2.at(2);

					auto &size2_val = metadata_2.at(3);

					uint32_t size1 = (size1_val);
					uint32_t size2 = (size2_val);

					assert(size1 && size2);

					uint32_t min1 = (min1_val);
					uint32_t max1 = (max1_val);
					uint32_t min2 = (min2_val);
					uint32_t max2 = (max2_val);

					assert(min1 != max1);
					assert(min2 != max2);

					// If the extents are identical (and the sizes are the same), the pointers MUST alias
					if((min1 == min2) && (max1 == max2) && (size1 == size2)) {
						return llvm::MustAlias;
					}

					// If the extents overlap, the pointers MAY alias
					if((min1 < max2) && (max1 > min2)) {
//							printf("PARTIAL alias: %u->%u (%u) vs %u->%u (%u)\n", min1, max1, size1, min2, max2, size2);
						if(min1 == min2) {
							return llvm::MustAlias;
						} else {
							return llvm::PartialAlias;
						}
					}
					// If there is no overlap, the pointers DO NOT alias
					else {
						return llvm::NoAlias;
					}

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
	isInitialised = true;
	pm.add(::llvm::createBasicAAWrapperPass());
//	//AddPass(new ::llvm::DataLayout(*datalayout));
//
	if (archsim::options::JitOptLevel.GetValue() == 0)
		return true;
//
//	//-constmerge -simplifycfg -lowerswitch  -globalopt -scalarrepl -deadargelim -functionattrs -constprop  -simplifycfg -argpromotion -inline -mem2reg -deadargelim
	AddPass(::llvm::createPromoteMemoryToRegisterPass());
	AddPass(::llvm::createConstantMergePass());
	AddPass(::llvm::createCFGSimplificationPass());
	AddPass(::llvm::createLowerSwitchPass());
	AddPass(::llvm::createGlobalOptimizerPass());
	AddPass(::llvm::createSROAPass());
	AddPass(::llvm::createDeadArgEliminationPass());
//	AddPass(::llvm::createFunctionAttrsPass());
	AddPass(::llvm::createConstantPropagationPass());
	AddPass(::llvm::createCFGSimplificationPass());
	AddPass(::llvm::createArgumentPromotionPass());
	AddPass(::llvm::createFunctionInliningPass());
	AddPass(::llvm::createPromoteMemoryToRegisterPass());
	AddPass(::llvm::createDeadArgEliminationPass());
//
//	//-argpromotion -loop-deletion -adce -loop-deletion -dse -break-crit-edges -ipsccp -break-crit-edges -deadargelim -simplifycfg -gvn -prune-eh -die -constmerge
	AddPass(::llvm::createArgumentPromotionPass());
	AddPass(::llvm::createLoopDeletionPass());
	AddPass(::llvm::createAggressiveDCEPass());
	AddPass(::llvm::createLoopDeletionPass());
	AddPass(::llvm::createDeadStoreEliminationPass());
	AddPass(::llvm::createBreakCriticalEdgesPass());
	AddPass(::llvm::createIPSCCPPass());
	AddPass(::llvm::createBreakCriticalEdgesPass());
	AddPass(::llvm::createDeadArgEliminationPass());
	AddPass(::llvm::createCFGSimplificationPass());
	AddPass(::llvm::createGVNPass(false));
	AddPass(::llvm::createPruneEHPass());
	AddPass(::llvm::createDeadInstEliminationPass());
	AddPass(::llvm::createConstantMergePass());
//
//	//-tailcallelim -simplifycfg -dse -globalopt  -loop-unswitch -memcpyopt -loop-unswitch -ipconstprop -deadargelim -jump-threading
	AddPass(::llvm::createTailCallEliminationPass());
	AddPass(::llvm::createCFGSimplificationPass());
	AddPass(::llvm::createDeadStoreEliminationPass());
	AddPass(::llvm::createGlobalOptimizerPass());
	AddPass(::llvm::createLoopUnswitchPass());
	AddPass(::llvm::createMemCpyOptPass());
	AddPass(::llvm::createLoopUnswitchPass());
	AddPass(::llvm::createIPConstantPropagationPass());
	AddPass(::llvm::createDeadArgEliminationPass());
	AddPass(::llvm::createJumpThreadingPass());

	AddPass(::llvm::createGlobalOptimizerPass());
	AddPass(::llvm::createIPSCCPPass());
	AddPass(::llvm::createDeadArgEliminationPass());

	AddPass(::llvm::createInstructionCombiningPass());
	AddPass(::llvm::createDeadInstEliminationPass());
	AddPass(::llvm::createCFGSimplificationPass());

	if (archsim::options::JitOptLevel.GetValue() == 1)
		return true;


	AddPass(::llvm::createPruneEHPass());
	AddPass(::llvm::createFunctionInliningPass());

//	AddPass(::llvm::createFunctionAttrsPass());
	AddPass(::llvm::createAggressiveDCEPass());

	AddPass(::llvm::createArgumentPromotionPass());

	AddPass(::llvm::createPromoteMemoryToRegisterPass());
	//	AddPass(::llvm::createSROAPass(false));

	AddPass(::llvm::createEarlyCSEPass());
	AddPass(::llvm::createJumpThreadingPass());
	AddPass(::llvm::createCorrelatedValuePropagationPass());
	AddPass(::llvm::createCFGSimplificationPass());
	AddPass(::llvm::createInstructionCombiningPass());

	AddPass(::llvm::createTailCallEliminationPass());
	AddPass(::llvm::createCFGSimplificationPass());

	AddPass(::llvm::createReassociatePass());
	//AddPass(::llvm::createLoopRotatePass());

	AddPass(::llvm::createInstructionCombiningPass());
	AddPass(::llvm::createIndVarSimplifyPass());
	//AddPass(::llvm::createLoopIdiomPass());
	//AddPass(::llvm::createLoopDeletionPass());

	//AddPass(::llvm::createLoopUnrollPass());

	AddPass(::llvm::createGVNPass(false));
	AddPass(::llvm::createMemCpyOptPass());
	AddPass(::llvm::createSCCPPass());

	AddPass(::llvm::createInstructionCombiningPass());
	AddPass(::llvm::createJumpThreadingPass());
	AddPass(::llvm::createCorrelatedValuePropagationPass());

	if (archsim::options::JitOptLevel.GetValue() == 2)
		return true;

	AddPass(::llvm::createDeadStoreEliminationPass());
	//AddPass(::llvm::createSLPVectorizerPass());
	AddPass(::llvm::createAggressiveDCEPass());
	AddPass(::llvm::createCFGSimplificationPass());
	AddPass(::llvm::createInstructionCombiningPass());
	AddPass(::llvm::createBarrierNoopPass());
	//AddPass(::llvm::createLoopVectorizePass(false));
	AddPass(::llvm::createInstructionCombiningPass());
	AddPass(::llvm::createCFGSimplificationPass());
	AddPass(::llvm::createDeadStoreEliminationPass());
	AddPass(::llvm::createStripDeadPrototypesPass());
	AddPass(::llvm::createGlobalDCEPass());
	AddPass(::llvm::createConstantMergePass());

	if(archsim::options::JitOptLevel.GetValue() == 4) {
		AddPass(::llvm::createFunctionInliningPass(1000));

		AddPass(::llvm::createGlobalOptimizerPass());
		AddPass(::llvm::createIPSCCPPass());
		AddPass(::llvm::createDeadArgEliminationPass());

		AddPass(::llvm::createInstructionCombiningPass());
		AddPass(::llvm::createCFGSimplificationPass());

		AddPass(::llvm::createPruneEHPass());


//		AddPass(::llvm::createFunctionAttrsPass());
		AddPass(::llvm::createAggressiveDCEPass());

		AddPass(::llvm::createArgumentPromotionPass());

		AddPass(::llvm::createPromoteMemoryToRegisterPass());
		//		AddPass(::llvm::createSROAPass(false));

		AddPass(::llvm::createEarlyCSEPass());
		AddPass(::llvm::createJumpThreadingPass());
		AddPass(::llvm::createCorrelatedValuePropagationPass());
		AddPass(::llvm::createCFGSimplificationPass());
		AddPass(::llvm::createInstructionCombiningPass());

		AddPass(::llvm::createTailCallEliminationPass());
		AddPass(::llvm::createCFGSimplificationPass());

		AddPass(::llvm::createReassociatePass());

		AddPass(::llvm::createInstructionCombiningPass());
		AddPass(::llvm::createIndVarSimplifyPass());

		AddPass(::llvm::createGVNPass(false));
		AddPass(::llvm::createMemCpyOptPass());
		AddPass(::llvm::createSCCPPass());

		AddPass(::llvm::createInstructionCombiningPass());
		AddPass(::llvm::createJumpThreadingPass());
		AddPass(::llvm::createCorrelatedValuePropagationPass());

		AddPass(::llvm::createDeadStoreEliminationPass());
		//AddPass(::llvm::createSLPVectorizerPass());
		AddPass(::llvm::createAggressiveDCEPass());
		AddPass(::llvm::createCFGSimplificationPass());
		AddPass(::llvm::createInstructionCombiningPass());
		AddPass(::llvm::createBarrierNoopPass());
		//AddPass(::llvm::createLoopVectorizePass(false));
		AddPass(::llvm::createInstructionCombiningPass());
		AddPass(::llvm::createCFGSimplificationPass());
		AddPass(::llvm::createDeadStoreEliminationPass());
		AddPass(::llvm::createStripDeadPrototypesPass());
		AddPass(::llvm::createGlobalDCEPass());
		AddPass(::llvm::createConstantMergePass());

	}

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
