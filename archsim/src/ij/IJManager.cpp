/*
 * ij/IJManager.cpp
 */
#include "ij/IJManager.h"
#include "ij/IJBlock.h"
#include "ij/IJTranslationContext.h"
#include "abi/memory/MemoryModel.h"
#include "gensim/gensim_processor.h"
#include "gensim/gensim_decode.h"
#include "translate/profile/RegionArch.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"

DeclareLogContext(LogIJ, "IJ");

using namespace archsim::ij;

DefineComponentType(IJManager);
RegisterComponent(IJManager, IJManager, "ij", "Instruction JIT");

IJManager::IJManager()
{

}

IJManager::~IJManager()
{

}

bool IJManager::Initialise()
{
	LC_DEBUG1(LogIJ) << "Initialising Instruction JIT";
	return true;
}

void IJManager::Destroy()
{
}

IJManager::ij_block_fn IJManager::TranslateBlock(gensim::Processor& cpu, phys_addr_t block_phys_addr)
{
	IJBlock block;

	bool end_of_block = false;
	addr_t offset = 0;
	uint32_t insn_count = 0;

	host_addr_t guest_page_data;
	cpu.GetEmulationModel().GetMemoryModel().LockRegion(block_phys_addr, archsim::translate::profile::RegionArch::PageSize, guest_page_data);

	while (!end_of_block && offset < archsim::translate::profile::RegionArch::PageSize) {
		gensim::BaseDecode *decode = cpu.GetNewDecode();

		uint32_t data = *(uint32_t *)((uint8_t *)guest_page_data + offset);
		cpu.DecodeInstrIr(data, 0, *decode);

		if(decode->Instr_Code == (uint16_t)(-1)) {
			LC_WARNING(LogIJ) << "Invalid Instruction at " << std::hex << (uint32_t)(block_phys_addr + offset) <<  ", ir=" << decode->ir << " whilst building IJ block";
			return NULL;
		}

		block.AddInstruction(*decode, offset);

		offset += decode->Instr_Length;
		end_of_block = decode->GetEndOfBlock();
		insn_count++;
	}

	IJTranslationContext ctx(*this, block, cpu);
	return ctx.Translate();
}
