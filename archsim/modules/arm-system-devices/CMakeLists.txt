PROJECT(archsim-arm-system-devices C CXX)

INCLUDE_DIRECTORIES(inc)

ADD_LIBRARY(arm-system-devices SHARED
	src/GenericInterruptController.cpp
	src/GenericPrimecellDevice.cpp
	src/module.cpp
	src/PL011.cpp
	src/PL031.cpp
	src/PL050.cpp
	src/PL061.cpp
	src/PL080.cpp
	src/PL110.cpp
	src/PL180.cpp
	src/PL190.cpp
	src/PrimecellDevice.cpp
	src/SP804.cpp
	src/SP810.cpp
	src/VersatileSIC.cpp
)

SET_TARGET_PROPERTIES(arm-system-devices
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
)

TARGET_LINK_LIBRARIES(arm-system-devices archsim-interface)
