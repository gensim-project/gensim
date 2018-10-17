archsim_add_sources(
	gensim_decode_context.cpp

	gensim_translate.cpp
)

IF("${ARCHSIM_SIMULATION_HOST}" STREQUAL "x86_64")
	archsim_add_sources(gensim_processor_api_x86_64.S)
ELSE()
	archsim_add_sources(gensim_processor_api_generic.cpp)
ENDIF()
