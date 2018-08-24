/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/ArchInterface.h"
#include <sstream>

using namespace libtrace;

ArchInterface::~ArchInterface()
{

}

DefaultArchInterface::~DefaultArchInterface()
{
}


std::string DefaultArchInterface::DisassembleInstruction(const InstructionCodeRecord &record)
{
	return "???";
}

std::string DefaultArchInterface::GetRegisterSlotName(int idx)
{
	std::stringstream str;
	str << idx;
	return str.str();
}
std::string DefaultArchInterface::GetRegisterBankName(int idx)
{
	std::stringstream str;
	str << idx;
	return str.str();
}

uint32_t DefaultArchInterface::GetRegisterSlotWidth(int idx)
{
	return 0;
}
uint32_t DefaultArchInterface::GetRegisterBankWidth(int idx)
{
	return 0;
}
