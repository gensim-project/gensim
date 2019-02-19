/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/Translation.h"
#include "translate/TranslationManager.h"
#include "translate/profile/Region.h"
#include "translate/profile/Block.h"
#include "util/LogContext.h"
#include "abi/Address.h"

#include <mutex>

UseLogContext(LogTranslate);
UseLogContext(LogLifetime);

using namespace archsim::translate::profile;

Region::Region(TranslationManager& mgr, Address phys_base_addr)
	:	mgr(mgr),
	  phys_base_addr(phys_base_addr),
	  current_generation(0),
	  total_interp_count_(0),
	  max_generation(0),
	  status(NotInTranslation),
	  invalid_(false),
	  txln(NULL),
	  max_block_interp_count_(0)
{

}

Region::~Region()
{
	LC_DEBUG2(LogLifetime) << "Deleting Region Object: " << *this;

	if(txln) {
		mgr.RegisterTranslationForGC(*txln);
	}
}

void Region::dump()
{
	std::cerr << *this;
}

void Region::dump_dot()
{
	std::cerr << "graph {" << std::endl;;

	for(auto i : blocks) {
		std::cerr << "block_" << std::hex << i.first << ";" << std::endl;
	}

	for(auto i : blocks) {
		for(auto j : i.second->GetSuccessors()) {
			std::cerr << "block_" << std::hex << i.first << " -> block_" << std::hex << j->GetOffset() << ";" << std::endl;
		}
	}

	std::cerr << "}" << std::endl;;
}


Block& Region::GetBlock(Address virt_addr, uint8_t isa_mode)
{
	Address offset = virt_addr.PageOffset();

	Block *&block = blocks[offset];
	if (UNLIKELY(!block)) {
		block = block_zone.Construct(*this, offset, isa_mode);
	}

	assert(block->GetISAMode() == isa_mode);
	return *block;
}

void Region::EraseBlock(Address virt_addr)
{
	Address offset = virt_addr.PageOffset();
	blocks.erase(offset);
}

void Region::InvalidateHeat()
{
	for(auto block : blocks) {
		block.second->ClearInterpCount();
	}
	total_interp_count_ = 0;
	max_block_interp_count_ = 0;
}

bool Region::HasTranslations() const
{
	return (GetStatus() != NotInTranslation) || txln;
}

void Region::TraceBlock(archsim::core::thread::ThreadInstance *thread, Address virt_addr)
{
	total_interp_count_++;

	if (GetStatus() == InTranslation)
		return;

	virtual_images.insert(virt_addr.PageBase());

	auto &block = GetBlock(virt_addr, thread->GetModeID());
	block.IncrementInterpCount();
	if(block.GetInterpCount() > max_block_interp_count_) {
		max_block_interp_count_ = block.GetInterpCount();
	}

	mgr.TraceBlock(thread, block);
}

void Region::Invalidate()
{
	invalid_ = true;
}

size_t Region::GetApproximateMemoryUsage() const
{
	size_t size = sizeof(*this);

	for(auto i : virtual_images) size += sizeof(i);
	for(auto i : blocks) size += sizeof(i);
	size += block_zone.GetAllocatedSize();

	return size;
}

namespace archsim
{
	namespace translate
	{
		namespace profile
		{

			std::ostream& operator<< (std::ostream& out, const Region& rgn)
			{
				// todo: fix lookup of Address::operator<<
				out << "[Region " << std::hex << rgn.phys_base_addr.Get() << "(" << &rgn << "), generation=" << std::dec << rgn.current_generation << "/" << rgn.max_generation;

				if (rgn.invalid_)
					out << " INVALID";

//	out << ", Heat = " << rgn.TotalBlockHeat() << "/" << rgn.TotalRegionHeat();

				out << "]";
				return out;
			}

		}
	}
}
