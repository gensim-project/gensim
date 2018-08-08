/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/memory/SparseMemoryModel.h"
#include "util/ComponentManager.h"

#if CONFIG_LLVM
#include <llvm/IR/Function.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Module.h>
#endif

#include <sys/mman.h>
#include <sys/mman.h>

#define ADDRESS_SPACE_SIZE	(0x100000000)

using namespace archsim::abi::memory;

//#define FAST_SPARSE_TRANSLATION

RegisterComponent(MemoryModel, SparseMemoryModel, "sparse", "");

#if CONFIG_LLVM
SparseMemoryTranslationModel::SparseMemoryTranslationModel(SparseMemoryModel& model)  {}
SparseMemoryTranslationModel::~SparseMemoryTranslationModel() {}

bool SparseMemoryTranslationModel::PrepareTranslation(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx)
{
	return MemoryTranslationModel::PrepareTranslation(insn_ctx);
}

#ifndef FAST_SPARSE_TRANSLATION
bool SparseMemoryTranslationModel::EmitMemoryRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	return MemoryTranslationModel::EmitMemoryRead(insn_ctx, width, sx, fault, address, destinationType, destination);
}

bool SparseMemoryTranslationModel::EmitMemoryWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	return MemoryTranslationModel::EmitMemoryWrite(insn_ctx, width, fault, address, value);
}
#else
bool SparseMemoryTranslationModel::EmitMemoryRead(archsim::internal::translate::TranslationWorkUnitCtx& twu, llvm::IRBuilder<>& builder, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	// return (host_addr_t)(_page_map[address >> 12] + (unsigned long)(address & 4095));

	llvm::Value *page_map = llvm::ConstantInt::get(twu.llvm_types.i64Type, (uint64_t)memory_model._page_map, "page_map");
	page_map = builder.CreateIntToPtr(page_map, llvm::PointerType::get(twu.llvm_types.ui8PtrType, 0));

	llvm::Value *address64 = builder.CreateCast(llvm::Instruction::ZExt, address, twu.llvm_types.i64Type, "guest_address");
	llvm::Value *page_index = builder.CreateLShr(address64, PAGE_BITS, "page_index");
	llvm::Value *page_base = builder.CreateGEP(page_map, page_index, "page_map_ptr");
	twu.add_aa_node((llvm::Instruction *)page_base, TAG_SPARSE_PAGE_MAP);
	page_base = builder.CreateLoad(page_base, "host_page_base");

	llvm::Value *page_offset = builder.CreateAnd(address64, PAGE_OFFSET_MASK, "page_offset");
	llvm::Value *real_addr = builder.CreateAdd(builder.CreatePtrToInt(page_base, twu.llvm_types.i64Type), page_offset, "host_address");

	switch (width) {
		case 1:
			real_addr = builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, twu.llvm_types.ui8PtrType);
			break;
		case 2:
			real_addr = builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, twu.llvm_types.ui16PtrType);
			break;
		case 4:
			real_addr = builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, twu.llvm_types.ui32PtrType);
			break;
		case 8:
			real_addr = builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, twu.llvm_types.ui64PtrType);
			break;
		default:
			return false;
	}

	if (real_addr->getValueID() >= llvm::Value::InstructionVal)
		twu.add_aa_node((llvm::Instruction*)real_addr, TAG_MEM_ACCESS);

	llvm::Value* value = builder.CreateLoad(real_addr);
	if (sx) {
		value = builder.CreateCast(llvm::Instruction::SExt, value, destinationType);
	} else {
		value = builder.CreateCast(llvm::Instruction::ZExt, value, destinationType);
	}

	builder.CreateStore(value, destination);

	return true;
}

bool SparseMemoryTranslationModel::EmitMemoryWrite(archsim::internal::translate::TranslationWorkUnitCtx& twu, llvm::IRBuilder<>& builder, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	llvm::Value *page_map = llvm::ConstantInt::get(twu.llvm_types.i64Type, (uint64_t)memory_model._page_map, "page_map");
	page_map = builder.CreateIntToPtr(page_map, llvm::PointerType::get(twu.llvm_types.ui8PtrType, 0));

	llvm::Value *address64 = builder.CreateCast(llvm::Instruction::ZExt, address, twu.llvm_types.i64Type, "guest_address");
	llvm::Value *page_index = builder.CreateLShr(address64, PAGE_BITS, "page_index");
	llvm::Value *page_base = builder.CreateGEP(page_map, page_index, "page_map_ptr");
	twu.add_aa_node((llvm::Instruction *)page_base, TAG_SPARSE_PAGE_MAP);
	page_base = builder.CreateLoad(page_base, "host_page_base");

	llvm::Value *page_offset = builder.CreateAnd(address64, PAGE_OFFSET_MASK, "page_offset");
	llvm::Value *real_addr = builder.CreateAdd(builder.CreatePtrToInt(page_base, twu.llvm_types.i64Type), page_offset, "host_address");

	switch (width) {
		case 1:
			real_addr = builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, twu.llvm_types.ui8PtrType);
			value = builder.CreateCast(llvm::Instruction::ZExt, value, twu.llvm_types.i8Type);
			break;
		case 2:
			real_addr = builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, twu.llvm_types.ui16PtrType);
			value = builder.CreateCast(llvm::Instruction::ZExt, value, twu.llvm_types.i16Type);
			break;
		case 4:
			real_addr = builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, twu.llvm_types.ui32PtrType);
			value = builder.CreateCast(llvm::Instruction::ZExt, value, twu.llvm_types.i32Type);
			break;
		case 8:
			real_addr = builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, twu.llvm_types.ui64PtrType);
			value = builder.CreateCast(llvm::Instruction::ZExt, value, twu.llvm_types.i64Type);
			break;
	}

	if (real_addr->getValueID() >= llvm::Value::InstructionVal)
		twu.add_aa_node((llvm::Instruction*)real_addr, TAG_MEM_ACCESS);
	builder.CreateStore(value, real_addr);

	return true;
}
#endif
#endif

SparseMemoryModel::SparseMemoryModel() : prev_page_data_(nullptr)
{
}

SparseMemoryModel::~SparseMemoryModel()
{

}

bool SparseMemoryModel::SynchroniseVMAProtection(GuestVMA& vma)
{
	return true;
}

bool SparseMemoryModel::LockRegion(guest_addr_t guest_addr, guest_size_t guest_size, host_addr_t& host_addr)
{
	if(guest_addr.GetPageIndex() != (guest_addr + guest_size - 1).GetPageIndex()) {
		return false;
	}

	auto page = GetPage(guest_addr);
	host_addr = page + guest_addr.GetPageOffset();
	return true;
}

bool SparseMemoryModel::LockRegions(guest_addr_t guest_addr, guest_size_t guest_size, LockedMemoryRegion& regions)
{
	std::vector<void*> ptrs;

	// lock each page
	guest_addr = guest_addr.PageBase();
	for(Address addr = guest_addr; addr < (guest_addr + guest_size); addr += Address::PageSize) {
		void *ptr;
		if(!LockRegion(addr, Address::PageSize, ptr)) {
			return false;
		}

		ptrs.push_back(ptr);
	}

	regions = LockedMemoryRegion(guest_addr, ptrs);
	return true;
}


bool SparseMemoryModel::UnlockRegion(guest_addr_t guest_addr, guest_size_t guest_size, host_addr_t host_addr)
{
	return false;
}

bool SparseMemoryModel::Initialise()
{

#if CONFIG_LLVM
	translation_model = new SparseMemoryTranslationModel(*this);
#endif
	return true;
}

void SparseMemoryModel::Destroy()
{

}

MemoryTranslationModel& SparseMemoryModel::GetTranslationModel()
{
#if CONFIG_LLVM
	return *translation_model;
#else
	assert(false);
#endif
}

bool SparseMemoryModel::AllocateVMA(GuestVMA &vma)
{
	// HACK:
	return true;
}

bool SparseMemoryModel::DeallocateVMA(GuestVMA &vma)
{

	return true;
}

bool SparseMemoryModel::ResizeVMA(GuestVMA &vma, guest_size_t new_size)
{
//	vma.host_base = addr;
	vma.size = new_size;

	return true;
}

char* SparseMemoryModel::GetPage(Address addr)
{
	std::lock_guard<std::mutex> lock_guard(map_lock_);

	if(prev_page_data_ && addr.PageBase() == prev_page_base_) {
		return prev_page_data_;
	}

	char *ptr = nullptr;
	if(!data_.count(addr.PageBase())) {
		ptr = (char*)malloc(4096);
		if(ptr == nullptr) {
			throw std::bad_alloc();
		}

		bzero(ptr, 4096);

		data_[addr.PageBase()] = ptr;
	} else {
		ptr = data_.at(addr.PageBase());
	}

	prev_page_base_ = addr.PageBase();
	prev_page_data_ = ptr;

	return ptr;
}


uint32_t SparseMemoryModel::Read(guest_addr_t addr, uint8_t *data, int size)
{
//	RaiseEvent(MemoryModel::MemEventRead, addr, size);
	if((addr + size - 1).PageBase() != addr.PageBase()) {
		for(int i = 0; i < size; ++i) {
			auto result = Read(addr + i, data + i, 1);
			if(result) {
				return result;
			}
		}
		return 0;
	}

	auto offset = addr.GetPageOffset();

	memcpy(data, GetPage(addr) + offset, size);

	return 0;
}

uint32_t SparseMemoryModel::Fetch(guest_addr_t addr, uint8_t *data, int size)
{
//	RaiseEvent(MemoryModel::MemEventFetch, addr, size);
	if((addr + size).PageBase() != addr.PageBase()) {
		UNIMPLEMENTED;
	}

	auto &page = data_[addr.PageBase()];
	auto offset = addr.GetPageOffset();

	memcpy(data, GetPage(addr) + offset, size);

	return 0;
}

uint32_t SparseMemoryModel::Write(guest_addr_t addr, uint8_t *data, int size)
{
//	RaiseEvent(MemoryModel::MemEventWrite, addr, size);
	if((addr + size - 1).PageBase() != addr.PageBase()) {
		for(int i = 0; i < size; ++i) {
			auto result = Write(addr + i, data + i, 1);
			if(result) {
				return result;
			}
		}
		return 0;
	}

	auto page = GetPage(addr);
	auto offset = addr.GetPageOffset();

	memcpy(page + offset, data, size);
	return 0;
}

uint32_t SparseMemoryModel::Peek(guest_addr_t addr, uint8_t *data, int size)
{
	UNIMPLEMENTED;
	return 1;
}

uint32_t SparseMemoryModel::Poke(guest_addr_t addr, uint8_t *data, int size)
{
	Write(addr, data, size);
}
