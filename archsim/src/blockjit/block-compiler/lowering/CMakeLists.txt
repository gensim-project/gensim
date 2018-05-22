
# Determine whether we should enable the blockjit, based on whether we have
# a lowering here

IF("${ARCHSIM_SIMULATION_HOST}" STREQUAL "generic")
	SET(ARCHSIM_BLOCKJIT_ENABLED FALSE)
ELSE()
	IF("${ARCHSIM_SIMULATION_HOST}" STREQUAL "x86_64")
		ADD_SUBDIRECTORY(x86)
	ELSE()
		SET(ARCHSIM_BLOCKJIT_ENABLED FALSE)
	ENDIF()
ENDIF()

archsim_add_sources(
	Finalisation.cpp
	InstructionLowerer.cpp
	LoweringContext.cpp
)
