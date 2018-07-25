CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(wutils)

ADD_LIBRARY(wutils INTERFACE)

TARGET_INCLUDE_DIRECTORIES(wutils INTERFACE inc/)


# Import and build gtest
# From http://www.kaizou.org/2014/11/gtest-cmake/

# We need thread support
find_package(Threads REQUIRED)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

SET(GTEST_LIBS_DIR "${binary_dir}/googlemock/gtest/")
SET(GTEST_INCLUDE_DIR "${source_dir}/googletest/include")

FILE(GLOB_RECURSE TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/tests *.cpp)

ADD_EXECUTABLE(libwutils-tests ${TEST_SRCS})

ADD_TEST(
	NAME libwutils-tests
	COMMAND libwutils-tests
)

ADD_DEPENDENCIES(libwutils-tests wutils)
TARGET_LINK_LIBRARIES(libwutils-tests ${GTEST_LIBS_DIR}/libgtest.a ${GTEST_LIBS_DIR}/libgtest_main.a ${CMAKE_THREAD_LIBS_INIT} wutils)
TARGET_INCLUDE_DIRECTORIES(libwutils-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GTEST_INCLUDE_DIR} inc/)
