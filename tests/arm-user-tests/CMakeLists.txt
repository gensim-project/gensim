

ADD_TEST(
	NAME 
		armv7a-arm-hello-world
	COMMAND
		archsim -s armv7a -m arm-user -l contiguous -e ${GENSIM_TEST_ARTIFACTS}/arm-user/arm-hello-world
)

ADD_TEST(
	NAME 
		armv7a-thumb-hello-world
	COMMAND
		archsim -s armv7a -m arm-user -l contiguous -e ${GENSIM_TEST_ARTIFACTS}/arm-user/thumb-hello-world
)
