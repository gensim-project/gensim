TARGET_ADD_SOURCES(gensim-lib
	ArchComponents.cpp
	ArchDescription.cpp
	ArchFeatures.cpp
	RegisterFile.cpp
	RegisterFileDescriptor.cpp

	ArchDescriptionParser.cpp
	MemoryInterfaceDescription.cpp
)

ADD_SUBDIRECTORY(testing)
