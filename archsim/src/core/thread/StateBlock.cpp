/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/thread/StateBlock.h"

using namespace archsim;

StateBlockDescriptor::StateBlockDescriptor() : total_size_(0)
{

}


uint32_t StateBlockDescriptor::AddBlock(const std::string& name, size_t size_in_bytes)
{
	block_offsets_[name] = total_size_;
	total_size_ += size_in_bytes;
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

