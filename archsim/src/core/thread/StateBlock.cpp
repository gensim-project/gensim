/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/thread/StateBlock.h"

using namespace archsim;

StateBlockDescriptor::StateBlockDescriptor() : total_size_(0)
{

}


uint32_t StateBlockDescriptor::AddBlock(const std::string& name, size_t size_in_bytes)
{
	block_offsets_[name] = total_size_;
	total_size_ += size_in_bytes;
	return total_size_;
}

size_t StateBlockDescriptor::GetBlockOffset(const std::string& name) const
{
	return block_offsets_.at(name);
}

uint32_t StateBlock::AddBlock(const std::string& name, size_t size_in_bytes)
{
	uint32_t offset = data_.size();
	descriptor_.AddBlock(name, size_in_bytes);
	data_.resize(data_.size() + size_in_bytes);
	return offset;
}

