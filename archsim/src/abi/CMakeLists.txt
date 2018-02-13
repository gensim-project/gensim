archsim_add_sources(
	EmulationModel.cpp 
	SystemEmulationModel.cpp 
	UserEmulationModel.cpp 
	LinuxUserEmulationModel.cpp 
	LinuxSystemEmulationModel.cpp 
	ElfSystemEmulationModel.cpp
)

ADD_SUBDIRECTORY(devices)
ADD_SUBDIRECTORY(loader)
ADD_SUBDIRECTORY(memory)
ADD_SUBDIRECTORY(user)
