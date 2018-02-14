ADD_LIBRARY(gensim-grammar SHARED)

SET(ANTLR_JAR "${CMAKE_CURRENT_BINARY_DIR}/antlr-3.4-complete.jar")

# Most distro versions of antlr don't generate C targets properly
# so we need to download a specific version
IF(NOT EXISTS ${ANTLR_JAR})
	execute_process(
		COMMAND "sh" "-c" "wget http://www.antlr3.org/download/antlr-3.4-complete.jar"
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(gensim-grammar PUBLIC ${ANTLR_INCLUDE_DIR})

function(grammar grammar-name)
	ADD_CUSTOM_COMMAND(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Lexer.c"
		       "${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Lexer.h"
		       "${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Parser.c"
		       "${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Parser.h"
		COMMAND "sh" "-c" "java -jar ${ANTLR_JAR} -o ${CMAKE_CURRENT_BINARY_DIR}/${grammar-name} ${CMAKE_CURRENT_SOURCE_DIR}/${grammar-name}.g"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${grammar-name}.g"
		COMMENT  "Building grammar ${grammar-name}.g"
	)
	
	SET_SOURCE_FILES_PROPERTIES(
			"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Lexer.c"
			"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Lexer.h"
			"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Parser.c"
			"${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Parser.h"
		PROPERTIES
			GENERATED 1
	)
	
	TARGET_SOURCES(gensim-grammar PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Lexer.c" "${CMAKE_CURRENT_BINARY_DIR}/${grammar-name}/${grammar-name}Parser.c")
	TARGET_INCLUDE_DIRECTORIES(gensim-grammar INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

grammar(archCBehaviour)
grammar(archcasm)
grammar(genC)
grammar(archc)
grammar(archCuArch)
grammar(ssa_asm)
