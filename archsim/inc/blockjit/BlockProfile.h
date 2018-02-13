/*
 * BlockProfile.h
 *
 *  Created on: 29 Sep 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCKPROFILE_H_
#define INC_BLOCKJIT_BLOCKPROFILE_H_

#include "blockjit/ir.h"
#include "abi/Address.h"
#include "util/MemAllocator.h"
#include "util/LogContext.h"

#include "gensim/ProcessorFeatures.h"

#include <bitset>
#include <map>
#include <unordered_map>
#include <unordered_set>

UseLogContext(LogBlockProfile);

namespace archsim
{

	using captive::shared::block_txln_fn;

	namespace blockjit
	{

		class BlockTranslation
		{
		public:
			BlockTranslation() : _fn(nullptr), _features_required(nullptr) {}

			void AddRequiredFeature(uint32_t feature_id, uint32_t feature_level)
			{
				if(!_features_required) _features_required = new archsim::ProcessorFeatureSet();
				_features_required->AddFeature(feature_id);
				_features_required->SetFeatureLevel(feature_id, feature_level);
			}

			block_txln_fn GetFn() const
			{
				return _fn;
			}
			void SetFn(block_txln_fn fn)
			{
				_fn = fn;
			}

			bool IsValid(const archsim::ProcessorFeatureSet &features)
			{
				if(_fn == nullptr) return false;
				return FeaturesValid(features);
			}
			bool FeaturesValid(const archsim::ProcessorFeatureSet &features) const;

			void Invalidate()
			{
				_fn = nullptr;
				if(_features_required) {
					delete _features_required;
					_features_required = nullptr;
				}
			}

			archsim::ProcessorFeatureSet GetFeatures()
			{
				if(_features_required)
					return *_features_required;
				else return archsim::ProcessorFeatureSet();
			}

		private:
			block_txln_fn _fn;
			archsim::ProcessorFeatureSet *_features_required;
		};

		class BlockPageProfile
		{
		public:
			BlockPageProfile(wulib::MemAllocator &allocator);
			~BlockPageProfile();

			void Insert(Address address, const BlockTranslation &txln);
			void InvalidateTxln(Address address);
			void Invalidate();

			BlockTranslation &Get(Address address)
			{
				return getChunk(address).chunk[(address.GetPageOffset() >> kInstructionAlignment) % kBlocksPerChunk];
			}

			static const size_t kInstructionAlignment = 1;
			static const uint32_t kInstructionSize = 1 << kInstructionAlignment;

			bool IsDirty() const
			{
				return _dirty;
			}
			void MakeDirty()
			{
				_dirty = true;
			}

		private:
			static const uint32_t kPageSize = archsim::translate::profile::RegionArch::PageSize;
			// XXX ARM HAX
			static const uint32_t kMaxBlocksPerPage = kPageSize / kInstructionSize;
			static const uint32_t kBlocksPerChunk = 128;

			typedef struct table_chunk_t {
				BlockTranslation chunk[kBlocksPerChunk];
			} table_chunk_t;

			table_chunk_t *&getChunkPtr(Address address)
			{
				return _table.at((address.GetPageOffset() >> kInstructionAlignment) / kBlocksPerChunk);
			}
			table_chunk_t &getChunk(Address address)
			{
				table_chunk_t *&ptr = getChunkPtr(address);
				if(ptr == nullptr) ptr = new table_chunk_t();
				return *ptr;
			}

			std::array<table_chunk_t*, kMaxBlocksPerPage/kBlocksPerChunk> _table;

			std::unordered_set<block_txln_fn> _txlns;

			wulib::MemAllocator &_allocator;

			bool _dirty:1;
			bool _valid:1;
		};


		class BlockProfile
		{
		public:
			BlockProfile(wulib::MemAllocator &allocator);

			void Insert(Address address, const BlockTranslation &txln);
			void InvalidatePage(Address address);
			void Invalidate();

			bool IsPageDirty(Address addr)
			{
				return getProfile(addr).IsDirty();
			}
			void MarkPageDirty(Address addr)
			{
				LC_DEBUG1(LogBlockProfile) << "Marking page " << std::hex << addr.GetPageBase() << " as dirty";
				getProfile(addr).MakeDirty();
				_dirty_pages.push_back({addr.PageBase(), &getProfile(addr)});
			}

			void GarbageCollect();

			// XXX ARM HAX
			static const size_t kInstructionSize = BlockPageProfile::kInstructionSize;

			bool Get(Address address, const archsim::ProcessorFeatureSet &features, BlockTranslation &out_txln)
			{
				if(!hasProfile(address)) {
					return false;
				}

				auto &txln = getProfile(address).Get(address);

				if(txln.IsValid(features)) {
					out_txln = txln;
					return true;
				} else {
					getProfile(address).InvalidateTxln(address);
					return false;
				}
			}

		private:
			static const size_t kTableCount = archsim::translate::profile::RegionArch::PageCount;
			static const size_t kTablePageBits = 12;
			static const size_t kTablePageSize = 1 << kTablePageBits;
			static const size_t kTablePageCount = kTableCount / kTablePageSize;

			static const size_t kProfileCount = archsim::translate::profile::RegionArch::PageCount;

			BlockPageProfile &getProfile(Address address)
			{
				if(!hasProfile(address)) {
					_page_profiles[address.GetPageIndex()] = new BlockPageProfile(_allocator);
				}
				return *_page_profiles[address.GetPageIndex()];
			}
			bool hasProfile(Address address)
			{
				return _page_profiles[address.GetPageIndex()] != nullptr;
			}

			std::bitset<kTablePageCount> _table_pages_dirty;

			std::vector<std::pair<Address, BlockPageProfile *> > _dirty_pages;

			BlockPageProfile *_page_profiles[kProfileCount];
			wulib::MemAllocator &_allocator;
		};

	}
}



#endif /* INC_BLOCKJIT_BLOCKPROFILE_H_ */
