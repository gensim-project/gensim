TARGET_ADD_SOURCES(gensim-lib
	AsmDescription.cpp
	AsmMapDescription.cpp
	FieldDescription.cpp
	HelperFnDescription.cpp
	InstructionDescription.cpp
	InstructionFormatDescription.cpp
	ISADescription.cpp


	AsmDescriptionParser.cpp	
	AsmMapDescriptionParser.cpp	
	ISADescriptionParser.cpp
	InstructionDescriptionParser.cpp
	HelperFnDescriptionParser.cpp
)

ADD_SUBDIRECTORY(testing)
