/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "define.h"
#include "isa/testing/TestISA.h"
#include "isa/ISADescription.h"
#include "isa/ISADescriptionParser.h"

using namespace gensim::isa;
using namespace gensim::isa::testing;

ISADescription *gensim::isa::testing::GetTestISA(bool include_instruction)
{
	ISADescription *isa = new ISADescription(0);

	InstructionFormatDescription *ifd;
	gensim::DiagnosticSource diagsrc ("Testing");
	gensim::DiagnosticContext diagctx (diagsrc);
	InstructionFormatDescriptionParser ifdp (diagctx, *isa);

	if(!ifdp.Parse("format", "%field1:8, %field2:8 %field3:8 %field4:8", ifd)) {
		throw std::logic_error("Field parse error");
	}

	isa->AddFormat(ifd);

	if(include_instruction) {
		InstructionDescription *id = new InstructionDescription("test_insn", *isa, ifd);
		id->BehaviourName = "test_instruction";

		isa->AddInstruction(id);
	}

	return isa;
}