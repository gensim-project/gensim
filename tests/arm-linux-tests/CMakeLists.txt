
ADD_TEST(armv7a-linux-boot
	COMMAND
		sh build-linux.sh && python run-to-userspace.py
