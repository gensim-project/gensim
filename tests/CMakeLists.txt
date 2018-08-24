
# Check out test artifact repo containing binaries and images

INCLUDE(ExternalProject)

ExternalProject_Add(gensim-test-artifacts
	HG_REPOSITORY "https://bitbucket.org/gensim/gensim-test-artifacts"
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
)

SET(GENSIM_TEST_ARTIFACTS "${CMAKE_CURRENT_BINARY_DIR}/gensim-test-artifacts-prefix/src/gensim-test-artifacts/artifacts")

ADD_SUBDIRECTORY(arm-linux-tests)
ADD_SUBDIRECTORY(arm-user-tests)
ADD_SUBDIRECTORY(x86_64-user-tests)
