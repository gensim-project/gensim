archsim_add_sources(
	X86DecodeContext.cpp
	X86Decoder.cpp
	X86LinuxUserEmulationModel.cpp
	X86DoomLinuxUserEmulationModel.cpp
)

SET(ARCHSIM_EXTRA_SOURCES "${ARCHSIM_EXTRA_SOURCES}" PARENT_SCOPE)
SET(ARCHSIM_EXTRA_TARGETS "${ARCHSIM_EXTRA_TARGETS}" PARENT_SCOPE)
