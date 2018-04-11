/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/ArchDescriptor.h"
#include "define.h"

using namespace archsim;

ArchDescriptor::ArchDescriptor(const RegisterFileDescriptor& rf, const MemoryInterfacesDescriptor& mem, const FeaturesDescriptor& f) : register_file_(rf), mem_interfaces_(mem), features_(f)
{

}
