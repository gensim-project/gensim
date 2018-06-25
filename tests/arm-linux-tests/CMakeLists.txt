
ADD_TEST(
	NAME 
		armv7a-linux-boot
	COMMAND
		sh -c "cp ${CMAKE_CURRENT_SOURCE_DIR}/realview-config . && sh ${CMAKE_CURRENT_SOURCE_DIR}/build-linux.sh && python ${CMAKE_CURRENT_SOURCE_DIR}/run-to-userspace.py"
)
