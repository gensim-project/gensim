ADD_LIBRARY(gensim-grammar SHARED)

FIND_PACKAGE(BISON REQUIRED)
FIND_PACKAGE(FLEX REQUIRED)

TARGET_INCLUDE_DIRECTORIES(gensim-grammar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

function(grammar_flex_bison grammar-name grammar-path)
	IF(NOT BISON_FOUND)
		MESSAGE(SEND_FATAL "Could not find bison")
		RETURN()
	ENDIF()
	IF(NOT FLEX_FOUND)
		MESSAGE(SEND_FATAL "Could not find bison")
		RETURN()
	ENDIF()

	# Generate lexer and parser from input files
	ADD_CUSTOM_COMMAND(
		OUTPUT 
			"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.l.cpp"
			"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.l.h"
		COMMAND "sh" "-c" "mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/"
		COMMAND "sh" "-c" "cd ${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/" "&&" "sh" "-c" "flex -o ${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.l.cpp --header-file=${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.l.h ${grammar-path}/${grammar-name}.lpp"
		DEPENDS "${grammar-path}/${grammar-name}.lpp"
		COMMENT "Generating lexer for grammar ${grammar-name}"
	)
	ADD_CUSTOM_COMMAND(
		OUTPUT
			"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.y.cpp"
			"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.tabs.h"
		COMMAND	"sh" "-c" "mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/"
		COMMAND "sh" "-c" "cd ${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/" "&&" "sh" "-c" "bison -W -Wconflicts-sr -v --graph=${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.dot -o ${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.y.cpp --defines=${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.tabs.h ${grammar-path}/${grammar-name}.ypp"
		DEPENDS "${grammar-path}/${grammar-name}.ypp"
		COMMENT "Generating parser for grammar ${grammar-name}"
	)

	SET_SOURCE_FILES_PROPERTIES(
		"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.l.cpp"
		"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.l.h"
		"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.tabs.h"
		"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.y.cpp"
		PROPERTIES
			GENERATED 1
	)

	# Add generated files to grammar library
	TARGET_SOURCES(gensim-grammar PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.l.cpp"
		"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}.y.cpp"
	)

	# Add generated headers to library interface
	TARGET_INCLUDE_DIRECTORIES(gensim-grammar PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${grammar-name})

endfunction()

grammar_flex_bison(genC "${CMAKE_CURRENT_SOURCE_DIR}/genC")
grammar_flex_bison(archC "${CMAKE_CURRENT_SOURCE_DIR}/archC")
grammar_flex_bison(archCAsm "${CMAKE_CURRENT_SOURCE_DIR}/archCAsm")
grammar_flex_bison(archCBehaviours "${CMAKE_CURRENT_SOURCE_DIR}/archCBehaviours")
grammar_flex_bison(genCSSA "${CMAKE_CURRENT_SOURCE_DIR}/genCSSA")

