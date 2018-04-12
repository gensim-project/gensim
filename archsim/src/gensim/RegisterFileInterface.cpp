/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/RegisterFileInterface.h"

using namespace archsim;

RegisterFileInterface::RegisterFileInterface(size_t size_in_bytes, std::initializer_list<RegisterFileEntryInterface>& entries) : size_in_bytes_(size_in_bytes)
{
	for(auto i : entries) {
		AddEntry(i);
	}
}

void RegisterFileInterface::AddEntry(const RegisterFileEntryInterface& entry)
{
	bank_interfaces_.insert(entry.GetName(), entry);
	bank_interfaces_id_.insert(entry.GetId(), entry);
}
