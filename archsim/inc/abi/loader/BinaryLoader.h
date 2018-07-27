/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   BinaryLoader.h
 * Author: s0457958
 *
 * Created on 20 November 2013, 15:25
 */

#ifndef BINARYLOADER_H
#define BINARYLOADER_H

#include <cstdint>
#include <string>
#include <elf.h>

#include "abi/Address.h"

namespace archsim
{
	namespace abi
	{
		class EmulationModel;

		namespace loader
		{

			class BinaryLoader
			{
			public:
				BinaryLoader(EmulationModel& emulation_model, bool load_symbols);
				virtual ~BinaryLoader();

				bool LoadBinary(std::string filename);

				Address GetEntryPoint();

			protected:
				std::string _binary_filename;
				const char* _binary_data;
				unsigned int _binary_size;
				Address _entry_point;
				EmulationModel& _emulation_model;

				virtual bool ProcessBinary(bool load_symbols) = 0;

			private:
				bool _load_symbols;
			};

			class FlatBinaryLoader : public BinaryLoader
			{
			public:
				FlatBinaryLoader(EmulationModel& emulation_model, Address base_addr);
				virtual ~FlatBinaryLoader();

			protected:
				bool ProcessBinary(bool load_symbols);

			private:
				Address _base_addr;
			};

			class ZImageBinaryLoader : public BinaryLoader
			{
			public:
				ZImageBinaryLoader(EmulationModel& emulation_model, Address base_addr, std::string symbol_map);
				ZImageBinaryLoader(EmulationModel& emulation_model, Address base_addr);
				virtual ~ZImageBinaryLoader();

			protected:
				bool ProcessBinary(bool load_symbols);

			private:
				bool LoadSymbolMap(std::string map_file);

				Address _base_addr;
				std::string _symbol_map;
			};

			struct ElfClass32 {
				const static int ElfClass = ELFCLASS32;

				using ElfHeader = Elf32_Ehdr;
				using PHeader = Elf32_Phdr;
				using SHeader = Elf32_Shdr;
				using Sym = Elf32_Sym;
			};
			struct ElfClass64 {
				const static int ElfClass = ELFCLASS64;

				using ElfHeader = Elf64_Ehdr;
				using PHeader = Elf64_Phdr;
				using SHeader = Elf64_Shdr;
				using Sym = Elf64_Sym;
			};

			template<typename elfclass> class ElfBinaryLoader : public BinaryLoader
			{
			public:
				ElfBinaryLoader(EmulationModel& emulation_model, bool load_symbols) : BinaryLoader(emulation_model, load_symbols), _load_bias(0_ga) {}
				virtual ~ElfBinaryLoader() {};

				Address GetProgramHeaderLocation() const
				{
					return _ph_loc;
				}
				Address GetProgramHeaderEntrySize() const
				{
					return Address(this->_elf_header->e_phentsize);
				}
				unsigned int GetProgramHeaderEntryCount() const
				{
					return this->_elf_header->e_phnum;
				}

			protected:
				Address _load_bias;
				Address _ph_loc;
				typename elfclass::ElfHeader *_elf_header;

				bool ProcessBinary(bool load_symbols);
				virtual bool PrepareLoad() = 0;
				virtual bool LoadSegment(typename elfclass::PHeader *segment) = 0;

			private:
				bool LoadSymbols();
				bool ProcessSymbolTable(typename elfclass::SHeader *string_table_section, typename elfclass::SHeader *symbol_table_section);
			};
			template class ElfBinaryLoader<ElfClass32>;
			template class ElfBinaryLoader<ElfClass64>;

			template<typename ElfClass> class UserElfBinaryLoader : public ElfBinaryLoader<ElfClass>
			{
			public:
				UserElfBinaryLoader(EmulationModel& emulation_model, bool load_symbols) : ElfBinaryLoader<ElfClass>(emulation_model, load_symbols) {}
				~UserElfBinaryLoader() {}

				Address GetInitialProgramBreak() const
				{
					return _initial_brk;
				}

			protected:
				bool PrepareLoad() override;
				bool LoadSegment(typename ElfClass::PHeader *segment) override;

			private:
				Address _initial_brk;
			};
			template class UserElfBinaryLoader<ElfClass32>;
			template class UserElfBinaryLoader<ElfClass64>;

			class SystemElfBinaryLoader : public ElfBinaryLoader<ElfClass32>
			{
			public:
				SystemElfBinaryLoader(EmulationModel& emulation_model, bool load_symbols) : ElfBinaryLoader<ElfClass32>(emulation_model, load_symbols) {}
				~SystemElfBinaryLoader() {}

			protected:
				bool PrepareLoad() override;
				bool LoadSegment(Elf32_Phdr *segment) override;
			};
		}
	}
}

#endif /* BINARYLOADER_H */
