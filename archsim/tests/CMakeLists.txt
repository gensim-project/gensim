# We need thread support
find_package(Threads REQUIRED)

# Enable ExternalProject CMake module
include(ExternalProject)

# Download and install GoogleTest
ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

SET(GTEST_LIBS_DIR "${binary_dir}/googlemock/gtest/")
SET(GTEST_INCLUDE_DIR "${source_dir}/googletest/include")

FILE(GLOB_RECURSE TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

ADD_EXECUTABLE(archsim-tests ${TEST_SRCS})

ADD_TEST(
	NAME archsim-tests 
	COMMAND archsim-tests
)

SET_TARGET_PROPERTIES(archsim-tests
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
)

ADD_DEPENDENCIES(archsim-tests gtest)
TARGET_LINK_LIBRARIES(archsim-tests ${GTEST_LIBS_DIR}/libgtest.a ${GTEST_LIBS_DIR}/libgtest_main.a ${CMAKE_THREAD_LIBS_INIT} archsim-core)
TARGET_INCLUDE_DIRECTORIES(archsim-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GTEST_INCLUDE_DIR})
