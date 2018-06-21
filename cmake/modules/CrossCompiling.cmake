# Functions for cross compiling files for inclusion in a native binary
# This is useful for bootloaders, helper functions, etc.

function(cross_compile_try_sysroot sysroot)
	MESSAGE(STATUS "Looking for a cross-compiler sysroot: ${sysroot}")
	
	find_file(
		SYSROOT
		"include/stdio.h"
		PATHS /usr/${sysroot}
		PATHS /usr/${sysroot}
		NO_DEFAULT_PATH
		NO_CMAKE_ENVIRONMENT_PATH
		NO_SYSTEM_ENVIRONMENT_PATH
		NO_CMAKE_SYSTEM_PATH
		NO_CMAKE_FIND_ROOT_PATH
	)
	
	string(REGEX REPLACE "include/stdio.h" "" SYSROOT ${SYSROOT})
	
	MESSAGE(STATUS "Found the sysroot at : ${SYSROOT}")
	
	SET(SYSROOT ${SYSROOT} PARENT_SCOPE)
endfunction()

function(cross_compile_try_compiler prefix)
	execute_process(
		COMMAND "sh" "-c" "which ${prefix}gcc"
		RESULT_VARIABLE COMPILE_RESULT
		OUTPUT_QUIET
		ERROR_QUIET
	)
	
	if(COMPILE_RESULT EQUAL 0)
		SET(COMPILER_FOUND 1 PARENT_SCOPE)
	else()
		UNSET(COMPILER_FOUND PARENT_SCOPE)
	endif()
endfunction()

function(cross_compile_try_arch_prefix arch prefix)
	cross_compile_try_compiler(${prefix})
	IF(COMPILER_FOUND)
		SET(CC_PREFIX_${arch} "${prefix}" CACHE STRING "Cross compiler prefix for ${arch}")
	ENDIF()
endfunction()

# Lookup cross compiler prefix for given architecture
function(cross_compile_prefix arch outvar)

	if(CC_PREFIX_${arch})
		SET(${outvar} ${CC_PREFIX_${arch}} PARENT_SCOPE)
		return()
	endif()

	MESSAGE(STATUS "Looking for a compiler prefix for ${arch}...")

#	arm-linux-gnu is for building kernels, not user space programs, so remove that one from the list
	cross_compile_try_arch_prefix("${arch}" "${arch}-linux-gnu-")

	cross_compile_try_arch_prefix("${arch}" "${arch}-linux-gnueabi-")
	cross_compile_try_arch_prefix("${arch}" "${arch}-unknown-linux-gnueabi-")
	cross_compile_try_arch_prefix("${arch}" "${arch}-none-eabi-")
	
	IF(CC_PREFIX_${arch})
		MESSAGE(STATUS "Found ${arch} cross compiler prefix: ${CC_PREFIX_${arch}}")
		SET(${outvar} ${CC_PREFIX_${arch}} PARENT_SCOPE)
	ELSE()
		MESSAGE(SEND_ERROR "Failed to find a cross compiler for ${arch}")
	ENDIF()
	
endfunction()

# Try and figure out the actual path to a given cross compiler
function(cross_compile_find ccprefix outvar)
	MESSAGE(STATUS "Looking for compiler for ${ccprefix}")
	execute_process(
		COMMAND "sh" "-c" "which ${ccprefix}gcc"
		OUTPUT_VARIABLE ccprefix
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	MESSAGE(STATUS "Found compiler ${ccprefix}")
	SET(${outvar} ${ccprefix} PARENT_SCOPE)
endfunction()

# Try and figure out the path to a given cross compiler by architecture
function(cross_compile_find_arch arch outvar)
	MESSAGE(STATUS "Looking for CC for ${arch}")
	cross_compile_prefix("${arch}" PREFIX)
	MESSAGE(STATUS "Got prefix ${PREFIX}")
	cross_compile_find("${PREFIX}" CCPATH)
	SET(${outvar} ${CCPATH} PARENT_SCOPE)
endfunction()

# Cross compile the given source file for the given architecture and
# objcopy it to a .bin file
function(cross_compile_bin ARCHITECTURE FLAGS SOURCE_FILE SYMBOL_NAME)

	string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${SOURCE_FILE})
	set(FILE_STEM ${CMAKE_MATCH_1})

	# Get the cross compiler prefix for the given architecture
	cross_compile_prefix(${ARCHITECTURE} CCPREFIX)

	SET(CC_OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_FILE}.processed.S)

	# Build the cross compilation command string
	SET(CC_ASSEMBLE_COMMAND "${CCPREFIX}gcc ${FLAGS} -pipe -c ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE} -o ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_FILE}.o")
	SET(CC_OBJCOPY_FROM "${CCPREFIX}objcopy -O binary -j .text -S -g ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_FILE}.o ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_FILE}.o.bin")
	SET(CC_HEXDUMP "hexdump -v -e '1/1 \".byte 0x%0x\\n\"' ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_FILE}.o.bin >> ${CC_OUTPUT_FILE}")

	ADD_CUSTOM_COMMAND(
		OUTPUT ${CC_OUTPUT_FILE}
		COMMAND "sh" "-c" "${CC_ASSEMBLE_COMMAND}"
		COMMAND "sh" "-c" "${CC_OBJCOPY_FROM}"
		COMMAND "sh" "-c" "echo .globl ${SYMBOL_NAME}_start > ${CC_OUTPUT_FILE}"
		COMMAND "sh" "-c" "echo .globl ${SYMBOL_NAME}_end >> ${CC_OUTPUT_FILE}"
		COMMAND "sh" "-c" "echo .globl ${SYMBOL_NAME}_size >> ${CC_OUTPUT_FILE}"
		COMMAND "sh" "-c" "echo ${SYMBOL_NAME}_start: >> ${CC_OUTPUT_FILE}"
		COMMAND "sh" "-c" "echo .incbin \\\"${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_FILE}.o.bin\\\"" >> ${CC_OUTPUT_FILE}
		COMMAND "sh" "-c" "echo ${SYMBOL_NAME}_end: .word 0 >> ${CC_OUTPUT_FILE}"
		COMMAND "sh" "-c" "echo ${SYMBOL_NAME}_size: .word ${SYMBOL_NAME}_end - ${SYMBOL_NAME}_start >> ${CC_OUTPUT_FILE}"
		DEPENDS ${CC_SOURCE_FILE} 
		COMMENT "Repacking ${SOURCE_FILE}"
		VERBATIM
	)
	ADD_CUSTOM_TARGET(generate_${SYMBOL_NAME} DEPENDS ${CC_OUTPUT_FILE})

	LIST(APPEND ARCHSIM_EXTRA_SOURCES ${CC_OUTPUT_FILE})
	SET(ARCHSIM_EXTRA_SOURCES ${ARCHSIM_EXTRA_SOURCES} PARENT_SCOPE)

	LIST(APPEND ARCHSIM_EXTRA_TARGETS generate_${SYMBOL_NAME})
	SET(ARCHSIM_EXTRA_TARGETS ${ARCHSIM_EXTRA_TARGETS} PARENT_SCOPE)
endfunction()


# Cross compile a complete executable for the given architecture
function(cross_compile_binary ARCHITECTURE TARGET_NAME FLAGS)
	cross_compile_prefix(${ARCHITECTURE} CCPREFIX)
	
	SET(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}")
	SET(SOURCES "${ARGN}")
	
	cross_compile_try_sysroot(arm-linux-gnu)
	
	ADD_CUSTOM_COMMAND(
		OUTPUT ${OUTPUT_FILE}
		COMMAND "sh" "-c" "${CCPREFIX}gcc -o ${OUTPUT_FILE} --sysroot=${SYSROOT} -I${SYSROOT}/include -L${SYSROOT}/lib ${FLAGS} ${SOURCES}"
		DEPENDS "${SOURCES}"
		COMMENT "Cross compiling ${TARGET_NAME}" 
	)
	
	ADD_CUSTOM_TARGET(
		${TARGET_NAME} ALL
		DEPENDS ${OUTPUT_FILE}
	)
	
	SET_TARGET_PROPERTIES(
		${TARGET_NAME} 
		PROPERTIES
			CC_BINARY ${OUTPUT_FILE}
	)
endfunction()
