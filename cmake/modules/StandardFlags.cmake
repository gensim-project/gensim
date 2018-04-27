# Include standard flags for DEBUG, DEBUGOPT, and RELEASE

IF(NOT CMAKE_BUILD_TYPE)
	MESSAGE(WARNING "No build type was specified, so defaulting to DEBUG")
	SET(CMAKE_BUILD_TYPE DEBUG)
ENDIF()

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	SET(CMAKE_BUILD_TYPE "DEBUG")
ENDIF()

MESSAGE(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

function(standard_flags target-name)

	IF("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
		TARGET_COMPILE_OPTIONS(${target-name} PRIVATE -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-reorder)
		SET(CONFIGSTRING "Debug" PARENT_SCOPE)
	ELSEIF("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUGOPT")
		TARGET_COMPILE_OPTIONS(${target-name} PRIVATE -g -O2)
		SET(CONFIGSTRING "Debug (Opt)" PARENT_SCOPE)
	ELSEIF("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
		TARGET_COMPILE_OPTIONS(${target-name} PRIVATE -O3)
		SET(CONFIGSTRING "Release" PARENT_SCOPE)
	ELSE()
		MESSAGE(SEND_ERROR "Unknown build type \"${CMAKE_BUILD_TYPE}\"")
	ENDIF()

endfunction()
