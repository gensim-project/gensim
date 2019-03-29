archsim_add_sources(
	Component.cpp 
	Device.cpp 
	EmptyDevice.cpp 
	PeripheralManager.cpp 
	IRQController.cpp 
	MMU.cpp 
	SerialPort.cpp 
	DeviceManager.cpp 
	WSBlockDevice.cpp
)

ADD_SUBDIRECTORY(arm)
ADD_SUBDIRECTORY(aarch64)
ADD_SUBDIRECTORY(generic)
ADD_SUBDIRECTORY(gfx)
ADD_SUBDIRECTORY(virtio)

IF(MODEL_model-riscv64_ENABLED OR MODEL_model-riscv32_ENABLED)
	ADD_SUBDIRECTORY(riscv)
ENDIF()
