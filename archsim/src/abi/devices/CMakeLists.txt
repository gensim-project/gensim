archsim_add_sources(
	Component.cpp 
	Device.cpp 
	PeripheralManager.cpp 
	IRQController.cpp 
	MMU.cpp 
	SerialPort.cpp 
	DeviceManager.cpp 
	WSBlockDevice.cpp
)

ADD_SUBDIRECTORY(arm)
ADD_SUBDIRECTORY(generic)
ADD_SUBDIRECTORY(gfx)
ADD_SUBDIRECTORY(virtio)
