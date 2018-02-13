# Finds the ANTLR 3 C Runtime library

find_path(
	ANTLR_INCLUDE_DIR 
	NAMES antlr3.h
)

find_library(
	ANTLR_LIB 
	NAMES antlr3c
)

set(ANTLR_LIBRARIES ${ANTLR_LIB})
set(ANTLR_INCLUDE_DIRS ${ANTLR_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(ANTLR DEFAULT_MSG ANTLR_LIB ANTLR_INCLUDE_DIR)

mark_as_advanced(ANTLR_INCLUDE_DIR ANTLR_LIB)
