ADD_SUBDIRECTORY(arch)
ADD_SUBDIRECTORY(execution)
ADD_SUBDIRECTORY(thread)

archsim_add_sources(
	MemoryInterface.cpp
	MemoryMonitor.cpp
)
