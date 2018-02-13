# TODO: improve this. Gensim interface is a bit of a nightmare.

function(define_model directory model-name arch-name)
	SET(DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/output/${model-name}.dll")
	
	SET(gensim-binary "$<TARGET_FILE:gensim>")
	
	# TODO: glob files in the model source directory
	SET(model-files "")
	
	# Some sort of weird bug with cmake if we try to get the interface
	# include directories directory. 
	GET_PROPERTY(libtrace-includes GLOBAL PROPERTY LIBTRACE_INCLUDES)
	GET_PROPERTY(archsim-includes GLOBAL PROPERTY ARCHSIM_INCLUDES)
	
	SET(gensim-options "-s decode,disasm,adapt_blkjit,jit,jumpinfo,makefile -o makefile.libtrace_path=${libtrace-includes},makefile.archsim_path=${archsim-includes},makefile.Optimise=0")
	
	
	ADD_CUSTOM_COMMAND(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${directory}/output"
		COMMAND "sh" "-c" "mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${directory}/output"
		COMMAND "sh" "-c" "${gensim-binary} -a ${CMAKE_CURRENT_SOURCE_DIR}/${directory}/${model-name}.ac ${gensim-options} -t ${CMAKE_CURRENT_BINARY_DIR}/${directory}/output/"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${directory}"
		DEPENDS gensim
		DEPENDS trace
		DEPENDS ${model-files}
		COMMENT "Generating ${model-name} model"
	)
	
	ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${directory}/output/${arch-name}.dll
		COMMAND "sh" "-c" "make -C ${CMAKE_CURRENT_BINARY_DIR}/${directory}/output"
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${directory}/output"
		COMMENT "Compiling ${model-name} model"
	)
	
	ADD_CUSTOM_TARGET(
		model-${model-name}-built
		ALL
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${directory}/output/${arch-name}.dll
	)
endfunction()

define_model(armv7 armv7a armv7a)
define_model(risc-v riscv riscv)
