/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * RegisterFile.h
 *
 *  Created on: 12 May 2015
 *      Author: harry
 */

#ifndef INC_REGISTERFILE_H_
#define INC_REGISTERFILE_H_

#include <cstdint>

#include <string>
#include <vector>
#include <ostream>
#include <map>

#include "DiagnosticContext.h"
#include "genC/ir/IRType.h"

namespace gensim
{

	namespace arch
	{

		class RegisterFile;
		class RegSpaceDescriptor;

		class RegSlotViewDescriptor
		{
		public:
			RegSlotViewDescriptor(RegSpaceDescriptor *parent,std::string id, std::string type, uint32_t width, uint32_t offset);
			RegSlotViewDescriptor(RegSpaceDescriptor *parent,std::string id, std::string type, uint32_t width, uint32_t offset, std::string tag);

			std::string GetID() const;
			uint32_t GetIndex() const;
			uint32_t GetWidth() const;
			bool HasTag() const;
			std::string GetTag() const;

			uint32_t GetRegSpaceOffset() const;
			uint32_t GetRegFileOffset() const;
			const genc::IRType GetIRType() const;

			bool PrettyPrint(std::ostream &) const;

			bool Resolve(DiagnosticContext &context);

			/*
			 * Write out a descriptor of this register slot.
			 *
			 * Takes a destination pointer (which may be null) and returns the size of the descriptor.
			 */
			uint32_t WriteDescriptor(uint8_t *dest) const;
		private:
			RegSpaceDescriptor *parent_space;

			std::string ID;
			std::string TypeName;

			uint32_t Offset;
			uint32_t Size;

			std::string tag;
			bool has_tag;

			uint32_t Index;
		};

		class RegBankViewDescriptor
		{
		public:
			RegBankViewDescriptor(
			    RegSpaceDescriptor *parent,
			    const std::string& id,
			    uint32_t space_offset,
			    uint32_t register_count,
			    uint32_t register_stride,
			    uint32_t element_count,
			    uint32_t element_size,
			    uint32_t element_stride,
			    const std::string& element_type
			) : parent_space(parent),
				ID(id),
				ElementTypeName(element_type),
				RegisterSpaceOffset(space_offset),
				RegisterCount(register_count),
				RegisterStride(register_stride),
				ElementCount(element_count),
				ElementSize(element_size),
				ElementStride(element_stride) { }


			const std::string ID;
			const std::string ElementTypeName;

			bool Hidden;

			uint32_t GetIndex() const;

			/**
			 * Get the size of the entire register bank.
			 */
			uint32_t GetBankSize() const
			{
				return GetRegisterCount() * GetRegisterStride();
			}

			uint32_t GetRegisterCount() const
			{
				return RegisterCount;
			}
			uint32_t GetRegisterWidth() const
			{
				return GetElementCount() * GetElementStride();
			}
			uint32_t GetRegisterStride() const
			{
				return RegisterStride;
			}

			uint32_t GetRegSpaceOffset() const
			{
				return RegisterSpaceOffset;
			}
			uint32_t GetRegFileOffset() const;

			uint32_t GetElementCount() const
			{
				return ElementCount;
			}
			uint32_t GetElementSize() const
			{
				return ElementSize;
			}
			uint32_t GetElementStride() const
			{
				return ElementStride;
			}

			const genc::IRType GetRegisterIRType() const;
			const genc::IRType GetElementIRType() const;

			bool PrettyPrint(std::ostream &) const;

			bool Resolve(DiagnosticContext &context);

			/*
			 * Write out a descriptor of this register bank.
			 *
			 * Takes a destination pointer (which may be null) and returns the size of the descriptor.
			 */
			uint32_t WriteDescriptor(uint8_t *dest) const;
		private:
			RegSpaceDescriptor *parent_space;

			uint32_t RegisterSpaceOffset;
			uint32_t RegisterCount;
			uint32_t RegisterStride;

			uint32_t ElementCount;
			uint32_t ElementSize;
			uint32_t ElementStride;
		};

		class RegSpaceDescriptor
		{
		public:
			RegSpaceDescriptor(RegisterFile *parent_file, uint32_t size);

			bool AddRegBankView(RegBankViewDescriptor *bank);
			bool AddRegSlotView(RegSlotViewDescriptor *slot);

			bool HasRegBank(std::string ID) const;
			bool HasRegSlot(std::string ID) const;
			bool HasView(std::string ID) const;

			uint32_t GetOffsetInFile() const;

			uint32_t GetSize() const
			{
				return size_in_bytes;
			}

			uint32_t GetAlignmentRequirements() const;

			bool PrettyPrint(std::ostream &) const;

			bool Resolve(DiagnosticContext &context);

			inline RegisterFile *GetFile()
			{
				return parent_file;
			}

			/*
			 * Write out a descriptor of this register space.
			 * This is the concatenation of descriptors of each register in this space.
			 *
			 * Takes a destination pointer (which may be null) and returns the size of the descriptor.
			 */
			uint32_t WriteDescriptor(uint8_t *dest) const;

		private:
			uint32_t size_in_bytes;

			RegisterFile *parent_file;

			std::vector<RegBankViewDescriptor*> banks;
			std::vector<RegSlotViewDescriptor*> slots;

		};

		class RegisterFile
		{
		public:
			typedef std::vector<RegBankViewDescriptor*> bank_container_t;
			typedef std::vector<RegSlotViewDescriptor*> slot_container_t;
			typedef std::vector<RegSpaceDescriptor*> space_container_t;
			typedef std::map<std::string, RegSlotViewDescriptor*> tagged_slot_container_t;

			uint32_t GetOffsetOf(const RegSpaceDescriptor *space) const;

			bool AddRegisterSpace(RegSpaceDescriptor *);
			RegSpaceDescriptor *GetSpaceByIndex(uint32_t idx);

			bool AddRegisterBank(RegSpaceDescriptor *space, RegBankViewDescriptor *bank);
			bool AddRegisterSlot(RegSpaceDescriptor *space, RegSlotViewDescriptor *slot);

			bool HasSpace(RegSpaceDescriptor *space) const;

			bool HasRegBank(std::string ID) const;
			bool HasRegSlot(std::string ID) const;
			bool HasView(std::string ID) const;

			bool HasTaggedRegSlot(std::string tag) const;
			const RegSlotViewDescriptor *GetTaggedRegSlot(std::string tag) const;

			const bank_container_t &GetBanks() const
			{
				return bank_views;
			}
			const slot_container_t &GetSlots() const
			{
				return slot_views;
			}
			const space_container_t &GetSpaces() const
			{
				return spaces;
			}

			RegSlotViewDescriptor &GetSlot(const std::string &id)
			{
				for(RegSlotViewDescriptor *i : slot_views) {
					if(i->GetID() == id) {
						return *i;
					}
				}
				throw std::logic_error("");
			}
			const RegSlotViewDescriptor &GetSlot(const std::string &id) const
			{
				for(RegSlotViewDescriptor *i : slot_views) {
					if(i->GetID() == id) {
						return *i;
					}
				}
				throw std::logic_error("");
			}
			RegBankViewDescriptor &GetBank(const std::string &id)
			{
				for(RegBankViewDescriptor *i : bank_views) {
					if(i->ID == id) {
						return *i;
					}
				}
				throw std::logic_error("");
			}

			inline const RegBankViewDescriptor &GetBankByIdx(uint32_t idx) const
			{
				return *bank_views.at(idx);
			}
			inline const RegSlotViewDescriptor &GetSlotByIdx(uint32_t idx) const
			{
				return *slot_views.at(idx);
			}

			uint32_t GetIndexOfSlot(const RegSlotViewDescriptor &slot) const;
			uint32_t GetIndexOfBank(const RegBankViewDescriptor &slot) const;

			uint32_t GetSize() const;

			bool PrettyPrint(std::ostream &) const;

			bool Resolve(DiagnosticContext &diag);

			/*
			 * Write out a descriptor of this register file.
			 * This is the concatenation of descriptors of each register space in this file.
			 *
			 * Takes a destination pointer (which may be null) and returns the size of the descriptor.
			 */
			uint32_t WriteDescriptor(uint8_t *dest) const;

		private:
			space_container_t spaces;
			bank_container_t bank_views;
			slot_container_t slot_views;
			tagged_slot_container_t tagged_slots;
		};

	}
}



#endif /* INC_REGISTERFILE_H_ */

