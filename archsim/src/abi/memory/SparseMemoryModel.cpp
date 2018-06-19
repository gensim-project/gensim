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

SparseMemoryModel::SparseMemoryModel() : _page_map(NULL)
{
}

SparseMemoryModel::~SparseMemoryModel()
{
	if (_page_map)
		free(_page_map);
}

bool SparseMemoryModel::Initialise()
{
	_page_map = (void **)calloc(ADDRESS_SPACE_SIZE / archsim::RegionArch::PageSize, sizeof(void *));
	if (_page_map == NULL)
		return false;

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

void SparseMemoryModel::SyncPageMap(GuestVMA &vma)
{
	Address guest_addr = vma.base;
	unsigned long host_addr = (unsigned long)vma.host_base;
	while (guest_addr < vma.base + vma.size) {
		_page_map[guest_addr.GetPageIndex()] = (void *)host_addr;

		guest_addr += archsim::RegionArch::PageSize;
		host_addr += archsim::RegionArch::PageSize;
	}
}

void SparseMemoryModel::ErasePageMap(GuestVMA &vma)
{
	Address guest_addr = vma.base;
	while (guest_addr < vma.base + vma.size) {
		_page_map[guest_addr.GetPageIndex()] = NULL;
		guest_addr += archsim::RegionArch::PageSize;
	}
}

bool SparseMemoryModel::AllocateVMA(GuestVMA &vma)
{
	vma.host_base = mmap(NULL, vma.size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (vma.host_base != MAP_FAILED) {
		SyncPageMap(vma);
		return true;
	} else {
		return false;
	}
}

bool SparseMemoryModel::DeallocateVMA(GuestVMA &vma)
{
	munmap(vma.host_base, vma.size);
	ErasePageMap(vma);

	return true;
}

bool SparseMemoryModel::ResizeVMA(GuestVMA &vma, guest_size_t new_size)
{
	void *addr = mremap(vma.host_base, vma.size, new_size, MREMAP_MAYMOVE);
	if (addr == MAP_FAILED)
		return false;

	vma.host_base = addr;
	vma.size = new_size;

	SyncPageMap(vma);
	return true;
}

bool SparseMemoryModel::SynchroniseVMAProtection(GuestVMA &vma)
{
	return true;
}

uint32_t SparseMemoryModel::Read(guest_addr_t addr, uint8_t *data, int size)
{
	RaiseEvent(MemoryModel::MemEventRead, addr, size);

	void *host = (void *)((unsigned long)_page_map[addr.GetPageIndex()] + (unsigned long)addr.GetPageOffset());
	memcpy(data, host, size);

	return 0;
}

uint32_t SparseMemoryModel::Fetch(guest_addr_t addr, uint8_t *data, int size)
{
	RaiseEvent(MemoryModel::MemEventFetch, addr, size);

	void *host = (void *)((unsigned long)_page_map[addr.GetPageIndex()] + (unsigned long)addr.GetPageOffset());
	memcpy(data, host, size);

	return 0;
}

uint32_t SparseMemoryModel::Write(guest_addr_t addr, uint8_t *data, int size)
{
	RaiseEvent(MemoryModel::MemEventWrite, addr, size);

	void *host = (void *)((unsigned long)_page_map[addr.GetPageIndex()] + (unsigned long)addr.GetPageOffset());
	memcpy(host, data, size);

	return 0;
}

uint32_t SparseMemoryModel::Peek(guest_addr_t addr, uint8_t *data, int size)
{
	return 1;
}

uint32_t SparseMemoryModel::Poke(guest_addr_t addr, uint8_t *data, int size)
{
	return 1;
}
