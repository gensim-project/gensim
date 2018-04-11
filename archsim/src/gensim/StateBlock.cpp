/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/StateBlock.h"

using namespace archsim;

StateBlockInstance* StateBlockDescriptor::GetNewInstance()
{
	return new StateBlockInstance(*this);
}

void StateBlockDescriptor::AddBlock(const std::string& name, size_t size_in_bytes)
{
	assert(!finalised_);
	
	block_offsets_[name] = total_size_;
	total_size_ += size_in_bytes;
}

size_t StateBlockDescriptor::GetBlockOffset(const std::string& name)
{
	assert(block_offsets_.count(name));
	return block_offsets_.at(name);
}

