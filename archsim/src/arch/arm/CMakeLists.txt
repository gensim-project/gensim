archsim_add_sources(
	AngelSyscallHandler.cpp
	ARMDecodeContext.cpp
	ArmLinuxUserEmulationModel.cpp
	ArmDoomLinuxUserEmulationModel.cpp
)	

# Compile ARM bootloader sources only if cross compiler is detected
cross_compile_try_prefix(arm ARM_CC_PREFIX)

MESSAGE(STATUS "Got ARM CC Prefix ${ARM_CC_PREFIX}")

if(ARM_CC_PREFIX_valid)
	MESSAGE(STATUS "Found a CC")
	SET(ARM_CC_PREFIX_FOUND 1)
else()
	MESSAGE(STATUS "NO CC")
	SET(ARM_CC_PREFIX_FOUND 0)
endif()
SET(ARCHSIM_BUILD_ARM_SYSTEM "${ARM_CC_PREFIX_FOUND}" CACHE STRING "Build the ARMv7 Full System emulation models")

if(${ARCHSIM_BUILD_ARM_SYSTEM})
	MESSAGE(STATUS "Building ARM bootloader and system models")

	cross_compile_bin(
			arm
			"-march=armv5"
			ArmBootloader.S
			bootloader
	)
	archsim_add_sources(
		ArmRealviewEmulationModel.cpp
		ArmVersatileEmulationModel.cpp
	)
else()
	MESSAGE(STATUS "Skipping ARM Full System emulation models")
endif()

SET(ARCHSIM_EXTRA_SOURCES "${ARCHSIM_EXTRA_SOURCES}" PARENT_SCOPE)
SET(ARCHSIM_EXTRA_TARGETS "${ARCHSIM_EXTRA_TARGETS}" PARENT_SCOPE)
