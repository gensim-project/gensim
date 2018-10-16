
ADD_TEST(
	NAME
		armv7a-linux-boot
	COMMAND
		sh -c "python3 ${CMAKE_CURRENT_SOURCE_DIR}/run-to-userspace.py -e ${GENSIM_TEST_ARTIFACTS}/arm-linux-user/arm-realview-zimage"
)

SET_TESTS_PROPERTIES(
	armv7a-linux-boot
	PROPERTIES
		TIMEOUT 300 # Could take longer than this if we're using an interpreter or slow machine, but we need to keep things moving
)
