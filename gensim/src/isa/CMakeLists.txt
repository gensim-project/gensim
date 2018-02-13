TARGET_ADD_SOURCES(gensim-lib
	AsmDescription.cpp
	AsmMapDescription.cpp
	FieldDescription.cpp
	HelperFnDescription.cpp
	HelperFnDescriptionParser.cpp
	InstructionDescription.cpp
	InstructionFormatDescription.cpp
	ISADescription.cpp
	ISADescriptionParser.cpp
)

ADD_SUBDIRECTORY(testing)
