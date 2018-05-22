archsim_add_sources(
	Finalisation.cpp
	InstructionLowerer.cpp
	LoweringContext.cpp
)
IF("${ARCHSIM_SIMULATION_HOST}" STREQUAL "x86_64")
	ADD_SUBDIRECTORY(x86)
ENDIF()