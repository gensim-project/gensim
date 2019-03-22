/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMOptimiser.h"
#include "translate/llvm/LLVMAliasAnalysis.h"
#include "translate/llvm/passes/LexicographicallyOrderBlocksPass.h"

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
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
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

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static llvm::Function *GetFunction(const llvm::Value *v1, const llvm::Value *v2)
{
	llvm::Function *fn = nullptr;

	if(llvm::isa<llvm::Instruction>(v1)) {
		fn = ((llvm::Instruction*)v1)->getFunction();
	}
	if(llvm::isa<llvm::Instruction>(v2)) {
		fn = ((llvm::Instruction*)v2)->getFunction();
	}

	if(llvm::isa<llvm::Argument>(v1)) {
		fn = ((llvm::Argument*)v1)->getParent();
	}
	if(llvm::isa<llvm::Argument>(v2)) {
		fn = ((llvm::Argument*)v2)->getParent();
	}

	return fn;
}

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

			llvm::Function *fn = GetFunction(v1, v2);
			if(fn == nullptr) {
				UNEXPECTED;
			}

			std::vector<int> v1aa, v2aa;

			PointerInformationProvider pip (fn);
			auto v1hasaa = pip.GetPointerInfo(v1, v1aa);
			auto v2hasaa = pip.GetPointerInfo(v2, v2aa);

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

	llvm::Function *fn = GetFunction(v1, v2);
	if(fn == nullptr) {
		return llvm::MayAlias;
	}

	PointerInformationProvider pip (fn);
	pip.GetPointerInfo(v1, metadata_1);
	pip.GetPointerInfo(v2, metadata_2);

//	GetArchsimAliasAnalysisInfo(v1, size1, metadata_1);
//	GetArchsimAliasAnalysisInfo(v2, size2, metadata_2);

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

					//TODO: fix this
					return llvm::MayAlias;

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
	if(my_aa_ == nullptr) {
		my_aa_ = new ArchSimAA();
	}

	if(archsim::options::JitOptLevel.GetValue() == 4) {
		pm.add(new LLVMRegisterOptimisationPass());
	}

	if(archsim::options::JitOptString.IsSpecified()) {
		std::string str = archsim::options::JitOptString.GetValue();
		for(int i = 0; i < str.size(); ++i) {
			switch(str[i]) {
				case 'N':
					break; // nop
				case 'a':
					pm.add(llvm::createCFGSimplificationPass());
					break;
				case 'b':
					pm.add(llvm::createEarlyCSEPass());
					break;
				case 'c':
					pm.add(llvm::createLowerExpectIntrinsicPass());
					break;
				case 'd':
					pm.add(llvm::createPromoteMemoryToRegisterPass());
					break;
				case 'e':
					pm.add(llvm::createJumpThreadingPass());
					break;
				case 'f':
					pm.add(llvm::createCFGSimplificationPass());
				case 'g':
					pm.add(llvm::createReassociatePass());
					break;
				case 'h':
					pm.add(llvm::createMemCpyOptPass());
					break;
				case 'i':
					pm.add(llvm::createSimpleLoopUnrollPass(2));
					break;
				case 'j':
					pm.add(llvm::createLICMPass());
					break;
				case 'k':
					pm.add(llvm::createSCCPPass());
					break;
				case 'l':
					pm.add(llvm::createBitTrackingDCEPass());
					break;
				case 'm':
					pm.add(llvm::createAggressiveDCEPass());
					break;
				case 'n':
					pm.add(llvm::createCFGSimplificationPass(1, true, true, false, true));
					break;
				case 'o':
					pm.add(llvm::createNewGVNPass());
					break;
				case 'p':
					pm.add(llvm::createDeadStoreEliminationPass());
					break;
				case 'q':
					pm.add(llvm::createInstructionCombiningPass(false));
					break;
				case 'r':
					pm.add(llvm::createInstructionCombiningPass(true));
					break;
			}
		}

	} else {
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

//		pm.add(new LLVMRegisterOptimisationPass());
//		pm.add(new LexicographicallyOrderBlocksPass());


		pmp.populateModulePassManager(pm);

//		if(!archsim::options::JitDisableAA) {
//			pm.add(llvm::createExternalAAWrapperPass([&](llvm::Pass &pass, llvm::Function &function, llvm::AAResults &results) {
//				results.addAAResult(*my_aa_);
//			}));
//		}
//
//		pm.add(llvm::createTypeBasedAAWrapperPass());
//		pm.add(llvm::createBasicAAWrapperPass());
//
//		pm.add(llvm::createCFGSimplificationPass());
//		pm.add(llvm::createEarlyCSEPass());
//		pm.add(llvm::createLowerExpectIntrinsicPass());
//
//		pm.add(llvm::createPromoteMemoryToRegisterPass());
//		pm.add(llvm::createJumpThreadingPass());
//		pm.add(llvm::createCFGSimplificationPass());
//		pm.add(llvm::createReassociatePass());
//
//		pm.add(llvm::createMemCpyOptPass());
//
//		pm.add(llvm::createSimpleLoopUnrollPass(2));   // Unroll small loops
//		pm.add(llvm::createLICMPass());
//
//		pm.add(llvm::createSCCPPass());
//
//		pm.add(llvm::createBitTrackingDCEPass());
//		pm.add(llvm::createJumpThreadingPass());
//		pm.add(llvm::createDeadStoreEliminationPass());
//
//		pm.add(llvm::createSCCPPass());
//		pm.add(llvm::createAggressiveDCEPass());
//		pm.add(llvm::createCFGSimplificationPass(1, true, true, false, true));
//
//		pm.add(llvm::createNewGVNPass());
//		pm.add(llvm::createDeadStoreEliminationPass());
//
//		pm.add(llvm::createInstructionCombiningPass(true));
	}
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
