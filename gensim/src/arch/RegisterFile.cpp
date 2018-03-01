/*
 * RegisterFile.cpp
 *
 *  Created on: 12 May 2015
 *      Author: harry
 */

#include <cassert>
#include <set>

#include "arch/RegisterFile.h"

using namespace gensim;
using namespace gensim::arch;

/*
 * RegSlotViewDescriptor Functions
 */

RegSlotViewDescriptor::RegSlotViewDescriptor(RegSpaceDescriptor *parent, std::string id, std::string type, uint32_t width, uint32_t offset) : parent_space(parent), ID(id), TypeName(type), Size(width), Offset(offset), has_tag(false)
{

}

RegSlotViewDescriptor::RegSlotViewDescriptor(RegSpaceDescriptor *parent, std::string id, std::string type, uint32_t width, uint32_t offset, std::string tag) : RegSlotViewDescriptor(parent, id, type, width, offset)
{
	this->tag = tag;
	has_tag = true;
}

std::string RegSlotViewDescriptor::GetID() const
{
	return ID;
}

uint32_t RegSlotViewDescriptor::GetIndex() const
{
	return parent_space->GetFile()->GetIndexOfSlot(*this);
}

uint32_t RegSlotViewDescriptor::GetWidth() const
{
	return Size;
}

bool RegSlotViewDescriptor::HasTag() const
{
	return has_tag;
}

std::string RegSlotViewDescriptor::GetTag() const
{
	assert(HasTag());
	return tag;
}

uint32_t RegSlotViewDescriptor::GetRegSpaceOffset() const
{
	return Offset;
}

uint32_t RegSlotViewDescriptor::GetRegFileOffset() const
{
	return parent_space->GetOffsetInFile() + GetRegSpaceOffset();
}

const genc::IRType RegSlotViewDescriptor::GetIRType() const
{
	genc::IRType type;
	genc::IRType::ParseType(TypeName, type);

	return type;
}

bool RegSlotViewDescriptor::PrettyPrint(std::ostream &str) const
{
	str << "slot " << ID << " (" << TypeName << ", " << Size << ", " << Offset << ");";
	return true;
}

bool RegSlotViewDescriptor::Resolve(DiagnosticContext &diag)
{
	bool success = true;

	if((GetRegSpaceOffset() + Size) > parent_space->GetSize()) {
		diag.Error("Register slot " + ID + ": exceeds the bounds of its register space!", DiagNode());
		success = false;
	}

	genc::IRType out;
	if(!genc::IRType::ParseType(TypeName, out)) {
		diag.Error("Register slot " + ID + ": Unknown type " + TypeName, DiagNode());
		success = false;
	}

	return success;
}

const genc::IRType RegBankViewDescriptor::GetElementIRType() const
{
	genc::IRType type;
	if (!genc::IRType::ParseType(ElementTypeName, type)) {
		throw std::logic_error("Unable to parse type: " + ElementTypeName);
	}

	return type;
}

uint32_t RegBankViewDescriptor::GetIndex() const
{
	return parent_space->GetFile()->GetIndexOfBank(*this);
}


const genc::IRType RegBankViewDescriptor::GetRegisterIRType() const
{
	genc::IRType type = GetElementIRType();
	if (ElementCount > 1) {
		type.VectorWidth = ElementCount;
	}

	return type;
}

bool RegBankViewDescriptor::PrettyPrint(std::ostream &str) const
{
	str << "bank " << ID << " (" << ElementTypeName << ", " << RegisterSpaceOffset << ", " << RegisterCount << ", " << RegisterStride << ", " << ElementCount << ", " << ElementSize << ", " << ElementStride << ");";
	return true;
}

bool RegBankViewDescriptor::Resolve(DiagnosticContext &diag)
{
	bool success = true;
	if(RegisterCount == 0) {
		diag.Error("Register bank " + ID + ": must have at least one register!", DiagNode());
		success = false;
	}

	if((RegisterSpaceOffset + GetBankSize()) > parent_space->GetSize()) {
		diag.Error("Register bank " + ID + ": does not fit within the bounds of its register space!", DiagNode());
		success = false;
	}

	if(ElementStride < ElementSize) {
		diag.Error("Register bank " + ID + ": Stride must be greater than element size", DiagNode());
		success = false;
	}

	if ((ElementCount * ElementStride) > RegisterStride) {
		diag.Error("Register bank " + ID + ": Element Count * Element Stride must be less than Register Stride", DiagNode());
		success = false;
	}

	genc::IRType out;
	if(!genc::IRType::ParseType(ElementTypeName, out)) {
		diag.Error("Register bank " + ID + ": Unknown type " + ElementTypeName, DiagNode());
		success = false;
	}

	return success;
}

uint32_t RegBankViewDescriptor::GetRegFileOffset() const
{
	return RegisterSpaceOffset + parent_space->GetOffsetInFile();
}

/*
 * RegSpaceDescriptor Functions
 */

RegSpaceDescriptor::RegSpaceDescriptor(RegisterFile *parent_file, uint32_t size) : size_in_bytes(size), parent_file(parent_file)
{
	assert(size > 0);
}

bool RegSpaceDescriptor::AddRegBankView(RegBankViewDescriptor *bank)
{
	if(HasView(bank->ID)) return false;
	banks.push_back(bank);
	return true;
}

bool RegSpaceDescriptor::AddRegSlotView(RegSlotViewDescriptor *slot)
{
	if(HasView(slot->GetID())) return false;
	slots.push_back(slot);
	return true;
}

bool RegSpaceDescriptor::HasRegBank(std::string ID) const
{
	for(auto &bank : banks) if(bank->ID == ID) return true;
	return false;
}

bool RegSpaceDescriptor::HasRegSlot(std::string ID) const
{
	for(auto &slot : slots) if(slot->GetID() == ID) return true;
	return false;
}

bool RegSpaceDescriptor::HasView(std::string ID) const
{
	return HasRegBank(ID) || HasRegSlot(ID);
}

uint32_t RegSpaceDescriptor::GetOffsetInFile() const
{
	return parent_file->GetOffsetOf(this);
}

uint32_t RegSpaceDescriptor::GetAlignmentRequirements() const
{
	uint32_t max_register_stride = 0;

	for (auto &bank : banks) {
		if (bank->GetRegisterStride() > max_register_stride) {
			max_register_stride = bank->GetRegisterStride();
		}
	}

	for (auto &slot : slots) {
		if (slot->GetWidth() > max_register_stride) {
			max_register_stride = slot->GetWidth();
		}
	}

	return max_register_stride;
}


bool RegSpaceDescriptor::PrettyPrint(std::ostream &str) const
{
	str << "ac_regspace (" << size_in_bytes << ") {" << std::endl;
	for(const auto &bank : banks) {
		bank->PrettyPrint(str);
		str << std::endl;
	}
	for(const auto &slot : slots) {
		slot->PrettyPrint(str);
		str << std::endl;
	}
	str << "}";
	return true;
}

bool RegSpaceDescriptor::Resolve(DiagnosticContext &diag)
{
	bool success = true;
	for(auto &bank : banks) {
		success &= bank->Resolve(diag);
	}
	for(auto &slot : slots) {
		success &= slot->Resolve(diag);
	}

	return success;
}

/*
 * RegisterFile Functions
 */

static uint32_t ALIGN_UP(uint32_t value, uint32_t alignment)
{
	if ((value % alignment) == 0) return value;
	return value + (alignment - (value % alignment));
}

uint32_t RegisterFile::GetOffsetOf(const RegSpaceDescriptor *test_space) const
{
	uint32_t total_offset = 0;
	for(const auto *space : spaces) {
		total_offset = ALIGN_UP(total_offset, space->GetAlignmentRequirements());
		if(test_space == space) {
			return total_offset;
		}
		total_offset += space->GetSize();
	}

	assert(false);
	return 0;
}

bool RegisterFile::HasSpace(RegSpaceDescriptor *space) const
{
	for(const auto my_space : spaces) {
		if(my_space == space) {
			return true;
		}
	}
	return false;
}

bool RegisterFile::AddRegisterSpace(RegSpaceDescriptor *space)
{
	if(HasSpace(space)) {
		return false;
	}
	spaces.push_back(space);
	return true;
}

RegSpaceDescriptor *RegisterFile::GetSpaceByIndex(uint32_t idx)
{
	if(idx > spaces.size()) {
		return nullptr;
	}
	return spaces[idx];
}

bool RegisterFile::AddRegisterBank(RegSpaceDescriptor *space, RegBankViewDescriptor *bank)
{
	space->AddRegBankView(bank);
	bank_views.push_back(bank);
	return true;
}

bool RegisterFile::AddRegisterSlot(RegSpaceDescriptor *space, RegSlotViewDescriptor *slot)
{
	space->AddRegSlotView(slot);
	slot_views.push_back(slot);

	if(slot->HasTag()) {
		tagged_slots[slot->GetTag()] = slot;
	}

	return true;
}

bool RegisterFile::HasRegBank(std::string ID) const
{
	for(const auto space : spaces) {
		if(space->HasRegBank(ID)) return true;
	}
	return false;
}

bool RegisterFile::HasRegSlot(std::string ID) const
{
	for(const auto space : spaces) {
		if(space->HasRegSlot(ID)) return true;
	}
	return false;
}

bool RegisterFile::HasView(std::string ID) const
{
	return HasRegBank(ID) || HasRegSlot(ID);
}

bool RegisterFile::HasTaggedRegSlot(std::string tag) const
{
	return tagged_slots.count(tag);
}

const RegSlotViewDescriptor *RegisterFile::GetTaggedRegSlot(std::string tag) const
{
	return tagged_slots.at(tag);
}

uint32_t RegisterFile::GetIndexOfSlot(const RegSlotViewDescriptor &slot) const
{
	uint32_t index = 0;
	for(index = 0; index < slot_views.size(); ++index) {
		if(slot_views.at(index) == &slot) return index;
	}
	assert(false);
	return -1;
}

uint32_t RegisterFile::GetIndexOfBank(const RegBankViewDescriptor &slot) const
{
	uint32_t index = 0;
	for(index = 0; index < bank_views.size(); ++index) {
		if(bank_views.at(index) == &slot) return index;
	}
	assert(false);
	return -1;
}

uint32_t RegisterFile::GetSize() const
{
	return spaces.back()->GetSize() + spaces.back()->GetOffsetInFile();
}

bool RegisterFile::PrettyPrint(std::ostream &str) const
{
	for(const auto space : spaces) space->PrettyPrint(str);
	return true;
}

bool RegisterFile::Resolve(DiagnosticContext &diag)
{
	std::set<std::string> names;
	std::set<std::string> tags;

	bool success = true;

	for(auto &space : spaces) {
		success &= space->Resolve(diag);
	}

	for(auto &bank : bank_views) {
		if(names.count(bank->ID)) {
			diag.Error("Register bank " + bank->ID + ": another bank or slot with this name already exists", DiagNode());
			success = false;
		}

		names.insert(bank->ID);
	}

	for(auto &slot : slot_views) {
		if(names.count(slot->GetID())) {
			diag.Error("Register slot " + slot->GetID() + ": another bank or slot with this name already exists", DiagNode());
			success = false;
		}

		if(slot->HasTag() && tags.count(slot->GetTag())) {
			diag.Error("Register slot " + slot->GetID() + ": another slot with tag " + slot->GetTag() + " already exists", DiagNode());
			success = false;
		}

		names.insert(slot->GetID());
		if(slot->HasTag()) {
			tags.insert(slot->GetTag());
		}
	}

	return success;
}
