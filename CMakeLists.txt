CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(GenSim C CXX)
ENABLE_LANGUAGE(ASM)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

INCLUDE(StandardFlags)

# Use ccache if it can be found
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Enable tests if requested by the user
IF(TESTING_ENABLED)
	ENABLE_TESTING()
	MESSAGE(STATUS "Tests enabled")
ELSE()
	MESSAGE(STATUS "Tests disabled")
ENDIF()

EXECUTE_PROCESS(
	COMMAND "hg" "id"
	OUTPUT_VARIABLE SCM_REV
	OUTPUT_STRIP_TRAILING_WHITESPACE
)


# Put things into an output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin)
set(ARCHSIM_MODULE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/archsim-modules)

function(TARGET_ADD_SOURCES target)
	foreach(source ${ARGN})
		TARGET_SOURCES(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${source})
	endforeach()
endfunction()

ADD_SUBDIRECTORY(archsim)
ADD_SUBDIRECTORY(gensim)
ADD_SUBDIRECTORY(scripts)

ADD_SUBDIRECTORY(support/libtrace)
ADD_SUBDIRECTORY(support/libgvnc)
ADD_SUBDIRECTORY(support/libwutils)

ADD_SUBDIRECTORY(models)
ADD_SUBDIRECTORY(tests)

