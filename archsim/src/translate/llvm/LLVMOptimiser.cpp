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

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/PromoteMemToReg.h>

UseLogContext(LogTranslate);
DeclareChildLogContext(LogAliasAnalysis, LogTranslate, "AliasAnalysis");

#define PERMISSIVE

#define CONSTVAL(a) (llvm::mdconst::extract<llvm::ConstantInt>(a)->getZExtValue())
#define ISCONSTVAL(a) (a->getValueID() == ::llvm::Instruction::ConstantIntVal)

using namespace archsim::translate::translate_llvm;

#if 0

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

static llvm::RegisterPass<ArchsimAAPass> P("archsim-aa", "ArcSim-specific Alias Analysis", false, true);
static llvm::RegisterAnalysisGroup<llvm::AAResultsWrapperPass> G(P);

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

		fprintf(stderr, "ALIAS: ");

		switch(rc) {
			case llvm::MayAlias:
				fprintf(stderr, "MAY");
				break;
			case llvm::NoAlias:
				fprintf(stderr, "NO");
				break;
			case llvm::MustAlias:
				fprintf(stderr, "MUST");
				break;
			default:
				fprintf(stderr, "???");
				break;
		}

		fprintf(stderr, "\n");
		v1->dump();
		v2->dump();
	}

	return rc;
}

llvm::AliasResult ArchSimAA::do_alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2)
{
	const llvm::Value *v1 = L1.Ptr, *v2 = L2.Ptr;

	if (IsInstr(v1) && IsInstr(v2)) {
		// Allocas do not alias with anytihng.
		if (IsAlloca(v1) || IsAlloca(v2)) {
			return llvm::NoAlias;
		}

		// Let's test the case where we are checking two instructions for aliasing.
		const llvm::Instruction *i1 = (const llvm::Instruction *)v1;
		const llvm::Instruction *i2 = (const llvm::Instruction *)v2;

		// Retrieve the ArcSim Alias-Analysis Information metadata node.
		const llvm::MDNode *md1 = i1->getMetadata("aaai");
		const llvm::MDNode *md2 = i2->getMetadata("aaai");

		// We must have AAAI for BOTH instructions to proceed.
		if (md1 != NULL && md2 != NULL && md1->getNumOperands() >= 1 && md2->getNumOperands() >= 1) {

			uint64_t tag_1 = CONSTVAL(md1->getOperand(0).get());
			uint64_t tag_2 = CONSTVAL(md2->getOperand(0).get());

			// Actually I break the below guarantee by saying that an SMM_PAGE could alias with MEM
			if(tag_1 == TAG_SMM_PAGE && tag_2 == TAG_MEM_ACCESS) return llvm::MayAlias;
			if(tag_2 == TAG_SMM_PAGE && tag_1 == TAG_MEM_ACCESS) return llvm::MayAlias;

			if (tag_1 != tag_2) {
				// Tagged memory operations that have different types cannot alias.  I guarantee it.
				return llvm::NoAlias;
			} else {
				AliasAnalysisTag tag = (AliasAnalysisTag)CONSTVAL(md1->getOperand(0));
				switch (tag) {
					case TAG_REG_ACCESS: {
						// The new register model produces alias information specifying the
						// minimum start and maximum end offsets of the access into the register
						// file, as well as the size of the access such that (addr + size < max)

						// If the number of operands is low, this indicates an automatically
						// generated register access which we should treat as MayAlias in all
						// cases.
						if(md1->getNumOperands() == 1 || md2->getNumOperands() == 1) return llvm::MayAlias;

						auto &min1_val = md1->getOperand(1);
						auto &max1_val = md1->getOperand(2);

						auto &size1_val = md1->getOperand(3);

						auto &min2_val = md2->getOperand(1);
						auto &max2_val = md2->getOperand(2);

						auto &size2_val = md2->getOperand(3);

						assert(IsConstVal(size1_val) && IsConstVal(size2_val));
						uint32_t size1 = CONSTVAL(size1_val);
						uint32_t size2 = CONSTVAL(size2_val);

						assert(size1 && size2);

						if(IsConstVal(min1_val) && IsConstVal(max1_val) && IsConstVal(min2_val) && IsConstVal(max2_val)) {
							uint32_t min1 = CONSTVAL(min1_val);
							uint32_t max1 = CONSTVAL(max1_val);
							uint32_t min2 = CONSTVAL(min2_val);
							uint32_t max2 = CONSTVAL(max2_val);

							assert(min1 != max1);
							assert(min2 != max2);

							// If the extents are identical (and the sizes are the same), the pointers MUST alias
							if((min1 == min2) && (max1 == max2) && (size1 == size2)) {
								return llvm::MustAlias;
							}

							// If the extents overlap, the pointers MAY alias
							if((min1 < max2) && (max1 > min2)) {
//							printf("MAY alias: %u->%u (%u) vs %u->%u (%u)\n", min1, max1, size1, min2, max2, size2);
								return llvm::MayAlias;
							}
							// If there is no overlap, the pointers DO NOT alias
							else {
								return llvm::NoAlias;
							}
						} else {
							// If we can't evaluate one of the extents, then we don't know if the pointers alias or not
							return llvm::MayAlias;
						}

						break;
					}
					case TAG_MEM_ACCESS:  // MEM
						if (IsIntToPtr(v1) && IsIntToPtr(v2)) {
//							llvm::IntToPtrInst *i2p1 = (llvm::IntToPtrInst *)v1;
//							llvm::IntToPtrInst *i2p2 = (llvm::IntToPtrInst *)v2;
//							llvm::Value *v1op = i2p1->getOperand(0);
//							llvm::Value *v2op = i2p2->getOperand(0);
//
//							// We perhaps need to be clever(er) here.
//							if (v1op == v2op) {
//								return llvm::MustAlias;
//							} else if (IsConstExpr(v1op) && IsConstExpr(v2op)) {
//								// HMMMMMMMMMMM - think about this.  Is it strictly necessary?
//								// Musings are that v1op and v2op must be equal, if they are
//								// const expressions and their constvals are equal.  But, maybe not.
//								// Depending on how equality is implemented/overridden (at all) then
//								// there may be two different llvm::Value's which are indeed the
//								// same constant.
//								return CONSTVAL(v1op) == CONSTVAL(v2op) ? llvm::MustAlias : llvm::NoAlias;
//							}
						}
						break;
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
	} else if (IsAlloca(v1) || IsAlloca(v2)) {
		// Allocas don't alias with anything.
		return llvm::NoAlias;
	} else if ((IsInstr(v1) || IsInstr(v2)) && (IsConstExpr(v1) || IsConstExpr(v2))) {
		const llvm::Instruction *insn = IsInstr(v1) ? (const llvm::Instruction *)v1 : (const llvm::Instruction *)v2;
		const llvm::MDNode *insn_md = insn->getMetadata("aaai");

		if (insn_md != NULL && CONSTVAL(insn_md->getOperand(0)) == TAG_MEM_ACCESS) {
			return llvm::MayAlias;
		} else {
			return llvm::NoAlias;
		}

		// TODO: Think about this one some more.
		//return MayAlias;
	} else if (IsConstExpr(v1) && IsConstExpr(v2)) {
		const llvm::ConstantExpr *c1 = (const llvm::ConstantExpr *)v1;
		const llvm::ConstantExpr *c2 = (const llvm::ConstantExpr *)v2;
		llvm::Constant *c1v = c1->getOperand(0);
		llvm::Constant *c2v = c2->getOperand(0);

		if (c1v->getValueID() == llvm::Instruction::ConstantIntVal && c2v->getValueID() == llvm::Instruction::ConstantIntVal) {
			llvm::ConstantInt *c1iv = (llvm::ConstantInt *)c1v;
			llvm::ConstantInt *c2iv = (llvm::ConstantInt *)c2v;

			if (c1iv->getZExtValue() == c2iv->getZExtValue())
				return llvm::MustAlias;
			else
				return llvm::NoAlias;
		} else
			return llvm::MayAlias; // TODO: We might be able to be cleverer here.
	}

	return llvm::AAResultBase<ArchSimAA>::alias(L1, L2);
}

char ArchsimAA::ID = 0;

ArchSimAA::ArchSimAA() : FunctionPass(ID)
{
}


void ArchsimAA::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
	AU.setPreservesAll();
}

bool ArchsimAA::runOnFunction(llvm::Function& F)
{
  return false;
}

#endif


LLVMOptimiser::LLVMOptimiser() : isInitialised(false)
{

}

LLVMOptimiser::~LLVMOptimiser()
{
}

bool LLVMOptimiser::Initialise(const ::llvm::DataLayout *datalayout)
{
//	isInitialised = true;
//	pm.add(::llvm::createTypeBasedAAWrapperPass());
//	//AddPass(new ::llvm::DataLayout(*datalayout));
//
//	if (archsim::options::JitOptLevel.GetValue() == 0)
//		return true;
//
//	//-constmerge -simplifycfg -lowerswitch  -globalopt -scalarrepl -deadargelim -functionattrs -constprop  -simplifycfg -argpromotion -inline -mem2reg -deadargelim
//	AddPass(::llvm::createConstantMergePass());
//	AddPass(::llvm::createCFGSimplificationPass());
//	AddPass(::llvm::createLowerSwitchPass());
//	AddPass(::llvm::createGlobalOptimizerPass());
//	AddPass(::llvm::createSROAPass());
//	AddPass(::llvm::createDeadArgEliminationPass());
////	AddPass(::llvm::createFunctionAttrsPass());
//	AddPass(::llvm::createConstantPropagationPass());
//	AddPass(::llvm::createCFGSimplificationPass());
//	AddPass(::llvm::createArgumentPromotionPass());
//	AddPass(::llvm::createFunctionInliningPass());
//	AddPass(::llvm::createPromoteMemoryToRegisterPass());
//	AddPass(::llvm::createDeadArgEliminationPass());
//
//	//-argpromotion -loop-deletion -adce -loop-deletion -dse -break-crit-edges -ipsccp -break-crit-edges -deadargelim -simplifycfg -gvn -prune-eh -die -constmerge
//	AddPass(::llvm::createArgumentPromotionPass());
//	AddPass(::llvm::createLoopDeletionPass());
//	AddPass(::llvm::createAggressiveDCEPass());
//	AddPass(::llvm::createLoopDeletionPass());
//	AddPass(::llvm::createDeadStoreEliminationPass());
//	AddPass(::llvm::createBreakCriticalEdgesPass());
//	AddPass(::llvm::createIPSCCPPass());
//	AddPass(::llvm::createBreakCriticalEdgesPass());
//	AddPass(::llvm::createDeadArgEliminationPass());
//	AddPass(::llvm::createCFGSimplificationPass());
////	AddPass(::llvm::createGVNPass(false));
//	AddPass(::llvm::createPruneEHPass());
//	AddPass(::llvm::createDeadInstEliminationPass());
//	AddPass(::llvm::createConstantMergePass());
//
//	//-tailcallelim -simplifycfg -dse -globalopt  -loop-unswitch -memcpyopt -loop-unswitch -ipconstprop -deadargelim -jump-threading
//	AddPass(::llvm::createTailCallEliminationPass());
//	AddPass(::llvm::createCFGSimplificationPass());
//	AddPass(::llvm::createDeadStoreEliminationPass());
//	AddPass(::llvm::createGlobalOptimizerPass());
//	AddPass(::llvm::createLoopUnswitchPass());
//	AddPass(::llvm::createMemCpyOptPass());
//	AddPass(::llvm::createLoopUnswitchPass());
//	AddPass(::llvm::createIPConstantPropagationPass());
//	AddPass(::llvm::createDeadArgEliminationPass());
//	AddPass(::llvm::createJumpThreadingPass());
	/*
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

	AddPass(::llvm::createFunctionAttrsPass());
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

	AddPass(::llvm::createGVNPass());
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


		AddPass(::llvm::createFunctionAttrsPass());
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

		AddPass(::llvm::createGVNPass());
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
	*/
	return true;
}

bool LLVMOptimiser::AddPass(::llvm::Pass *pass)
{
	if (!archsim::options::JitDisableAA) {
//		pm.add(createArcSimAliasAnalysisPass());
	}

	pm.add(pass);

	return true;
}

bool LLVMOptimiser::Optimise(::llvm::Module* module, const ::llvm::DataLayout *data_layout)
{
	//std::ostringstream str;
	if(archsim::options::Debug && ::llvm::verifyModule(*module, &::llvm::outs())) assert(false);

	if(!isInitialised)Initialise(data_layout);
//	pm.run(*module);



	return true;
}
