//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Various simulator internal enumerations and types.
//
// =====================================================================

#ifndef INC_SIMTYPES_H_
#define INC_SIMTYPES_H_

#if defined(__GNUC__) && defined(HAVE_MMX)
typedef short v4hi __attribute__((vector_size(8)));
typedef short vect4[4];
#endif

// Format of the object to load.
////
typedef enum {
	OF_ELF,  // in ELF32 binary
	OF_HEX,  // in HEX text
	OF_BIN   // in binary image
} ObjectFormat;

#define MAP_OPERATING_MODE(_mode) ((_mode == 0) ? KERNEL_MODE : USER_MODE)
// Processor operating modes
////
typedef enum {
	KERNEL_MODE = 0,  // highest level of privilege - DEFAULT MODE
	USER_MODE = 1,    // lowest level of privilege and provides limited access to
	// machine state
	NUM_OPERATING_MODES
} OperatingMode;

// Types of interrupts that are possible
////
typedef enum {
	INTERRUPT_EXCEPTION,  // state during exception
	INTERRUPT_L1_IRQ,     // state during L1 interrupt
	INTERRUPT_L2_IRQ,     // state during L2 interrupt
	INTERRUPT_NONE,       // no interrupt occured
	NUM_INTERRUPT_STATES
} InterruptState;

// Trace modes
////
typedef enum {
	kCompilationModeBasicBlock = 0x1,	    // Basic block trace mode
	kCompilationModePageControlFlowGraph = 0x2,  // Page trace mode
} CompilationMode;

// Format string indices for shared library name mappings
////
typedef enum {
	SHLIB_FMT_FUNCTIONAL,
	SHLIB_FMT_CYCLE,
	NUM_SHLIB_FMT
} ShlibNameFormat;

// Supported pipeline variants
////
typedef enum {
	E_PL_EC5,
	E_PL_EC7,
	E_PL_SKIPJACK
} ProcessorPipelineVariant;

// Translation variant
////
typedef enum {
	VARIANT_FUNCTIONAL,
	VARIANT_CYCLE_ACCURATE,
	NUM_TRANSLATION_VARIANT
} TranslationVariant;

// Instruction size constants
/////
typedef enum {
	INST_SIZE_BASE = 0,
	INST_SIZE_16BIT_IR = 1,
	INST_SIZE_HAS_LIMM = 2,
	INST_SIZE_16BIT = INST_SIZE_BASE | INST_SIZE_16BIT_IR,
	INST_SIZE_32BIT = INST_SIZE_BASE,
	INST_SIZE_48BIT = INST_SIZE_16BIT_IR | INST_SIZE_HAS_LIMM,
	INST_SIZE_64BIT = INST_SIZE_BASE | INST_SIZE_HAS_LIMM
} InstSizeType;

#endif  // !defined (INC_SIMTYPES_H_)
