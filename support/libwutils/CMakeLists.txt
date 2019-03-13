CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(wutils)

ADD_LIBRARY(wutils INTERFACE)

TARGET_INCLUDE_DIRECTORIES(wutils INTERFACE inc/)

# We need thread support
find_package(Threads REQUIRED)

IF(TESTING_ENABLED)
	FILE(GLOB_RECURSE TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/tests *.cpp)

	ADD_EXECUTABLE(libwutils-tests ${TEST_SRCS})

	ADD_TEST(
		NAME libwutils-tests
		COMMAND libwutils-tests
	)
	
	ADD_DEPENDENCIES(libwutils-tests wutils gtest)
	TARGET_LINK_LIBRARIES(libwutils-tests ${GTEST_LIBS_DIR}/libgtest.a ${GTEST_LIBS_DIR}/libgtest_main.a ${CMAKE_THREAD_LIBS_INIT} wutils)
	TARGET_INCLUDE_DIRECTORIES(libwutils-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GTEST_INCLUDE_DIR} inc/)
ENDIF()
