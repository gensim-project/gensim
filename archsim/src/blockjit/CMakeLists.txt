archsim_add_sources(
	ir-sorter.cpp
	translation-context.cpp
	BlockProfile.cpp
	blockjit-funs.cpp
	PerfMap.cpp
	BlockCache.cpp
)

IF(ARCHSIM_SIMULATION_HOST STREQUAL "x86_64")
	ADD_SUBDIRECTORY(block-compiler)

	archsim_add_sources(
		BlockJitTranslate.cpp
		blockjit-shunts.S
	)
ENDIF()
