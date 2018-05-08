# TODO: improve this. Gensim interface is a bit of a nightmare.

function(define_model target-name arch-name arch-file)

	SET(DLL_PATH "${CMAKE_CURRENT_BINARY_DIR}/output/${arch-name}.dll")
	SET(OUTPUT_DLL_PATH "${ARCHSIM_MODULE_DIRECTORY}/${arch-name}.dll")

	SET(gensim-binary "$<TARGET_FILE:gensim>")
	SET(model-files ${ARGN})

	# Some sort of weird bug with cmake if we try to get the interface
	# include directories directory.
	GET_PROPERTY(libtrace-includes GLOBAL PROPERTY LIBTRACE_INCLUDES)
	GET_PROPERTY(archsim-includes GLOBAL PROPERTY ARCHSIM_INCLUDES)

	# Figure out correct optimisation level depending on build type
	IF(${CMAKE_BUILD_TYPE} STREQUAL "DEBUG")
		SET(MODEL_OPT "0")
	ELSEIF(${CMAKE_BUILD_TYPE} STREQUAL "DEBUGOPT")
		SET(MODEL_OPT "2")
	ELSEIF(${CMAKE_BUILD_TYPE} STREQUAL "RELEASE")
		SET(MODEL_OPT "3")
	ENDIF()

	#SET(gensim-options -s module,arch,decode,disasm,ee_interp,jumpinfo,makefile -o makefile.libtrace_path=${libtrace-includes},makefile.archsim_path=${archsim-includes},makefile.Optimise=${MODEL_OPT},makefile.Debug=1)

    SET(gensim-options -s module,arch,decode,disasm,ee_blockjit,jumpinfo,makefile -o makefile.libtrace_path=${libtrace-includes},makefile.archsim_path=${archsim-includes},makefile.Optimise=${MODEL_OPT},makefile.Debug=1)

	ADD_CUSTOM_COMMAND(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/output/Makefile"
		COMMAND "sh" "-c" "mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/output"
		COMMAND ${gensim-binary} -a ${CMAKE_CURRENT_SOURCE_DIR}/${arch-file} ${gensim-options} -t ${CMAKE_CURRENT_BINARY_DIR}/output/
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		DEPENDS gensim
		DEPENDS trace
		DEPENDS ${model-files}
		COMMENT "Generating ${target-name}"
	)

	ADD_CUSTOM_COMMAND(
		OUTPUT ${DLL_PATH}
		COMMAND "sh" "-c" "make -C ${CMAKE_CURRENT_BINARY_DIR}/output -j4"
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/output/Makefile"
		COMMENT "Compiling ${target-name}"
	)

	ADD_CUSTOM_COMMAND(
		OUTPUT ${OUTPUT_DLL_PATH}
		COMMAND "sh" "-c" "cp ${DLL_PATH} ${OUTPUT_DLL_PATH}"
		DEPENDS "${DLL_PATH}"
	)

	ADD_CUSTOM_TARGET(
		${target-name} ALL
		DEPENDS ${OUTPUT_DLL_PATH}
	)

	SET_TARGET_PROPERTIES(
		${target-name}
		PROPERTIES
			MODEL_PATH ${DLL_PATH}
	)
endfunction()

ADD_SUBDIRECTORY(armv7)
ADD_SUBDIRECTORY(risc-v)
