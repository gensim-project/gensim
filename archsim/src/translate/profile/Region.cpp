#include "translate/Translation.h"
#include "translate/profile/Region.h"
#include "translate/profile/Block.h"
#include "gensim/gensim_processor.h"
#include "util/LogContext.h"

#include <mutex>

UseLogContext(LogTranslate);
UseLogContext(LogLifetime);

using namespace archsim::translate::profile;

Region::Region(TranslationManager& mgr, phys_addr_t phys_base_addr)
	:	mgr(mgr),
	  phys_base_addr(phys_base_addr),
	  current_generation(0),
	  max_generation(0),
	  status(NotInTranslation),
	  invalid(false),
	  txln(NULL)
{
//	fprintf(stderr, "*** Region Create %p 0x%08x\n", this, phys_base_addr);
}

Region::~Region()
{
	LC_DEBUG2(LogLifetime) << "Deleting Region Object: " << *this;

//	fprintf(stderr, "*** Region Delete %p 0x%08x\n", this, phys_base_addr);

	if(txln)mgr.RegisterTranslationForGC(*txln);
}

Block& Region::GetBlock(virt_addr_t virt_addr, uint8_t isa_mode)
{
	addr_t offset = RegionArch::PageOffsetOf(virt_addr);

	Block *&block = blocks[offset];
	if (UNLIKELY(!block)) {
		block = block_zone.Construct(*this, offset, isa_mode);
	}

	assert(block->GetISAMode() == isa_mode);
	return *block;
}

void Region::EraseBlock(virt_addr_t virt_addr)
{
	addr_t offset = RegionArch::PageOffsetOf(virt_addr);
	blocks.erase(offset);
}

bool Region::HasTranslations() const
{
	return (GetStatus() == InTranslation) || txln;
}

void Region::TraceBlock(gensim::Processor& cpu, virt_addr_t virt_addr)
{
	if (status == InTranslation)
		return;

	virtual_images.insert(RegionArch::PageBaseOf(virt_addr));
	block_interp_count[RegionArch::PageOffsetOf(virt_addr)]++;
	total_interp_count++;

	mgr.TraceBlock(cpu, GetBlock(virt_addr, cpu.state.isa_mode));
}

void Region::Invalidate()
{
	invalid = true;
}

size_t Region::GetApproximateMemoryUsage() const
{
	size_t size = sizeof(*this);

	for(auto i : virtual_images) size += sizeof(i);
	for(auto i : block_interp_count) size += sizeof(i);
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

			std::ostream& operator<< (std::ostream& out, Region& rgn)
			{
				out << "[Region " << std::hex << rgn.phys_base_addr << "(" << &rgn << "), generation=" << std::dec << rgn.current_generation << "/" << rgn.max_generation;

				if (rgn.invalid)
					out << " INVALID";

//	out << ", Heat = " << rgn.TotalBlockHeat() << "/" << rgn.TotalRegionHeat();

				out << "]";
				return out;
			}

		}
	}
}
