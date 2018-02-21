CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

find_package(Antlr REQUIRED)

add_library(gensim-lib SHARED)
add_library(gensim-test SHARED)

STANDARD_FLAGS(gensim-lib)
STANDARD_FLAGS(gensim-test)

TARGET_LINK_LIBRARIES(gensim-lib PRIVATE gensim-test gensim-grammar ${ANTLR_LIB})

TARGET_INCLUDE_DIRECTORIES(gensim-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
TARGET_INCLUDE_DIRECTORIES(gensim-lib PUBLIC ${ANTLR_INCLUDE_DIR})
TARGET_INCLUDE_DIRECTORIES(gensim-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

SET_TARGET_PROPERTIES(gensim-lib
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
)

SET_TARGET_PROPERTIES(gensim-test
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
)

ADD_SUBDIRECTORY(src)

if(NOT ANTLR_FOUND)
	MESSAGE(SEND_ERROR "Could not find ANTLR")
endif()

# Also build tools
function(gensim_tool tool_name)

	ADD_EXECUTABLE(${tool_name} tools/${tool_name}.cpp)
	TARGET_LINK_LIBRARIES(${tool_name} gensim-lib)
	TARGET_LINK_LIBRARIES(${tool_name} gensim-grammar)

	SET_TARGET_PROPERTIES(${tool_name}
		PROPERTIES
			CXX_STANDARD 11
			CXX_STANDARD_REQUIRED YES
	)

	STANDARD_FLAGS(${tool_name})

endfunction()

# Tools to build
gensim_tool(gensim)
gensim_tool(gensim-frontend)
gensim_tool(genc-opt)


# Also include tests
ADD_SUBDIRECTORY(tests)
