PROJECT(libtrace)

FIND_PACKAGE(Curses QUIET)
pkg_check_modules(CAPSTONE capstone)

FILE(GLOB LIBTRACE_SOURCES lib/*.cpp)

if(CAPSTONE_FOUND==1)
	MESSAGE(STATUS "Found Capstone so including diassembler")
	FILE(GLOB LIBTRACE_DISASM_SOURCES lib/disasm/*.cpp)
endif()

ADD_LIBRARY(trace ${LIBTRACE_SOURCES} ${LIBTRACE_DISASM_SOURCES})

SET(INCLUDEDIRS inc/)

TARGET_INCLUDE_DIRECTORIES(trace PUBLIC inc/ ${CURSES_INCLUDE_DIR} ${CAPSTONE_INCLUDE_DIR})
TARGET_COMPILE_OPTIONS(trace PRIVATE -fno-rtti)

if(CAPSTONE_FOUND==1)
	TARGET_LINK_LIBRARIES(trace ${CAPSTONE_LIBRARIES})
endif()

SET_TARGET_PROPERTIES(trace
	PROPERTIES
		CXX_STANDARD 14
		CXX_STANDARD_REQUIRED YES
		POSITION_INDEPENDENT_CODE ON
)
standard_flags(trace)

function(add_trace_tool tool-name)
	ADD_EXECUTABLE(${tool-name} ${ARGN})

	standard_flags(${tool-name})

	TARGET_LINK_LIBRARIES(${tool-name} trace ${CURSES_LIBRARIES})

	if(CAPSTONE_FOUND==1)
		target_compile_definitions(${tool-name} PRIVATE HAVE_CAPSTONE=1)
	endif()
endfunction()

if(CURSES_FOUND)
	MESSAGE(STATUS "Found Curses so building TraceLess")
	add_trace_tool(TraceCat ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordCat.cpp)
	add_trace_tool(TracePCDiff ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordPCDiff.cpp)
    add_trace_tool(TraceMemDiff ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordDiffMemory.cpp)
	add_trace_tool(TraceIRDiff ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordIRDiff.cpp)
	add_trace_tool(TraceTail ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordTail.cpp)
	add_trace_tool(TraceLess ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordLess.cpp)
	add_trace_tool(TraceCut ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordCut.cpp)
endif()

SET_PROPERTY(GLOBAL PROPERTY LIBTRACE_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/inc")

# Also build a PIN version of the library
ADD_LIBRARY(trace-pin ${LIBTRACE_SOURCES})

SET(PIN_LOCATION /home/harry/Work/intel-pin/pin-3.7-97619-g0d0c92f4f-gcc-linux/)
SET(PIN_INCLUDE_LOCATION ${PIN_LOCATION}/source/include/pin)

TARGET_INCLUDE_DIRECTORIES(trace-pin PUBLIC inc/ ${CURSES_INCLUDE_DIR})
TARGET_COMPILE_DEFINITIONS(trace-pin PRIVATE -DTARGET_LINUX -DHOST_IA32E -DTARGET_IA32E -D__PIN__=1 -DPIN_CRT=1)
TARGET_COMPILE_OPTIONS(trace-pin PRIVATE -g -nostdlib -fabi-version=2 -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -fPIC)

TARGET_LINK_LIBRARIES(trace-pin "-nostdlib")

SET_TARGET_PROPERTIES(trace-pin
	PROPERTIES
		CXX_STANDARD 14
		CXX_STANDARD_REQUIRED YES
		POSITION_INDEPENDENT_CODE ON
)

TARGET_INCLUDE_DIRECTORIES(trace-pin SYSTEM PRIVATE
	${PIN_LOCATION}/extras/stlport/include/
	${PIN_LOCATION}/extras/libstdc++/include/
	${PIN_LOCATION}/extras/crt/include
	${PIN_LOCATION}/extras/crt/include/arch-x86_64
	${PIN_LOCATION}/extras/crt/include/kernel/uapi
	${PIN_LOCATION}/extras/crt/include/kernel/uapi/asm-x86
)

TARGET_INCLUDE_DIRECTORIES(trace-pin PRIVATE
	${PIN_LOCATION}/extras/components/include
	${PIN_LOCATION}/extras/xed-intel64/include/xed
)
