archsim_add_sources(
	AngelSyscallHandler.cpp
	ARMDecodeContext.cpp
	ArmLinuxUserEmulationModel.cpp
	ArmDoomLinuxUserEmulationModel.cpp
	ArmRealviewEmulationModel.cpp
	ArmVersatileEmulationModel.cpp
)	

# Compile ARM bootloader sources
cross_compile_bin(
        arm
        "-march=armv5"
        ArmBootloader.S
        bootloader
)

cross_compile_bin(
        arm
        "-march=armv5"
        EabiHelpers.S
        eabi_helpers
)

SET(ARCHSIM_EXTRA_SOURCES "${ARCHSIM_EXTRA_SOURCES}" PARENT_SCOPE)
SET(ARCHSIM_EXTRA_TARGETS "${ARCHSIM_EXTRA_TARGETS}" PARENT_SCOPE)
