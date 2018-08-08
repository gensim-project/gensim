/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * abi/loader/ElfBinaryLoader.cpp
 */
#include "abi/loader/BinaryLoader.h"
#include "abi/EmulationModel.h"
#include "abi/UserEmulationModel.h"
#include "abi/SystemEmulationModel.h"

#include "util/LogContext.h"

UseLogContext(LogLoader);
DeclareChildLogContext(LogElf, LogLoader, "ELF");

using namespace archsim::abi::loader;
using archsim::Address;

template<typename elfclass> bool ElfBinaryLoader<elfclass>::ProcessSymbolTable(typename elfclass::SHeader *string_table_section, typename elfclass::SHeader *symbol_table_section)
{
	using Sym = typename elfclass::Sym;

	const char *strings_base = (const char *)((unsigned long)_elf_header + string_table_section->sh_offset);
	Sym *symbols_base = (Sym *)((unsigned long)_elf_header + symbol_table_section->sh_offset);

	for (unsigned int i = 0; i < (symbol_table_section->sh_size / symbol_table_section->sh_entsize); i++) {
		Sym *symbol = (Sym *)((unsigned long)symbols_base + (i * symbol_table_section->sh_entsize));
		uint32_t symbol_type = ELF32_ST_TYPE(symbol->st_info);
		if(symbol_type == STT_FUNC)
			_emulation_model.AddSymbol(symbol->st_value + _load_bias, symbol->st_size, std::string(&strings_base[symbol->st_name]), FunctionSymbol);
		else
			_emulation_model.AddSymbol(symbol->st_value + _load_bias, symbol->st_size, std::string(&strings_base[symbol->st_name]), ObjectSymbol);
	}

	return true;
}


template<typename elfclass> bool ElfBinaryLoader<elfclass>::LoadSymbols()
{
	using SHeader = typename elfclass::SHeader;

	SHeader *section_header_base = (SHeader *)((unsigned long)_elf_header + _elf_header->e_shoff);
	SHeader *symbol_string_table = (SHeader *)((unsigned long)section_header_base + (_elf_header->e_shstrndx * _elf_header->e_shentsize));
	const char *symbol_strings_base = (const char *)((unsigned long)_elf_header + symbol_string_table->sh_offset);
	SHeader *string_table = NULL;

	for (int i = 0; i < _elf_header->e_shnum; i++) {
		SHeader *section_header = (SHeader *)((unsigned long)section_header_base + (i * _elf_header->e_shentsize));

		if (section_header->sh_type == SHT_STRTAB && (strcmp(&symbol_strings_base[section_header->sh_name], ".strtab") == 0)) {
			LC_DEBUG1(LogElf) << "Found string table " << &symbol_strings_base[section_header->sh_name];
			string_table = section_header;
			break;
		}
	}

	if (!string_table) {
		LC_DEBUG1(LogElf) << "!string_table";
		return false;
	}

	for (int i = 0; i < _elf_header->e_shnum; i++) {
		SHeader *section_header = (SHeader *)((unsigned long)section_header_base + (i * _elf_header->e_shentsize));

		if (section_header->sh_type == SHT_SYMTAB) {
			if (!ProcessSymbolTable(string_table, section_header)) {
				LC_DEBUG1(LogElf) << "!ProcessSymbolTable";
				return false;
			}
		}
	}

	_emulation_model.FixupSymbols();

	return true;
}

template<typename elfclass> bool ElfBinaryLoader<elfclass>::ProcessBinary(bool load_symbols)
{
	using ElfHeader = typename elfclass::ElfHeader;
	using PHeader = typename elfclass::PHeader;
	_elf_header = (ElfHeader *)_binary_data;

	// Check the binary file size for some (basic) sanity.
	if (_binary_size < sizeof(ElfHeader)) {
		LC_DEBUG1(LogElf) << "Binary is too small to be an ELF file.";
		return false;
	}

	// Check the magic number.
	unsigned char *ident = _elf_header->e_ident;
	if (!(ident[EI_MAG0] == ELFMAG0 && ident[EI_MAG1] == ELFMAG1 && ident[EI_MAG2] == ELFMAG2 && ident[EI_MAG3] == ELFMAG3)) {
		LC_DEBUG1(LogElf) << "Binary is not an ELF file.";
		return false;
	}
	uint8_t elf_class = ident[EI_CLASS];
	if(elf_class != elfclass::ElfClass) {
		throw std::logic_error("Unsupported elf class");
	}

	_entry_point = Address(_elf_header->e_entry);

	// If the ELF file is dynamic, create a load bias.
	if (_elf_header->e_type == ET_DYN) {
		_load_bias = Address(0xF0000000);
		_entry_point += 0xF0000000;
	} else
		_load_bias = Address(0);

	if (load_symbols && !LoadSymbols()) {
		LC_DEBUG1(LogElf) << "Symbol loading failed.";
//		return false;
	}

	LC_DEBUG1(LogElf) << "Loading ELF Binary with " << _elf_header->e_phnum << " segments";

	if (!PrepareLoad()) {
		LC_DEBUG1(LogElf) << "Could not prepare environment for loaded binary.";
		return false;
	}

	PHeader *prog_header_base = (PHeader *)((unsigned long)_elf_header + _elf_header->e_phoff);

	for (int i = 0; i < _elf_header->e_phnum; i++) {
		PHeader *prog_header = (PHeader *)((unsigned long)prog_header_base + (i * _elf_header->e_phentsize));

		if (prog_header->p_type == PT_LOAD) {

			if(_ph_loc == Address::NullPtr) {
				_ph_loc = Address(prog_header->p_vaddr + _elf_header->e_phoff);
			}

			LC_DEBUG1(LogElf) << "Encountered loadable ELF segment: vaddr=" << std::hex << prog_header->p_vaddr << ", memsz=" << std::hex << prog_header->p_memsz << ", filesz=" << std::hex << prog_header->p_filesz;
			if (!LoadSegment(prog_header)) {
				LC_DEBUG1(LogElf) << "Could not load segment.";
				return false;
			}
		} else if(prog_header->p_type == PT_PHDR) {
			LC_DEBUG1(LogElf) << "Found PHDR segment with address " << Address(prog_header->p_vaddr);
			_ph_loc = Address(prog_header->p_vaddr);
		}
	}


	return true;
}

template<typename elfclass> bool UserElfBinaryLoader<elfclass>::PrepareLoad()
{
	using PHeader = typename elfclass::PHeader;
	// Err - arbitrary?


//	PHeader *prog_header_base = (PHeader *)((unsigned long)this->_elf_header + this->_elf_header->e_phoff);
//
//	//unsigned long aligned_ph_size = _emulation_model.memory_model.AlignUp(_elf_header->e_phentsize * _elf_header->e_phnum);
//
//	this->_emulation_model.GetMemoryModel().GetMappingManager()->MapRegion(_ph_loc, 4096, archsim::abi::memory::RegFlagReadWriteExecute, "[prog header]");
//	this->_emulation_model.GetMemoryModel().Poke(this->_ph_loc, (uint8_t *)prog_header_base, this->_elf_header->e_phentsize * this->_elf_header->e_phnum);
//
//	for(Address i = _ph_loc; i < _ph_loc + (this->_elf_header->e_phentsize * this->_elf_header->e_phnum); i += 1) {
//		uint8_t byte;
//		this->_emulation_model.GetMemoryModel().Read8(i, byte);
//	}

	_initial_brk = Address(0);

	return true;
}

template<typename elfclass> bool UserElfBinaryLoader<elfclass>::LoadSegment(typename elfclass::PHeader *segment)
{
	Address target_address = segment->p_vaddr + this->_load_bias;
	Address aligned_address = this->_emulation_model.GetMemoryModel().AlignDown(target_address);
	unsigned long region_size = this->_emulation_model.GetMemoryModel().AlignUp(segment->p_memsz + (target_address - aligned_address)).Get();

	LC_DEBUG1(LogElf) << "[ELF] Loading ELF Segment, target address = " << std::hex << target_address << " aligned to " << std::hex << aligned_address << " size " << std::hex << region_size;

	memory::RegionFlags protection = memory::RegFlagWrite;

	if (segment->p_flags & PF_R) {
		protection = (memory::RegionFlags)(protection | memory::RegFlagRead);
		LC_DEBUG1(LogElf) << "[ELF] Segment has 'read' flag";
	}

	if (segment->p_flags & PF_W) {
		protection = (memory::RegionFlags)(protection | memory::RegFlagWrite);
		LC_DEBUG1(LogElf) << "[ELF] Segment has 'write' flag";
	}

	if (segment->p_flags & PF_X) {
		protection = (memory::RegionFlags)(protection | memory::RegFlagExecute);
		LC_DEBUG1(LogElf) << "[ELF] Segment has 'execute' flag";
	}

	LC_DEBUG1(LogElf) << "[ELF] Loading ELF Segment to " << std::hex << aligned_address << " (" << std::hex << region_size << ")";
	if (!this->_emulation_model.GetMemoryModel().GetMappingManager()->MapRegion(aligned_address, region_size, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), this->_binary_filename)) {
		LC_ERROR(LogElf) << "[ELF] Unable to map memory region for ELF segment";
		return false;
	}

	this->_emulation_model.GetMemoryModel().Poke(target_address, (uint8_t *)((unsigned long)this->_elf_header + segment->p_offset), (size_t)segment->p_filesz);
	this->_emulation_model.GetMemoryModel().GetMappingManager()->ProtectRegion(aligned_address, region_size, protection);

	if (aligned_address + region_size > _initial_brk) {
		_initial_brk = aligned_address + region_size;
	}

	return true;
}

bool SystemElfBinaryLoader::PrepareLoad()
{
	return true;
}

bool SystemElfBinaryLoader::LoadSegment(Elf32_Phdr* segment)
{
	Address target_address = segment->p_paddr + _load_bias;

	LC_DEBUG1(LogElf) << "[ELF] Loading ELF Segment, target address = " << std::hex << target_address << " poffset " << segment->p_offset;
	return _emulation_model.GetMemoryModel().Poke(target_address, (uint8_t *)((unsigned long)_elf_header + segment->p_offset), (size_t)segment->p_filesz) == 0;
}
