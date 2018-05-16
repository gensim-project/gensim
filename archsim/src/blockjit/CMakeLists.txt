archsim_add_sources(
	encode.cpp 
	ir-sorter.cpp
	translation-context.cpp
	BlockJitTranslate.cpp
	BlockProfile.cpp
	blockjit-funs.cpp
	PerfMap.cpp
	BlockCache.cpp
	IRPrinter.cpp

	blockjit-shunts.S
)

ADD_SUBDIRECTORY(block-compiler)
