archsim_add_sources(
	ExecutionContextManager.cpp
	ExecutionEngine.cpp
	BasicJITExecutionEngine.cpp
	BlockJITExecutionEngine.cpp
	InterpreterExecutionEngine.cpp

	ExecutionEngineFactory.cpp
)

IF(ARCHSIM_ENABLE_LLVM)
archsim_add_sources(
	LLVMRegionJITExecutionEngine.cpp
	BlockToLLVMExecutionEngine.cpp
	BlockLLVMExecutionEngine.cpp
)
ENDIF()