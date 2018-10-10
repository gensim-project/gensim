ADD_LIBRARY(pintrace SHARED pintrace.cpp)
TARGET_LINK_LIBRARIES(pintrace trace-pin)

SET(PIN_LOCATION /home/harry/Work/intel-pin/pin-3.7-97619-g0d0c92f4f-gcc-linux/)
SET(PIN_INCLUDE_LOCATION ${PIN_LOCATION}/source/include/pin)


TARGET_INCLUDE_DIRECTORIES(pintrace PRIVATE ${PIN_INCLUDE_LOCATION})
TARGET_INCLUDE_DIRECTORIES(pintrace PRIVATE ${PIN_INCLUDE_LOCATION}/gen)
TARGET_COMPILE_DEFINITIONS(pintrace PRIVATE -DTARGET_LINUX -DHOST_IA32E -DTARGET_IA32E -D__PIN__=1 -DPIN_CRT=1)
TARGET_COMPILE_OPTIONS(pintrace PRIVATE -g -fabi-version=2 -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -fPIC)


TARGET_LINK_LIBRARIES(pintrace "${PIN_LOCATION}/intel64/runtime/pincrt/crtbeginS.o")
TARGET_LINK_LIBRARIES(pintrace "-nostdlib -Wl,--hash-style=sysv -Wl,-Bsymbolic -Wl,--version-script=${PIN_LOCATION}/source/include/pin/pintool.ver")

TARGET_LINK_LIBRARIES(pintrace 
	${PIN_LOCATION}/intel64/lib/libpin.a
	${PIN_LOCATION}/extras/xed-intel64/lib/libxed.so
	${PIN_LOCATION}/intel64/lib-ext/libpin3dwarf.so
	${PIN_LOCATION}/intel64/runtime/pincrt/libdl-dynamic.so
	${PIN_LOCATION}/intel64/runtime/pincrt/libstlport-dynamic.so
	${PIN_LOCATION}/intel64/runtime/pincrt/libm-dynamic.so
	${PIN_LOCATION}/intel64/runtime/pincrt/libc-dynamic.so
	${PIN_LOCATION}/intel64/runtime/pincrt/libunwind-dynamic.so
)

TARGET_LINK_LIBRARIES(pintrace "${PIN_LOCATION}/intel64/runtime/pincrt/crtendS.o")

TARGET_INCLUDE_DIRECTORIES(pintrace SYSTEM PRIVATE 
	${PIN_LOCATION}/extras/stlport/include/
	${PIN_LOCATION}/extras/libstdc++/include/
	${PIN_LOCATION}/extras/crt/include
	${PIN_LOCATION}/extras/crt/include/arch-x86_64
	${PIN_LOCATION}/extras/crt/include/kernel/uapi
	${PIN_LOCATION}/extras/crt/include/kernel/uapi/asm-x86
)

TARGET_INCLUDE_DIRECTORIES(pintrace PRIVATE
	${PIN_LOCATION}/extras/components/include
	${PIN_LOCATION}/extras/xed-intel64/include/xed
)
