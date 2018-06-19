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

ElfBinaryLoader::ElfBinaryLoader(EmulationModel &emulation_model, bool load_symbols) : BinaryLoader(emulation_model, load_symbols), _load_bias(0) {}

ElfBinaryLoader::~ElfBinaryLoader() {}

bool ElfBinaryLoader::ProcessSymbolTable(Elf32_Shdr *string_table_section, Elf32_Shdr *symbol_table_section)
{
	const char *strings_base = (const char *)((unsigned long)_elf_header + string_table_section->sh_offset);
	Elf32_Sym *symbols_base = (Elf32_Sym *)((unsigned long)_elf_header + symbol_table_section->sh_offset);

	for (unsigned int i = 0; i < (symbol_table_section->sh_size / symbol_table_section->sh_entsize); i++) {
		Elf32_Sym *symbol = (Elf32_Sym *)((unsigned long)symbols_base + (i * symbol_table_section->sh_entsize));
		uint32_t symbol_type = ELF32_ST_TYPE(symbol->st_info);
		if(symbol_type == STT_FUNC)
			_emulation_model.AddSymbol(symbol->st_value + _load_bias, symbol->st_size, std::string(&strings_base[symbol->st_name]), FunctionSymbol);
		else
			_emulation_model.AddSymbol(symbol->st_value + _load_bias, symbol->st_size, std::string(&strings_base[symbol->st_name]), ObjectSymbol);
	}

	return true;
}

bool ElfBinaryLoader::LoadSymbols()
{
	Elf32_Shdr *section_header_base = (Elf32_Shdr *)((unsigned long)_elf_header + _elf_header->e_shoff);
	Elf32_Shdr *symbol_string_table = (Elf32_Shdr *)((unsigned long)section_header_base + (_elf_header->e_shstrndx * _elf_header->e_shentsize));
	const char *symbol_strings_base = (const char *)((unsigned long)_elf_header + symbol_string_table->sh_offset);
	Elf32_Shdr *string_table = NULL;

	for (int i = 0; i < _elf_header->e_shnum; i++) {
		Elf32_Shdr *section_header = (Elf32_Shdr *)((unsigned long)section_header_base + (i * _elf_header->e_shentsize));

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
		Elf32_Shdr *section_header = (Elf32_Shdr *)((unsigned long)section_header_base + (i * _elf_header->e_shentsize));

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

bool ElfBinaryLoader::ProcessBinary(bool load_symbols)
{
	_elf_header = (Elf32_Ehdr *)_binary_data;

	// Check the binary file size for some (basic) sanity.
	if (_binary_size < sizeof(Elf32_Ehdr)) {
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
	switch(elf_class) {
		case ELFCLASS64:
		case ELFCLASSNONE:
			throw std::logic_error("Unsupported ELF type");
		case ELFCLASS32:
			//OK!
			break;
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

	Elf32_Phdr *prog_header_base = (Elf32_Phdr *)((unsigned long)_elf_header + _elf_header->e_phoff);

	for (int i = 0; i < _elf_header->e_phnum; i++) {
		Elf32_Phdr *prog_header = (Elf32_Phdr *)((unsigned long)prog_header_base + (i * _elf_header->e_phentsize));

		if (prog_header->p_type == PT_LOAD) {
			LC_DEBUG1(LogElf) << "Encountered loadable ELF segment: vaddr=" << std::hex << prog_header->p_vaddr << ", memsz=" << std::hex << prog_header->p_memsz << ", filesz=" << std::hex << prog_header->p_filesz;
			if (!LoadSegment(prog_header)) {
				LC_DEBUG1(LogElf) << "Could not load segment.";
				return false;
			}
		}
	}

	return true;
}

UserElfBinaryLoader::UserElfBinaryLoader(UserEmulationModel& model, bool load_symbols) : ElfBinaryLoader(model, load_symbols)
{
}

UserElfBinaryLoader::~UserElfBinaryLoader()
{
}

bool UserElfBinaryLoader::PrepareLoad()
{
	// Err - arbitrary?
	_ph_loc = Address(0xfffd0000);

	Elf32_Phdr *prog_header_base = (Elf32_Phdr *)((unsigned long)_elf_header + _elf_header->e_phoff);

	//unsigned long aligned_ph_size = _emulation_model.memory_model.AlignUp(_elf_header->e_phentsize * _elf_header->e_phnum);

	_emulation_model.GetMemoryModel().GetMappingManager()->MapRegion(_ph_loc, 4096, archsim::abi::memory::RegFlagReadWriteExecute, "[prog header]");
	_emulation_model.GetMemoryModel().Poke(_ph_loc, (uint8_t *)prog_header_base, _elf_header->e_phentsize * _elf_header->e_phnum);

	for(Address i = _ph_loc; i < _ph_loc + (_elf_header->e_phentsize * _elf_header->e_phnum); i += 1) {
		uint8_t byte;
		_emulation_model.GetMemoryModel().Read8(i, byte);
	}

	_initial_brk = Address(0);

	return true;
}

bool UserElfBinaryLoader::LoadSegment(Elf32_Phdr *segment)
{
	Address target_address = segment->p_vaddr + _load_bias;
	Address aligned_address = _emulation_model.GetMemoryModel().AlignDown(target_address);
	unsigned long region_size = _emulation_model.GetMemoryModel().AlignUp(segment->p_memsz + (target_address - aligned_address)).Get();

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
	if (!_emulation_model.GetMemoryModel().GetMappingManager()->MapRegion(aligned_address, region_size, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), _binary_filename)) {
		LC_ERROR(LogElf) << "[ELF] Unable to map memory region for ELF segment";
		return false;
	}

	_emulation_model.GetMemoryModel().Poke(target_address, (uint8_t *)((unsigned long)_elf_header + segment->p_offset), (size_t)segment->p_filesz);
	_emulation_model.GetMemoryModel().GetMappingManager()->ProtectRegion(aligned_address, region_size, protection);

	if (aligned_address + region_size > _initial_brk) {
		_initial_brk = aligned_address + region_size;
	}

	return true;
}

Address UserElfBinaryLoader::GetInitialProgramBreak()
{
	return _initial_brk;
}

Address UserElfBinaryLoader::GetProgramHeaderLocation()
{
	return _ph_loc;
}

Address UserElfBinaryLoader::GetProgramHeaderEntrySize()
{
	return Address(_elf_header->e_phentsize);
}

unsigned int UserElfBinaryLoader::GetProgramHeaderEntryCount()
{
	return _elf_header->e_phnum;
}

SystemElfBinaryLoader::SystemElfBinaryLoader(SystemEmulationModel& model, bool load_symbols) : ElfBinaryLoader(model, load_symbols)
{
}

SystemElfBinaryLoader::~SystemElfBinaryLoader()
{
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
