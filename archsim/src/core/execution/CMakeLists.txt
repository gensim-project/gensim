archsim_add_sources(
	ExecutionContextManager.cpp
	ExecutionEngine.cpp
)

IF(ARCHSIM_SIMULATION_HOST STREQUAL "x86_64")
	archsim_add_sources(
		BlockJITExecutionEngine.cpp
	)
ENDIF()