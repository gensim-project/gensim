/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/thread/StateBlock.h"

using namespace archsim;

uint32_t StateBlock::AddBlock(const std::string& name, size_t size_in_bytes)
{
	uint32_t offset = data_.size();
	block_offsets_[name] = offset;
	data_.resize(data_.size() + size_in_bytes);
	return offset;
}

size_t StateBlock::GetBlockOffset(const std::string& name) const
{
	return block_offsets_.at(name);
}
