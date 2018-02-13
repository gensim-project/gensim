/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/testing/TestArch.h"
#include "arch/ArchDescription.h"

using namespace gensim::arch;
using namespace gensim::arch::testing;

ArchDescription *gensim::arch::testing::GetTestArch()
{
	ArchDescription *desc = new ArchDescription();

	desc->Name = "TestArch";
	desc->wordsize = 32;
	
	// add some gprs
	RegSpaceDescriptor *rsd = new RegSpaceDescriptor(&desc->GetRegFile(), 64);
	desc->GetRegFile().AddRegisterSpace(rsd);
	desc->GetRegFile().AddRegisterBank(rsd, new RegBankViewDescriptor(rsd, "RB", 0, 16, 4, 1, 4, 4, "uint32"));

	// add some flags
	rsd = new RegSpaceDescriptor(&desc->GetRegFile(), 4);
	desc->GetRegFile().AddRegisterSpace(rsd);
	desc->GetRegFile().AddRegisterSlot(rsd, new RegSlotViewDescriptor(rsd, "C", "uint8", 1, 0));
	desc->GetRegFile().AddRegisterSlot(rsd, new RegSlotViewDescriptor(rsd, "V", "uint8", 1, 1));
	desc->GetRegFile().AddRegisterSlot(rsd, new RegSlotViewDescriptor(rsd, "N", "uint8", 1, 2));
	desc->GetRegFile().AddRegisterSlot(rsd, new RegSlotViewDescriptor(rsd, "Z", "uint8", 1, 3));

	
	
	return desc;
}
