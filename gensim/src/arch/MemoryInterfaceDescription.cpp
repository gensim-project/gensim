/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/MemoryInterfaceDescription.h"

using namespace gensim::arch;

MemoryInterfacesDescription::MemoryInterfacesDescription() {
}

MemoryInterfaceDescription::MemoryInterfaceDescription(const std::string& name, uint64_t address_width_bytes, uint64_t word_width_bytes, bool big_endian) : name_(name), address_width_bytes_(address_width_bytes), data_width_bytes_(word_width_bytes), is_big_endian_(big_endian)
{

}
