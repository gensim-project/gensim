/*
 * TranslationProvider.cpp
 *
 *  Created on: 17 Jun 2014
 *      Author: harry
 */

#include "abi/memory/TranslationProvider.h"
#include "abi/devices/MMU.h"

using namespace archsim::abi::memory;
using namespace archsim::abi::devices;

TranslationProvider::~TranslationProvider()
{

}

MMUBasedTranslationProvider::MMUBasedTranslationProvider(MMU &mmu) : mmu(mmu) {}

MMUBasedTranslationProvider::~MMUBasedTranslationProvider() {}

uint32_t MMUBasedTranslationProvider::Translate(gensim::Processor *cpu, uint32_t virt_addr, uint32_t &phys_addr, const struct archsim::abi::devices::AccessInfo info)
{
	return (uint32_t)mmu.Translate(cpu, virt_addr, phys_addr, info);
}
