PROJECT(libtrace)

FIND_PACKAGE(Curses)

FILE(GLOB LIBTRACE_SOURCES lib/*.cpp)
ADD_LIBRARY(trace ${LIBTRACE_SOURCES})

SET(INCLUDEDIRS inc/)

TARGET_INCLUDE_DIRECTORIES(trace PUBLIC inc/ ${CURSES_INCLUDE_DIR})

SET_TARGET_PROPERTIES(trace
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
)

function(add_trace_tool tool-name)
	ADD_EXECUTABLE(${tool-name} ${ARGN})
	
	standard_flags(${tool-name})
	
	TARGET_LINK_LIBRARIES(${tool-name} trace ${CURSES_LIBRARIES})

	SET_TARGET_PROPERTIES(${tool-name}
		PROPERTIES
			CXX_STANDARD 11
			CXX_STANDARD_REQUIRED YES
	)
endfunction()

add_trace_tool(TraceDiffMemory ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordDiffMemory.cpp)
add_trace_tool(TraceCat ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordCat.cpp)
add_trace_tool(TracePCDiff ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordPCDiff.cpp)
add_trace_tool(TraceTail ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordTail.cpp)

if(CURSES_FOUND)
	MESSAGE(STATUS "Found Curses so building TraceLess")
	add_trace_tool(TraceLess ${CMAKE_CURRENT_SOURCE_DIR}/tools/RecordLess.cpp)
endif()

SET_PROPERTY(GLOBAL PROPERTY LIBTRACE_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/inc")
