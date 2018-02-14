cross_compile_binary(arm arm-hello-world "-march=armv5 -static" ${CMAKE_CURRENT_SOURCE_DIR}/hello-world.c)

add_test(
	NAME arm-hello-world-test
	COMMAND archsim -s armv7a -e arm-hello-world -m arm-user -l contiguous --mode interp
)
add_custom_target(
	NAME build-arm-hello-world ALL
	DEPENDS arm-hello-world
)
