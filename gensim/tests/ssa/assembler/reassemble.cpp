/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/Parser.h"
#include "genC/ssa/analysis/LoopAnalysis.h"
#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/validation/SSAStatementValidationPass.h"

#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/Disassemblers.h"
#include "genC/ssa/io/AssemblyReader.h"

using namespace gensim::genc::ssa::testing;
