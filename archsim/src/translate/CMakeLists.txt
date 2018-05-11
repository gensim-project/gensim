archsim_add_sources(
	jit_funs.cpp
	TranslationManager.cpp
	TranslationContext.cpp
	TranslationWorkUnit.cpp
	TranslationEngine.cpp
	TranslationCache.cpp
)

ADD_SUBDIRECTORY(profile)
ADD_SUBDIRECTORY(interrupts)
ADD_SUBDIRECTORY(llvm)
