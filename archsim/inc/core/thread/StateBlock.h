/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   StateBlock.h
 * Author: harry
 *
 * Created on 10 April 2018, 15:42
 */

#ifndef STATEBLOCK_H
#define STATEBLOCK_H

#include <cassert>
#include <string>
#include <vector>
#include <cstring>
#include <map>
#include <cstdint>

namespace archsim
{

	class StateBlock;

	/**
	 * The State Block is essentially a contiguous key-value store used
	 * for various metadata and metadata pointers. During Thread
	 * initialisation, different parts of the simulation can allocate
	 * memory within the state block which can then be used later on
	 * during simulation.
	 *
	 * This way, JIT systems can keep a pointer to the state block and
	 * still be able to quickly look up information by using the block
	 * offsets.
	 */

	class StateBlockDescriptor
	{
	public:
		StateBlockDescriptor();

		uint32_t AddBlock(const std::string &name, size_t size_in_bytes);
		size_t GetBlockOffset(const std::string &name) const;
		size_t GetBlockSizeInBytes(const std::string &name) const;
		bool HasEntry(const std::string &name) const
		{
			return block_offsets_.count(name);
		}

	private:
		std::map<std::string, uint64_t> block_offsets_;
		std::map<std::string, uint64_t> block_sizes_in_bytes_;
		uint32_t total_size_;
	};

	class StateBlock
	{
	public:
		uint32_t AddBlock(const std::string &name, size_t size_in_bytes);

		StateBlockDescriptor &GetDescriptor()
		{
			return descriptor_;
		}

		uint32_t GetBlockOffset(const std::string &name) const
		{
			return descriptor_.GetBlockOffset(name);
		}
		void *GetData()
		{
			return data_.data();
		}
		const void *GetData() const
		{
			return data_.data();
		}

		template<typename T> T GetEntry(const std::string &entryname) const
		{
			return *(T*)(data_.data() + descriptor_.GetBlockOffset(entryname));
		}
		template<typename T> void GetEntry(const std::string &entryname, T* target) const
		{
			memcpy(target, GetEntryPointer<T>(entryname), sizeof(T));
		}
		template<typename T> void SetEntry(const std::string &entryname, const T& t)
		{
			memcpy(GetEntryPointer<T>(entryname), &t, sizeof(t));
		}


		template<typename T> T* GetEntryPointer(const std::string &entryname)
		{
			return (T*)(((char*)GetData()) + descriptor_.GetBlockOffset(entryname));
		}
		template<typename T> const T* GetEntryPointer(const std::string &entryname) const
		{
			return (T*)(((char*)GetData()) + descriptor_.GetBlockOffset(entryname));
		}

	private:
		StateBlockDescriptor descriptor_;
		std::vector<unsigned char> data_;
	};

}

#endif /* STATEBLOCK_H */
