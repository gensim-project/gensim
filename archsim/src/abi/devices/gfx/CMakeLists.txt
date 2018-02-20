archsim_add_sources(
	VirtualScreen.cpp 
	VirtualScreenManager.cpp
)

FIND_PACKAGE(SDL2)
FIND_PACKAGE(GTK2)

IF(SDL2_FOUND)
	archsim_add_sources(SDLScreen.cpp)
	MESSAGE(STATUS "SDL2 Found - including SDL Screen")
ELSE()
	MESSAGE(STATUS "SDL2 NOT FOUND")
ENDIF()

IF(GTK2_FOUND)
	archsim_add_sources(GtkScreen.cpp)
	MESSAGE(STATUS "GTK Found - including GTKScreen")
ELSE()
	MESSAGE(STATUS "GTK NOT FOUND")
ENDIF()
