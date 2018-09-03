include(ExternalProject)

FIND_PACKAGE(LLVM 6.0.0 REQUIRED CONFIG)

GET_PROPERTY(libtrace-includes GLOBAL PROPERTY LIBTRACE_INCLUDES)
GET_PROPERTY(archsim-includes GLOBAL PROPERTY ARCHSIM_INCLUDES)

SET(gensim-options -s module,external_decoder,arch,ee_interp,llvm_translator,jumpinfo,function,makefile -o external_decoder.Filename=X86Decoder.h,external_decoder.Class=archsim::arch::x86::X86Decoder,makefile.libtrace_path=${libtrace-includes},makefile.llvm_path=${LLVM_INCLUDE_DIRS},makefile.archsim_path=${archsim-includes},makefile.Optimise=0,makefile.Debug=1)

build_model(x86 x86 x86.ac "${gensim-options}" behaviours.x86 execute.x86-base isa.ac x86.ac x86-base.ac x86-sse.ac execute.sse execute.x87 x86-x87.ac)

# x86 model also depends on intel XED (defined in Archsim cmakelists)
ADD_DEPENDENCIES(x86 IntelXED)
