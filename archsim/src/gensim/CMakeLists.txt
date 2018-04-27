archsim_add_sources(
	gensim_decode_context.cpp
	#gensim_processor_blockjit.cpp
	#gensim_processor.cpp
	#gensim_processor_jit.cpp
	#gensim_tracing.cpp
)

IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
	archsim_add_sources(gensim_processor_api_x86_64.S)
ELSE()
	archsim_add_sources(gensim_processor_api_generic.cpp)
ENDIF()
