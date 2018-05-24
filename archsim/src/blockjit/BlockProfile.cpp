/*
 * BlockProfile.cpp
 *
 *  Created on: 29 Sep 2015
 *      Author: harry
 */

#include <vector>

#include "blockjit/BlockProfile.h"
#include "util/NTZero.h"
#include "util/LogContext.h"

#include <fstream>

//TODO: this should probably have a parent
DeclareLogContext(LogBlockProfile, "BlockProfile");

using captive::shared::block_txln_fn;

using namespace archsim::blockjit;

bool BlockTranslation::FeaturesValid(const archsim::ProcessorFeatureSet& features) const
{
	if(_features_required == nullptr) return true;

	for(auto i = _features_required->begin(); i != _features_required->end(); ++i) {
		if(features.GetFeatureLevel(i->first) != i->second) return false;
	}

	return true;
}

void BlockTranslation::Dump(const std::string &filename)
{
	std::ofstream f(filename);
	f.write((char*)_fn, GetSize());
}


BlockPageProfile::BlockPageProfile(wulib::MemAllocator &allocator) : _allocator(allocator)
{
	_valid = false;
	_dirty = false;
	for(auto &i : _table) i = nullptr;
}

BlockPageProfile::~BlockPageProfile()
{

}

void BlockPageProfile::Insert(Address address, const BlockTranslation &txln)
{
	// We shouldn't be translating any code on a page which is dirty
	if(_dirty) Invalidate();

	if(!_valid) {

		for(auto &i : _table) {
			delete i;
			i = nullptr;
		}

		_valid = true;
	}

	// Get the page offset of the address
	uint32_t address_offset = address.GetPageOffset();

	// XXX ARM HAX
	uint32_t index = address_offset >> kInstructionAlignment;

	assert(Get(address).GetFn() == nullptr);
	_txlns.insert(txln.GetFn());
	Get(address) = txln;
}

void BlockPageProfile::InvalidateTxln(Address address)
{
	auto &txln = Get(address);

	auto fn = txln.GetFn();
	if(fn) {
		_allocator.Free((void*)fn);
		_txlns.erase(fn);
	}

	txln.Invalidate();
}

void BlockPageProfile::Invalidate()
{
	_valid = false;
	_dirty = false;
	for(auto i : _txlns) {
		assert(i != nullptr);
		_allocator.Free((void*)i);
	}

	for(auto &i : _table) {
		if(i) delete i;
		i = nullptr;
	}
	_txlns.clear();
}

BlockProfile::BlockProfile(wulib::MemAllocator &allocator) : _allocator(allocator)
{
	_table_pages_dirty.set();
	for(auto &i : _page_profiles) {
		i = nullptr;
	}
	Invalidate();
}

void BlockProfile::Insert(Address address, const BlockTranslation &txln)
{
	// Get the page index of the address
	uint32_t index = address.GetPageIndex();

	LC_DEBUG2(LogBlockProfile) << "Inserting " << std::hex << address.Get() << " into the block profile";
	getProfile(address).Insert(address, txln);

}

void BlockProfile::Invalidate()
{
	LC_DEBUG1(LogBlockProfile) << "Performing a full invalidation";
	for(auto &i : _page_profiles) {
		if(i != nullptr) {
			i->Invalidate();
		}
	}

	_table_pages_dirty.reset();
}

void BlockProfile::InvalidatePage(Address address)
{
	getProfile(address).Invalidate();
}

void BlockProfile::GarbageCollect()
{
	if(_dirty_pages.size()) {
		LC_DEBUG1(LogBlockProfile) << "Performing a garbage collection";
	}
	for(auto &i : _dirty_pages) {
		i.second->Invalidate();
	}

	_dirty_pages.clear();
}