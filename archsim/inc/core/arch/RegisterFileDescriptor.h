/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   ArchDescriptor.h
 * Author: harry
 *
 * Created on 10 April 2018, 15:23
 */

#ifndef REGISTERFILEDESCRIPTOR_H
#define REGISTERFILEDESCRIPTOR_H

#include "abi/Address.h"

#include <functional>
#include <map>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace archsim
{

	class RegisterFileEntryDescriptor
	{
	public:
		RegisterFileEntryDescriptor(const std::string &name, uint32_t id, uint32_t offset, uint32_t entry_count, uint32_t entry_size, uint32_t entry_stride, const std::string tag = "");

		uint64_t GetOffset() const
		{
			return offset_;
		}
		uint64_t GetEntryCount() const
		{
			return entry_count_;
		}
		uint64_t GetEntrySize() const
		{
			return entry_size_;
		}
		uint64_t GetEntryStride() const
		{
			return entry_stride_;
		}

		const std::string &GetTag() const
		{
			return tag_;
		}
		uint32_t GetID() const
		{
			return id_;
		}
		const std::string &GetName() const
		{
			return name_;
		}

	private:
		std::string name_;
		std::string tag_;
		uint32_t id_;
		uint64_t offset_;
		uint64_t entry_count_;
		uint64_t entry_size_;
		uint64_t entry_stride_;
	};

	class RegisterFileDescriptor
	{
	public:
		using register_file_entry_collection_t = std::unordered_map<std::string, RegisterFileEntryDescriptor>;
		using register_file_id_collection_t = std::unordered_map<uint32_t, std::string>;

		RegisterFileDescriptor(uint64_t total_size, const std::initializer_list<RegisterFileEntryDescriptor> &entries);
		uint64_t GetSize() const
		{
			return total_size_in_bytes_;
		}

		const register_file_entry_collection_t &GetEntries() const
		{
			return entries_;
		}

		const RegisterFileEntryDescriptor &GetTaggedEntry(const std::string &tag) const
		{
			if(tagged_entries_.count(tag) == 0) {
				throw std::logic_error("Unknown tag \"" + tag + "\"");
			}
			return tagged_entries_.at(tag);
		}

		const RegisterFileEntryDescriptor &GetByID(uint32_t id) const
		{
			return entries_.at(id_to_names_.at(id));
		}

	private:
		uint64_t total_size_in_bytes_;
		register_file_entry_collection_t entries_;
		register_file_id_collection_t id_to_names_;
		register_file_entry_collection_t tagged_entries_;
	};

}

#endif /* ARCHDESCRIPTOR_H */

