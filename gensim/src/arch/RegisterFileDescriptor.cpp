/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/RegisterFile.h"
#include "arch/RegisterFileDescriptor.h"
#include <string.h>

namespace gensim
{
	namespace arch
	{

		uint32_t RegSlotViewDescriptor::WriteDescriptor(uint8_t *dest) const
		{
			if(dest) {
				struct RegisterSlotDescriptorStruct *data = (struct RegisterSlotDescriptorStruct*)dest;
				data->type = 1;
				data->width_in_bytes = GetWidth();
				data->offset_in_bytes = GetRegFileOffset();
				data->name_length_in_bytes = GetID().size();
				data->tag_length_in_bytes = GetTag().size();
				memcpy(data->names, GetID().c_str(), GetID().size());
				memcpy(data->names + GetID().size(), GetTag().c_str(), GetTag().size());
			}

			return sizeof(struct RegisterSlotDescriptorStruct) + ID.size() + tag.size();
		}

		uint32_t RegBankViewDescriptor::WriteDescriptor(uint8_t *dest) const
		{
			if(dest) {
				struct RegisterBankDescriptorStruct *data = (struct RegisterBankDescriptorStruct*)dest;
				data->type = 2;
				data->width_in_bytes = GetElementSize();
				data->stride_in_bytes = GetElementSize();
				data->offset_in_bytes = GetRegFileOffset();
				data->name_length_in_bytes = ID.size();
				memcpy(data->name, ID.c_str(), ID.size());
			}

			return sizeof(struct RegisterBankDescriptorStruct) + ID.size();
		}

		uint32_t RegSpaceDescriptor::WriteDescriptor(uint8_t *dest) const
		{
			if(dest) {
				dest += sizeof(struct RegisterSpaceDescriptorStruct);
			}

			uint32_t total_size = sizeof(struct RegisterFileDescriptorStruct);
			for(const auto space : banks) {
				uint32_t space_size = space->WriteDescriptor(dest);
				total_size += space_size;
				if(dest) dest += space_size;
			}
			for(const auto space : slots) {
				uint32_t space_size = space->WriteDescriptor(dest);
				total_size += space_size;
				if(dest) dest += space_size;
			}

			return total_size;
		}

		uint32_t RegisterFile::WriteDescriptor(uint8_t *dest) const
		{
			if(dest) {
				dest += sizeof(struct RegisterFileDescriptorStruct);
			}

			uint32_t total_size = sizeof(struct RegisterFileDescriptorStruct);
			for(const auto space : spaces) {
				uint32_t space_size = space->WriteDescriptor(dest);
				total_size += space_size;
				if(dest) dest += space_size;
			}

			return total_size;
		}

	}
}
