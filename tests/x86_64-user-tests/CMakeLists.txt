ADD_TEST(
	NAME 
		x86_64-hello-world
	COMMAND
		archsim -s x86 -m x86-user -l sparse -e ${GENSIM_TEST_ARTIFACTS}/x86_64-user/x86_64-hello-world
)
