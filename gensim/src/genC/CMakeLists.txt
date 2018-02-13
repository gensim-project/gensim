TARGET_ADD_SOURCES(gensim-lib
	DiagNode.cpp
	ExternalFunctions.cpp
	InstStructBuilder.cpp
	Intrinsics.cpp
	Parser.cpp
)

ADD_SUBDIRECTORY(ir)
ADD_SUBDIRECTORY(ssa)
ADD_SUBDIRECTORY(testing)
