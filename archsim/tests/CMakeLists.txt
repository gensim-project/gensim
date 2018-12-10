# We need thread support
find_package(Threads REQUIRED)

SET(TEST_SRCS 
	blockjit/test-cmov.cpp blockjit/test-cmp-branch.cpp blockjit/test-cmp.cpp blockjit/test-compile.cpp
	general/test_test.cpp general/test_vector.cpp
	llvm/transform/test-archsim-dse.cpp llvm/transform/test-analysis.cpp 
)

ADD_EXECUTABLE(archsim-tests ${TEST_SRCS})
standard_flags(archsim-tests)

ADD_TEST(
	NAME archsim-tests 
	COMMAND archsim-tests
)

ADD_DEPENDENCIES(archsim-tests gtest)
ADD_DEPENDENCIES(archsim-tests archsim-core)
TARGET_LINK_LIBRARIES(archsim-tests ${GTEST_LIBS_DIR}/libgtest.a ${GTEST_LIBS_DIR}/libgtest_main.a ${CMAKE_THREAD_LIBS_INIT} archsim-core)
TARGET_INCLUDE_DIRECTORIES(archsim-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GTEST_INCLUDE_DIR} inc/)
