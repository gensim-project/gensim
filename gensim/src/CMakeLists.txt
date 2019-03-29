TARGET_ADD_SOURCES(gensim-lib
	DecodeTree.cpp
	DiagnosticContext.cpp
	Util.cpp
)

ADD_SUBDIRECTORY(arch)
ADD_SUBDIRECTORY(genC)
ADD_SUBDIRECTORY(generators)
ADD_SUBDIRECTORY(grammar)
ADD_SUBDIRECTORY(isa)
ADD_SUBDIRECTORY(util)
