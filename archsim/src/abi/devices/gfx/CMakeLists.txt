archsim_add_sources(
	VirtualScreen.cpp 
	VirtualScreenManager.cpp
	
	VNCScreen.cpp
	FBScreen.cpp
)


FIND_PACKAGE(PkgConfig)



IF(PkgConfig_FOUND)
	PKG_CHECK_MODULES(GTK3 gtk+-3.0)
ENDIF()

FIND_PACKAGE(SDL2)

IF(SDL2_FOUND)
	archsim_add_sources(SDLScreen.cpp)
	MESSAGE(STATUS "SDL2 Found - including SDL Screen")
ELSE()
	MESSAGE(STATUS "SDL2 NOT FOUND")
ENDIF()

IF(GTK3_FOUND)
	archsim_add_sources(GtkScreen.cpp)
	MESSAGE(STATUS "GTK Found - including GTKScreen")
ELSE()
	MESSAGE(STATUS "GTK NOT FOUND")
ENDIF()
