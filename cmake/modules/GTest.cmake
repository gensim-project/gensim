# Enable ExternalProject CMake module
include(ExternalProject)

find_package(Threads REQUIRED)

# Get GTest source and binary directories from CMake project
# Download and install GoogleTest
ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(gtest source_dir binary_dir)

SET(GTEST_LIBS_DIR "${binary_dir}/lib/")
SET(GTEST_INCLUDE_DIR "${source_dir}/googletest/include")
