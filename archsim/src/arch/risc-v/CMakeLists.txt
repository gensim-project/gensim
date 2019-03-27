IF(MODEL_model-riscv64_ENABLED OR MODEL_model-riscv32_ENABLED)

archsim_add_sources(
	RiscVDecodeContext.cpp
	RiscVLinuxUserEmulationModel.cpp
	RiscVComplianceEmulationModel.cpp
	RiscVSifiveFU540EmulationModel.cpp
	RiscVSifiveFreedomEEmulationModel.cpp
	RiscVSystemCoprocessor.cpp
	RiscVSystemEmulationModel.cpp
	RiscVMMU.cpp
)

cross_compile_bin(
        riscv64
        ""
        riscv-linux-vdso.S
        riscv64_linux_vdso
)

SET(ARCHSIM_EXTRA_SOURCES "${ARCHSIM_EXTRA_SOURCES}" PARENT_SCOPE)
SET(ARCHSIM_EXTRA_TARGETS "${ARCHSIM_EXTRA_TARGETS}" PARENT_SCOPE)

ENDIF()
