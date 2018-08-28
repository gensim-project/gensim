archsim_add_sources(
	jit_funs.cpp
	TranslationManager.cpp
	TranslationContext.cpp
	TranslationWorkUnit.cpp
	TranslationEngine.cpp
	TranslationCache.cpp
)

ADD_SUBDIRECTORY(adapt)
ADD_SUBDIRECTORY(profile)
ADD_SUBDIRECTORY(interrupts)

IF(ARCHSIM_ENABLE_LLVM)
	archsim_add_sources(
		AsynchronousTranslationManager.cpp
		AsynchronousTranslationWorker.cpp
	)

#ADD_SUBDIRECTORY(llvm)
ENDIF()
