archsim_add_sources(
	LLVMAliasAnalysis.cpp
	LLVMGuestRegisterAccessEmitter.cpp
	LLVMMemoryManager.cpp
	LLVMMemoryAccessEmitter.cpp
	LLVMOptimiser.cpp
	LLVMTranslation.cpp
	LLVMTranslationContext.cpp
	LLVMRegisterOptimisationPass.cpp
	LLVMWorkUnitTranslator.cpp
	LLVMBlockTranslator.cpp
	LLVMCompiler.cpp
)

ADD_SUBDIRECTORY(passes)