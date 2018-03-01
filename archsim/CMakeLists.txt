# Archsim top-level CMAKE file

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

# Dependencies
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(SDL2)

FIND_PACKAGE(PkgConfig)
IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(GTK2 gtk+-2.0)
ENDIF()

# Add the main ArchSim executable to the build, along with its dependencies
ADD_EXECUTABLE(archsim src/main.cpp)
ADD_LIBRARY(archsim-interface INTERFACE)

TARGET_COMPILE_OPTIONS(archsim PRIVATE "-fomit-frame-pointer")

standard_flags(archsim)

SET_PROPERTY(GLOBAL PROPERTY ARCHSIM_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/inc")

TARGET_LINK_LIBRARIES(archsim
	${CMAKE_THREAD_LIBS_INIT}
	${CMAKE_DL_LIBS}

	trace
)

SET_TARGET_PROPERTIES(archsim
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
		POSITION_INDEPENDENT_CODE ON
)

# Add each provided source file along with its source directory
function(archsim_add_sources)
	foreach(source ${ARGN})
		TARGET_SOURCES(archsim PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
	endforeach()
endfunction()

# Include the archsim include directory
TARGET_INCLUDE_DIRECTORIES(archsim PRIVATE "inc/")
TARGET_INCLUDE_DIRECTORIES(archsim PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/inc/")
TARGET_INCLUDE_DIRECTORIES(archsim PRIVATE)
TARGET_INCLUDE_DIRECTORIES(archsim-interface INTERFACE "inc/")
TARGET_INCLUDE_DIRECTORIES(archsim-interface INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/inc/")

TARGET_COMPILE_DEFINITIONS(archsim PUBLIC "-DARCHSIM_MODULE_DIRECTORY=${ARCHSIM_MODULE_DIRECTORY}")

# Process the configuration files
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/inc/cmake-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/inc/cmake-config.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/inc/cmake-scm.h.in ${CMAKE_CURRENT_BINARY_DIR}/inc/cmake-scm.h)

IF(SDL2_FOUND)
	TARGET_LINK_LIBRARIES(archsim ${SDL2_LIBRARIES})
ENDIF()

IF(GTK2_FOUND)
	TARGET_LINK_LIBRARIES(archsim ${GTK2_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(archsim PRIVATE ${GTK2_INCLUDE_DIRS})
ENDIF()

# Include helper functions
INCLUDE(CrossCompiling)

# Add the archsim src directory to the build
ADD_SUBDIRECTORY("src")

# Recursively build modules
ADD_SUBDIRECTORY("modules")


# Add extra objects, dependency targets, and extra source files (which might be generated)
TARGET_LINK_LIBRARIES(archsim
	${ARCHSIM_EXTRA_OBJECTS}
)

foreach(dep ${ARCHSIM_EXTRA_TARGETS})
	ADD_DEPENDENCIES(archsim ${dep})
endforeach()

foreach(dep ${ARCHSIM_EXTRA_SOURCES})	
	target_sources(archsim PRIVATE ${dep})
	SET_SOURCE_FILES_PROPERTIES(${dep} PROPERTIES GENERATED TRUE)
endforeach()
