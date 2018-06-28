# Import and build gtest
# From http://www.kaizou.org/2014/11/gtest-cmake/

# We need thread support
find_package(Threads REQUIRED)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

SET(GTEST_LIBS_DIR "${binary_dir}/googlemock/gtest/")
SET(GTEST_INCLUDE_DIR "${source_dir}/googletest/include")

FILE(GLOB_RECURSE TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

ADD_EXECUTABLE(gensim-tests ${TEST_SRCS})

ADD_TEST(
	NAME gensim-tests 
	COMMAND gensim-tests
)

standard_flags(gensim-tests)

ADD_DEPENDENCIES(gensim-tests gtest)
TARGET_LINK_LIBRARIES(gensim-tests ${GTEST_LIBS_DIR}/libgtest.a ${GTEST_LIBS_DIR}/libgtest_main.a ${CMAKE_THREAD_LIBS_INIT} gensim-lib gensim-test)
TARGET_INCLUDE_DIRECTORIES(gensim-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GTEST_INCLUDE_DIR})
