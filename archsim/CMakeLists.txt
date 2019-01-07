# Archsim top-level CMAKE file

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
include(ExternalProject)

# Dependencies
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(SDL2 QUIET)
FIND_PACKAGE(X11 QUIET)

FIND_PACKAGE(PkgConfig QUIET)
IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(GTK3 gtk+-3.0)
ENDIF()

IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(Valgrind valgrind)
	IF(Valgrind_FOUND AND NOT ARCHSIM_DISABLE_VALGRIND)
		SET(archsim_extra_options ${archsim_extra_options} ${Valgrind_CFLAGS} -DCONFIG_VALGRIND=1)
		SET(archsim_libs "${archsim_extra_libs};${Valgrind_LIBRARIES}")
		LINK_DIRECTORIES(${Valgrind_LIBRARY_DIRS})
	ENDIF()
ENDIF()


# Add the main ArchSim executable to the build, along with its dependencies
ADD_LIBRARY(archsim-core SHARED)
ADD_EXECUTABLE(archsim src/main.cpp)

standard_flags(archsim)
standard_flags(archsim-core)

TARGET_COMPILE_OPTIONS(archsim-core PRIVATE ${archsim_extra_options} -fomit-frame-pointer)

SET_PROPERTY(GLOBAL PROPERTY ARCHSIM_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/inc")

TARGET_LINK_LIBRARIES(archsim
	archsim-core
	 -Wl,-z,now
)

TARGET_LINK_LIBRARIES(archsim-core 
	trace 
	gvnc
	wutils

	${CMAKE_THREAD_LIBS_INIT}
	${CMAKE_DL_LIBS}
)

# Need intel XED for the x86 model

# It has its own weird mbuild stuff (lucky us)
ExternalProject_Add(
	mbuild
	
	GIT_REPOSITORY https://github.com/intelxed/mbuild.git
	PREFIX intel-xed
	CONFIGURE_COMMAND true
	BUILD_COMMAND true
	INSTALL_COMMAND true
)

ExternalProject_Add(
	IntelXED
	
	GIT_REPOSITORY https://github.com/intelxed/xed.git
	PREFIX intel-xed
	DEPENDS mbuild
	CONFIGURE_COMMAND true
	BUILD_COMMAND cd ../IntelXED && ./mfile.py --shared --opt=3 --install-dir ${CMAKE_CURRENT_BINARY_DIR}/IntelXED/kit/ install
	INSTALL_COMMAND echo "XED ready at ${CMAKE_CURRENT_BINARY_DIR}/IntelXED/kit/"
)

SET(ARCHSIM_XED_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/IntelXED/kit/ CACHE STRING "XED Kit location")
ADD_DEPENDENCIES(archsim-core IntelXED)

TARGET_INCLUDE_DIRECTORIES(archsim-core PRIVATE "${ARCHSIM_XED_DIRECTORY}/include")
TARGET_LINK_LIBRARIES(archsim-core "${ARCHSIM_XED_DIRECTORY}/lib/libxed.so")



# Add each provided source file along with its source directory
function(archsim_add_sources)
	foreach(source ${ARGN})
		TARGET_SOURCES(archsim-core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
	endforeach()
endfunction()

# Include the archsim include directory
TARGET_INCLUDE_DIRECTORIES(archsim-core PUBLIC "inc/")
TARGET_COMPILE_DEFINITIONS(archsim-core PUBLIC "-DARCHSIM_MODULE_DIRECTORY=${ARCHSIM_MODULE_DIRECTORY}")

TARGET_COMPILE_DEFINITIONS(archsim-core PUBLIC "-DARCHSIM_CONFIGSTRING=${CONFIGSTRING}")

# Process the configuration files
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/inc/cmake-scm.h.in ${CMAKE_CURRENT_SOURCE_DIR}/inc/cmake-scm.h)

# Determing the simulation host that we're building for
IF(NOT ARCHSIM_SIMULATION_HOST)

	# What simulation hosts are supported? This actually depends on what features
	# are desired (blockjit, llvm, interpreter, etc) but for now we're either
	# x86 or we're generic
	IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
		SET(ARCHSIM_SIMULATION_HOST_ "${CMAKE_SYSTEM_PROCESSOR}")
	ELSE()
		SET(ARCHSIM_SIMULATION_HOST_ "generic")
	ENDIF()
	
	SET(ARCHSIM_SIMULATION_HOST "${ARCHSIM_SIMULATION_HOST_}" CACHE STRING "What simulation host should ArchSim be compiled for?")
ENDIF()

MESSAGE(STATUS "ArchSim simulation host: ${ARCHSIM_SIMULATION_HOST}")
TARGET_COMPILE_DEFINITIONS(archsim PUBLIC "-DARCHSIM_SIMULATION_HOST_IS_${ARCHSIM_SIMULATION_HOST}=1")

# Depending on the version of SDL2 which is available, they might be 
# using the old system of xxx_LIBRARIES or the new imported target
# system. 
IF(SDL2_FOUND)
	if(NOT SDL2_LIBRARIES)
		TARGET_LINK_LIBRARIES(archsim-core SDL2::SDL2)
	else()
		TARGET_LINK_LIBRARIES(archsim-core ${SDL2_LIBRARIES})
	endif()
ENDIF()

IF(GTK3_FOUND)
	TARGET_LINK_LIBRARIES(archsim-core ${GTK3_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(archsim-core PRIVATE ${GTK3_INCLUDE_DIRS})
ENDIF()

SET(ARCHSIM_ENABLE_LLVM FALSE CACHE BOOL "Should LLVM DBT support be used in ArchSim?")
SET(ARCHSIM_LLVM_DIR "" CACHE STRING "Location of LLVM cmake configuration to be used in ArchSim")

IF(ARCHSIM_ENABLE_LLVM)
	FIND_PACKAGE(LLVM 8.0.0 REQUIRED CONFIG PATHS ${ARCHSIM_LLVM_DIR})
	TARGET_COMPILE_OPTIONS(archsim-core PUBLIC "-DARCHSIM_ENABLE_LLVM=1")
	
	IF(LLVM_FOUND)
		llvm_map_components_to_libnames(llvm_libs target x86desc x86info x86codegen x86asmparser runtimedyld linker support core irreader asmparser codegen mcjit orcjit executionengine passes)

		message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
		message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

		message(STATUS "Using LLVM Libs: ${llvm_libs}")

		TARGET_COMPILE_DEFINITIONS(archsim-core PRIVATE ${LLVM_DEFINITIONS})
		TARGET_COMPILE_DEFINITIONS(archsim-core PUBLIC "-DARCHSIM_ENABLE_LLVM=1")
		TARGET_LINK_LIBRARIES(archsim-core ${llvm_libs})
		TARGET_INCLUDE_DIRECTORIES(archsim-core PUBLIC ${LLVM_INCLUDE_DIRS})
	ELSE()
		MESSAGE(SEND_ERROR "ARCHSIM_ENABLE_LLVM was set but LLVM could not be found.")
	ENDIF()
ENDIF()

# Include helper functions
INCLUDE(CrossCompiling)

# Add the archsim src directory to the build
ADD_SUBDIRECTORY("src")

# Recursively build modules
ADD_SUBDIRECTORY("modules")

# Build tests
ADD_SUBDIRECTORY("tests")

# Add extra objects, dependency targets, and extra source files (which might be generated)
TARGET_LINK_LIBRARIES(archsim-core
	${ARCHSIM_EXTRA_OBJECTS}
)

foreach(dep ${ARCHSIM_EXTRA_TARGETS})
	ADD_DEPENDENCIES(archsim-core ${dep})
endforeach()

foreach(dep ${ARCHSIM_EXTRA_SOURCES})	
	target_sources(archsim-core PRIVATE ${dep})
	SET_SOURCE_FILES_PROPERTIES(${dep} PROPERTIES GENERATED TRUE)
endforeach()
