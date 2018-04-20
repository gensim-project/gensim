/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/StateBlock.h"

using namespace archsim;

void StateBlock::AddBlock(const std::string& name, size_t size_in_bytes)
{
	block_offsets_[name] = data_.size();
	data_.resize(data_.size() + size_in_bytes);
}

size_t StateBlock::GetBlockOffset(const std::string& name) const
{
	return block_offsets_.at(name);
}
