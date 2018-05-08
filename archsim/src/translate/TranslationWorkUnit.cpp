#include "translate/TranslationWorkUnit.h"
#include "translate/profile/Block.h"
#include "translate/profile/Region.h"
#include "translate/interrupts/InterruptCheckingScheme.h"

#include "gensim/gensim_decode.h"
#include "gensim/gensim_translate.h"

#include "util/LogContext.h"

#include <stdio.h>

UseLogContext(LogTranslate);

using namespace archsim::translate;
using namespace archsim::translate::interrupts;

TranslationWorkUnit::TranslationWorkUnit(archsim::core::thread::ThreadInstance *thread, profile::Region& region, uint32_t generation, uint32_t weight) : thread(thread), region(region), generation(generation), weight(weight), emit_trace_calls(thread->GetTraceSource() != nullptr)
{
	region.Acquire();
}

TranslationWorkUnit::~TranslationWorkUnit()
{
	for(auto a : blocks) delete a.second;

	region.Release();
}

void TranslationWorkUnit::DumpGraph()
{
	std::stringstream s;
	s << "region-" << std::hex << GetRegion().GetPhysicalBaseAddress() << ".dot";

	FILE *f = fopen(s.str().c_str(), "wt");
	fprintf(f, "digraph a {\n");

	for (auto block : blocks) {
		fprintf(f, "B_%08x [color=%s,shape=Mrecord]\n", block.first, block.second->IsEntryBlock() ? "red" : "black");

		if (block.second->IsInterruptCheck())
			fprintf(f, "B_%08x [fillcolor=yellow,style=filled]\n", block.second->GetOffset());

		for (auto succ : block.second->GetSuccessors()) {
			fprintf(f, "B_%08x -> B_%08x\n", block.first, succ->GetOffset());
		}
	}

	fprintf(f, "}\n");
	fclose(f);
}

TranslationBlockUnit *TranslationWorkUnit::AddBlock(profile::Block& block, bool entry)
{
	auto tbu = new TranslationBlockUnit(*this, block.GetOffset(), block.GetISAMode(), entry);
	blocks[block.GetOffset()] = tbu;
	return tbu;
}

TranslationWorkUnit *TranslationWorkUnit::Build(archsim::core::thread::ThreadInstance *thread, profile::Region& region, InterruptCheckingScheme& ics, uint32_t weight)
{
	TranslationWorkUnit *twu = new TranslationWorkUnit(thread, region, region.GetMaxGeneration(), weight);

	twu->potential_virtual_bases.insert(region.virtual_images.begin(), region.virtual_images.end());

	host_addr_t guest_page_data;
//	thread->GetEmulationModel().GetMemoryModel().LockRegion(region.GetPhysicalBaseAddress(), profile::RegionArch::PageSize, guest_page_data);

	for (auto block : region.blocks) {
		auto tbu = twu->AddBlock(*block.second, block.second->IsRootBlock());

		bool end_of_block = false;
		addr_t offset = tbu->GetOffset();
		uint32_t insn_count = 0;

		while (!end_of_block && offset < profile::RegionArch::PageSize) {
			gensim::BaseDecode *decode = thread->GetArch().GetISA(block.second->GetISAMode()).GetNewDecode();

			UNIMPLEMENTED;
			
//			thread->GetArch().GetISA(block.second->GetISAMode()).DecodeInstr(region.GetPhysicalBaseAddress() + offset, );
			
			uint32_t data;
			if (block.second->GetISAMode() == 1) {
				data = *(uint16_t *)((uint8_t *)guest_page_data + offset);
				data <<= 16;
				data |= *(uint16_t *)((uint8_t *)guest_page_data + offset + 2);
			} else {
				data = *(uint32_t *)((uint8_t *)guest_page_data + offset);
			}

//			cpu.DecodeInstrIr(data, block.second->GetISAMode(), *decode);

			if(decode->Instr_Code == (uint16_t)(-1)) {
				LC_WARNING(LogTranslate) << "Invalid Instruction at " << std::hex << (uint32_t)(region.GetPhysicalBaseAddress() + offset) <<  ", ir=" << decode->ir << ", isa mode=" << (uint32_t)block.second->GetISAMode() << " whilst building " << *twu;
				delete decode;
				delete twu;
				return NULL;
			}

			tbu->AddInstruction(decode, offset);

			offset += decode->Instr_Length;
			end_of_block = decode->GetEndOfBlock();
			insn_count++;
		}

		if (!end_of_block) {
			tbu->SetSpanning(true);
		}
	}

//	cpu.GetEmulationModel().GetMemoryModel().UnlockRegion(region.GetPhysicalBaseAddress(), profile::RegionArch::PageSize, guest_page_data);

	for (auto block : region.blocks) {
		if (!twu->ContainsBlock(block.second->GetOffset()))
			continue;

		auto tbu = twu->blocks[block.first];
		for (auto succ : block.second->GetSuccessors()) {
			if (!twu->ContainsBlock(succ->GetOffset()))
				continue;

			tbu->AddSuccessor(twu->blocks[succ->GetOffset()]);
		}
	}

	ics.ApplyInterruptChecks(twu->blocks);

//#define COUNT_STATIC_CHECKS
#ifdef COUNT_STATIC_CHECKS
	int checks = 0;
	for (auto block : twu->blocks) {
		if (block.second->IsInterruptCheck()) checks++;
	}
	fprintf(stderr, "placed: %d\n", checks);
#endif

	region.IncrementGeneration();

	return twu;
}

TranslationBlockUnit::TranslationBlockUnit(TranslationWorkUnit& twu, addr_t offset, uint8_t isa_mode, bool entry)
	: twu(twu),
	  offset(offset),
	  isa_mode(isa_mode),
	  entry(entry),
	  interrupt_check(false),
	  spanning(false)
{

}

TranslationBlockUnit::~TranslationBlockUnit()
{

}

TranslationInstructionUnit *TranslationBlockUnit::AddInstruction(gensim::BaseDecode* decode, addr_t offset)
{
	assert(decode);
	auto tiu = twu.GetInstructionZone().Construct(decode, offset);
	instructions.push_back(tiu);
	return tiu;
}

void TranslationBlockUnit::GetCtrlFlowInfo(bool &direct_jump, bool &indirect_jump, int32_t &direct_offset, int32_t &fallthrough_offset) const
{
	uint32_t direct_target;

	auto jumpinfo = twu.GetThread()->GetArch().GetISA(twu.GetThread()->GetModeID()).GetNewJumpInfo();
	jumpinfo->GetJumpInfo(&GetLastInstruction().GetDecode(), 0, indirect_jump, direct_jump, direct_target);
	delete jumpinfo;

	direct_offset = (int32_t)direct_target;
	fallthrough_offset = GetLastInstruction().GetOffset() + GetLastInstruction().GetDecode().Instr_Length;
}

TranslationInstructionUnit::TranslationInstructionUnit(gensim::BaseDecode* decode, addr_t offset) : decode(decode), offset(offset)
{

}

TranslationInstructionUnit::~TranslationInstructionUnit()
{
	delete decode;
}

namespace archsim
{
	namespace translate
	{

		std::ostream& operator<< (std::ostream& out, TranslationWorkUnit& twu)
		{
			out << "[TWU weight=" << std::dec << twu.weight << ", generation=" << twu.generation << ", " << twu.region << "]";
			return out;
		}

	}
}
