archsim_add_sources(
	encode.cpp 
	ir-sorter.cpp
	block-tramp-source.cpp
	translation-context.cpp
	BlockJitTranslate.cpp
	BlockProfile.cpp
	blockjit-funs.cpp
	PerfMap.cpp
	BlockCache.cpp
	IRInstruction.cpp	

	blockjit-shunts.S
	block-tramp.S
)

ADD_SUBDIRECTORY(block-compiler)
