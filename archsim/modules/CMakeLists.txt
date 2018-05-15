ADD_SUBDIRECTORY("arm-system-devices")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gpu")
	ADD_SUBDIRECTORY("gpu")
endif()
