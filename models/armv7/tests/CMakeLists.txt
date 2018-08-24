cross_compile_try_sysroot("arm-linux-gnu")

if(NOT SYSROOT-NOTFOUND)
	cross_compile_binary(arm arm-hello-world "-march=armv5 -static" ${CMAKE_CURRENT_SOURCE_DIR}/hello-world.c)

	add_test(
		NAME arm-hello-world-test
		COMMAND archsim -s armv7a -e arm-hello-world -m arm-user -l contiguous 
	)
	add_custom_target(
		NAME build-arm-hello-world ALL
		DEPENDS arm-hello-world
	)

else()
	MESSAGE(WARNING "No ARM sysroot found so not all tests will be run")
endif()

