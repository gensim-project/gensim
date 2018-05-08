TARGET_ADD_SOURCES(gensim-lib
	ByteByByteDecodeGenerator.cpp
	DecodeGenerator.cpp
	DisasmGenerator.cpp
	FunctionalDecodeGenerator.cpp
	GenerationManager.cpp
	JumpInfoGenerator.cpp
	MakefileGenerator.cpp
	ArchDescriptorGenerator.cpp
	ModuleDescriptorGenerator.cpp

# These are deprecated and should be removed soon
	InterpretiveExecutionEngineGenerator.cpp
	ClangLLVMTranslationGenerator.cpp
	TranslationGenerator.cpp
	ExecutionEngineGenerator.cpp
)

ADD_SUBDIRECTORY(BlockJIT)
ADD_SUBDIRECTORY(Captive)
ADD_SUBDIRECTORY(ExecutionEngine)

# This stuff needs refactoring into something a bit more manageable
ADD_SUBDIRECTORY(GenCInterpreter)
ADD_SUBDIRECTORY(GenCJIT)
