CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(libgvnc)

ADD_LIBRARY(gvnc 
	lib/Client.cpp
	lib/Server.cpp
	lib/Framebuffer.cpp
	lib/Encoder.cpp
)

standard_flags(gvnc)

TARGET_INCLUDE_DIRECTORIES(gvnc PUBLIC inc/)

SET_TARGET_PROPERTIES(gvnc
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_RQUIRED YES
)

