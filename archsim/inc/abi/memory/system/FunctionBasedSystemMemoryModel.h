/*
 * FunctionBasedSystemMemoryModel.h
 *
 *  Created on: 22 Apr 2015
 *      Author: harry
 */

#ifndef FUNCTIONBASEDSYSTEMMEMORYMODEL_H_
#define FUNCTIONBASEDSYSTEMMEMORYMODEL_H_

#include "abi/memory/MemoryModel.h"
#include "abi/memory/system/BaseSystemMemoryModel.h"
#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "translate/profile/RegionArch.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "util/PubSubSync.h"
#include "util/LogContext.h"
#include "util/RawZoneMap.h"
#include <stdio.h>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class MemoryComponent;
			class DeviceManager;
			class MMU;
		}
		namespace memory
		{
			class SystemMemoryTranslationModel;
			class FunctionGenSystemMemoryTranslationModel;

			class FunctionBasedSystemMemoryModel : public SystemMemoryModel
			{
			public:
				struct memfunc_rval {
					uint32_t fault;
					uint8_t *ptr;
				};

				typedef memfunc_rval (*read_handler_t)(cpuState *, uint32_t);
				typedef memfunc_rval (*translate_handler_t)(cpuState *, uint32_t, uint64_t);
				typedef memfunc_rval (*write_handler_t)(cpuState *, uint32_t);

				friend class FunctionGenSystemMemoryTranslationModel;

				FunctionBasedSystemMemoryModel(MemoryModel *phys_mem, util::PubSubContext *pubsub);
				~FunctionBasedSystemMemoryModel();

				bool Initialise() override;
				void Destroy() override;

				void FlushCaches() override;
				void EvictCacheEntry(virt_addr_t virt_addr) override;

				MemoryTranslationModel &GetTranslationModel() override;

				uint32_t Read(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Fetch(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Write(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Peek(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Poke(guest_addr_t guest_addr, uint8_t *data, int size) override;

				uint32_t Read8_zx(guest_addr_t addr, uint32_t &data) override;
				uint32_t Read32(guest_addr_t addr, uint32_t &data) override;
				uint32_t Write32(guest_addr_t addr, uint32_t data) override;

				uint32_t Read8User(guest_addr_t guest_addr, uint32_t&data) override;
				uint32_t Read32User(guest_addr_t guest_addr, uint32_t&data) override;
				uint32_t Write8User(guest_addr_t guest_addr, uint8_t data) override;
				uint32_t Write32User(guest_addr_t guest_addr, uint32_t data) override;

				uint32_t Fetch32(guest_addr_t guest_addr, uint32_t &data) override;

				bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr) override;

				uint32_t PerformTranslation(virt_addr_t virt_addr, phys_addr_t &out_phys_addr, const struct archsim::abi::devices::AccessInfo &info) override;


				read_handler_t GenerateAndInstallReadFunction(gensim::Processor *cpu, uint32_t address, bool is_user_only);
				write_handler_t GenerateAndInstallWriteFunction(gensim::Processor *cpu, uint32_t address, bool is_user_only);

				void InstallCaches();

				bool Complete;

			private:
				FunctionGenSystemMemoryTranslationModel *translation_model;
				BaseSystemMemoryModel base_mem;

				bool dirty;


				read_handler_t *read_cache;
				uint32_t read_cache_size;
				write_handler_t *write_cache;
				uint32_t write_cache_size;

				read_handler_t *read_user_cache;
				uint32_t read_user_cache_size;
				write_handler_t *write_user_cache;
				uint32_t write_user_cache_size;

				read_handler_t GenerateAndInstallColdReadFunction(gensim::Processor *cpu, uint32_t address);

				void check_overflow();

				// Zone for storing generated functions
				util::RawZoneMap<uint32_t, 64, 1024*1024> r_fn_zone;
				util::RawZoneMap<uint32_t, 64, 1024*1024> w_fn_zone;
				util::RawZoneMap<uint32_t, 64, 1024*1024> r_user_fn_zone;
				util::RawZoneMap<uint32_t, 64, 1024*1024> w_user_fn_zone;

				util::PubSubscriber _subscriber;
			};

#if CONFIG_LLVM
			class FunctionGenSystemMemoryTranslationModel : public SystemMemoryTranslationModel
			{
			public:

				FunctionGenSystemMemoryTranslationModel();

				virtual bool Initialise(SystemMemoryModel &mem_model) override;

				bool EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination) override;
				bool EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value) override;

				bool EmitNonPrivilegedRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, ::llvm::Value*& fault, ::llvm::Value *address, ::llvm::Type *destinationType, ::llvm::Value *destination) override;
				bool EmitNonPrivilegedWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, ::llvm::Value*& fault, ::llvm::Value *address, ::llvm::Value *value) override;

				bool EmitPerformTranslation(archsim::translate::llvm::LLVMRegionTranslationContext &ctx, llvm::Value *virt_addr, llvm::Value *&phys_addr, llvm::Value *&fault) override;

				void Flush() override;
				void Evict(virt_addr_t virt_addr) override;

				// This indicates whether the translation model should create one function for every page in memory, or act as a cache
				// Right now only Complete (one function per page) mode is supported
				bool Complete;

				util::Histogram fn_histogram;

			private:


				llvm::Value *GetReadFunctionTable(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx);
				llvm::Value *GetWriteFunctionTable(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx);

				llvm::Value *GetReadUserFunctionTable(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx);
				llvm::Value *GetWriteUserFunctionTable(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx);

				llvm::Value *GetFunFor(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx, llvm::Value *fn_table, llvm::Value *address);
				llvm::Value *CallFn(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx, llvm::Value *fn, llvm::Value *address);
				llvm::Value *GetFault(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx, llvm::Value *rval);
				llvm::Value *GetHostPtr(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx, llvm::Value *rval);


				FunctionBasedSystemMemoryModel *mem_model;
				util::PubSubscriber *subscriber;
			};
#endif
		}
	}
}


#endif /* FUNCTIONBASEDSYSTEMMEMORYMODEL_H_ */
