
ADD_TEST(
	NAME 
		armv7a-linux-boot
	COMMAND
		sh -c "python ${CMAKE_CURRENT_SOURCE_DIR}/run-to-userspace.py -e ${GENSIM_TEST_ARTIFACTS}/arm-linux-user/arm-realview-zimage"
)
