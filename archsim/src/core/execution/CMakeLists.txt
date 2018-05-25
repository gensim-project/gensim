archsim_add_sources(
	ExecutionContextManager.cpp
	ExecutionEngine.cpp
	BlockJITExecutionEngine.cpp
	InterpreterExecutionEngine.cpp

	ExecutionEngineFactory.cpp
)

IF(ARCHSIM_ENABLE_LLVM)
archsim_add_sources(
	LLVMRegionJITExecutionEngine.cpp
	BlockLLVMExecutionEngine.cpp
)
ENDIF()