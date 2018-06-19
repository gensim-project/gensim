#include "abi/memory/MemoryModel.h"
#include "abi/memory/MemoryTranslationModel.h"
#include "abi/memory/FlipperMemoryModel.h"

using namespace archsim::abi::memory;

#if CONFIG_LLVM
class FlipperTranslationModel : public MemoryTranslationModel
{
public:
	FlipperTranslationModel();
	~FlipperTranslationModel();
};

FlipperTranslationModel::FlipperTranslationModel()
{

}

FlipperTranslationModel::~FlipperTranslationModel()
{

}
#endif

FlipperMemoryModel::FlipperMemoryModel(MemoryModel& _lhs, MemoryModel& _rhs, guest_addr_t _split_address)
	: translation_model(nullptr), split_address(_split_address), lhs(_lhs), rhs(_rhs)
{
#if CONFIG_LLVM
	translation_model = new FlipperTranslationModel();
#endif

}

FlipperMemoryModel::~FlipperMemoryModel()
{

}

bool FlipperMemoryModel::Initialise()
{
	bool init = lhs.Initialise() && rhs.Initialise();
	if(init) {
		lhs_map = lhs.GetMappingManager();
		rhs_map = rhs.GetMappingManager();
	}
	return init;
}

MappingManager *FlipperMemoryModel::GetMappingManager()
{
	return this;
}

bool FlipperMemoryModel::MapAll(RegionFlags prot)
{
	assert(false && "invalid operation exception");
	return false;
}

bool FlipperMemoryModel::MapRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot, std::string name)
{
	if ((addr < split_address) && lhs_map)
		return lhs_map->MapRegion(addr, size, prot, name);
	else if (rhs_map)
		return rhs_map->MapRegion(addr, size, prot, name);
	else
		return false;
}

bool FlipperMemoryModel::RemapRegion(guest_addr_t addr, guest_size_t size)
{
	if ((addr < split_address) && lhs_map)
		return lhs_map->RemapRegion(addr, size);
	else if (rhs_map)
		return rhs_map->RemapRegion(addr, size);
	else
		return false;
}

bool FlipperMemoryModel::UnmapRegion(guest_addr_t addr, guest_size_t size)
{
	if ((addr < split_address) && lhs_map)
		return lhs_map->UnmapRegion(addr, size);
	else if (rhs_map)
		return rhs_map->UnmapRegion(addr, size);
	else
		return false;
}

bool FlipperMemoryModel::ProtectRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot)
{
	if ((addr < split_address) && lhs_map)
		return lhs_map->ProtectRegion(addr, size, prot);
	else if (rhs_map)
		return rhs_map->ProtectRegion(addr, size, prot);
	else
		return false;
}

bool FlipperMemoryModel::GetRegionProtection(guest_addr_t addr, RegionFlags& prot)
{
	if ((addr < split_address) && lhs_map)
		return lhs_map->GetRegionProtection(addr, prot);
	else if (rhs_map)
		return rhs_map->GetRegionProtection(addr, prot);
	else
		return false;
}

guest_addr_t FlipperMemoryModel::MapAnonymousRegion(guest_size_t size, RegionFlags prot)
{
	return Address(-1);
}

void FlipperMemoryModel::DumpRegions()
{
	if (lhs_map)
		lhs_map->DumpRegions();

	if (rhs_map)
		rhs_map->DumpRegions();
}

bool FlipperMemoryModel::ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr)
{
	if (!lhs.ResolveGuestAddress(host_addr, guest_addr))
		return rhs.ResolveGuestAddress(host_addr, guest_addr);
	return true;
}

bool FlipperMemoryModel::HandleSegFault(host_const_addr_t host_addr)
{
	if (!lhs.HandleSegFault(host_addr))
		return rhs.HandleSegFault(host_addr);
	return true;
}

MemoryTranslationModel& FlipperMemoryModel::GetTranslationModel()
{
	return *translation_model;
}

uint32_t FlipperMemoryModel::Read(guest_addr_t addr, uint8_t *data, int size)
{
	if (addr < split_address)
		return lhs.Read(addr, data, size);
	else
		return rhs.Read(addr, data, size);
}

uint32_t FlipperMemoryModel::Write(guest_addr_t addr, uint8_t *data, int size)
{
	if (addr < split_address)
		return lhs.Write(addr, data, size);
	else
		return rhs.Write(addr, data, size);
}

uint32_t FlipperMemoryModel::Peek(guest_addr_t addr, uint8_t *data, int size)
{
	if (addr < split_address)
		return lhs.Peek(addr, data, size);
	else
		return rhs.Peek(addr, data, size);
}

uint32_t FlipperMemoryModel::Poke(guest_addr_t addr, uint8_t *data, int size)
{
	if (addr < split_address)
		return lhs.Poke(addr, data, size);
	else
		return rhs.Poke(addr, data, size);
}
