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
ADD_SUBDIRECTORY(riscv)
