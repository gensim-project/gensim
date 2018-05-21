/*
 * File:   LLVMTranslationContext.h
 * Author: s0457958
 *
 * Created on 16 July 2014, 16:15
 */

#ifndef LLVMTRANSLATIONCONTEXT_H
#define	LLVMTRANSLATIONCONTEXT_H

#include "define.h"

#include "translate/TranslationContext.h"
#include "translate/llvm/LLVMAliasAnalysis.h"
#include "translate/llvm/LLVMOptimiser.h"

#include "util/PubSubSync.h"

#include "util/Histogram.h"
#include "util/Counter.h"
#include "util/PagePool.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

#include <set>

namespace llvm
{
	class Function;
	class ExecutionEngine;
}

namespace gensim
{
	class BaseDecode;
	class BaseLLVMTranslate;
}

namespace archsim
{
	namespace util
	{
		class Counter64;
	}
	namespace translate
	{
		namespace translate_llvm
		{

			class LLVMTranslationContext;
			class LLVMRegionTranslationContext : public RegionTranslationContext<LLVMTranslationContext>
			{
			public:
				enum LeaveReasons {
					Normal = 0,
					RequireInterpret = 1,
					UnableToChain = 2,
					InterruptActionAbort = 3,
					ExceptionExit = 4,
					NoIndirectTarget = 5,
					NoBlockAvailable = 6,
				};

				LLVMRegionTranslationContext(LLVMTranslationContext& parent, ::llvm::IRBuilder<>& builder);

				::llvm::IRBuilder<>& builder;
				::llvm::Function *region_fn;
				::llvm::BasicBlock *entry_block;
				::llvm::BasicBlock *dispatch_block;
				::llvm::BasicBlock *chain_block;
				::llvm::BasicBlock *interrupt_handler_block;

				struct {
					::llvm::Value *cpu_ctx_val;
					::llvm::Argument *state_val;
					::llvm::Value *reg_state_val;
					::llvm::Value *region_txln_cache_ptr_val;

					::llvm::Value *mem_read_temp_8;
					::llvm::Value *mem_read_temp_16;
					::llvm::Value *mem_read_temp_32;

					::llvm::Value *virt_page_base;
					::llvm::Value *virt_page_idx;

					std::vector<std::vector< ::llvm::Value*> > banked_register_pointer_matrix;
					std::vector< ::llvm::Value*> register_pointer_array;
				} values;

				::llvm::GlobalVariable *jump_table;
				::llvm::Constant *jump_table_entries[2048];

				::llvm::BasicBlock *CreateBlock(std::string name);
				void EmitCounterUpdate(::llvm::IRBuilder<> &builder, archsim::util::Counter64& counter, int64_t increment);
				void EmitCounterPointerUpdate(::llvm::Value *ptr, int64_t increment);
				void EmitHistogramUpdate(archsim::util::Histogram& histogram, uint32_t idx, int64_t increment);
				void EmitHistogramUpdate(archsim::util::Histogram& histogram, ::llvm::Value* idx, int64_t increment);

				void EmitPublishEvent(PubSubType::PubSubType type, std::vector<::llvm::Value*> data);

				void EmitLeaveInstruction(LeaveReasons reason, ::llvm::Value *pc_offset, ::llvm::Value *condition = NULL, ::llvm::BasicBlock *cont_block = NULL, bool invert = false);
				void EmitLeaveInstruction(LeaveReasons reason, addr_t pc_offset, ::llvm::Value *condition = NULL, ::llvm::BasicBlock *cont_block = NULL, bool invert = false);
				void EmitLeaveInstructionTrace(LeaveReasons reason, ::llvm::Value *pc_offset, ::llvm::Value *condition = NULL, ::llvm::BasicBlock *cont_block = NULL, bool invert = false);

				void EmitTakeException(addr_t pc_offset, ::llvm::Value *arg0, ::llvm::Value *arg1);
				void EmitTakeException(::llvm::Value *pc_offset, ::llvm::Value *arg0, ::llvm::Value *arg1);

				void EmitLeave(LeaveReasons reason, ::llvm::Value *condition = NULL, ::llvm::BasicBlock *cont_block = NULL, bool invert = false);

				void EmitStackMap(uint64_t id);

				::llvm::Value *CreateMaterialise(::llvm::Value *offset);
				::llvm::Value *CreateMaterialise(uint32_t offset);

				::llvm::BasicBlock* GetExitBlock(LeaveReasons reason);
				::llvm::BasicBlock* GetInstructionExitBlock(LeaveReasons reason);

				::llvm::Value *GetSlot(std::string name, ::llvm::Type *type);

				// Register slots
				// Banked registers have index < bank index, register index >
				// Register slots have index < -1, slot index >
				std::map<std::pair<int32_t, uint32_t>, ::llvm::Value*> RegSlots;
			private:
				std::map<std::string, ::llvm::Value*> Slots;

				std::map<LeaveReasons, ::llvm::BasicBlock *> exit_blocks;
				std::map<LeaveReasons, ::llvm::BasicBlock *> exit_instruction_blocks;
				std::map<LeaveReasons, ::llvm::PHINode *> exit_instruction_phis;

				bool CreateExitBlock(LLVMRegionTranslationContext::LeaveReasons reason);
				bool CreateInstructionExitBlock(LLVMRegionTranslationContext::LeaveReasons reason);

				/*::llvm::Value *CreateReadPC();
				void CreateWritePC(::llvm::Value *new_pc);*/
			};

			class LLVMTranslationContext : public TranslationContext
			{
			public:
				LLVMTranslationContext(TranslationManager& tmgr, TranslationWorkUnit& twu, ::llvm::LLVMContext& llvm_ctx, LLVMOptimiser &opt, archsim::util::PagePool &code_pool);
				~LLVMTranslationContext();

				archsim::util::PagePool &code_pool;

				gensim::BaseLLVMTranslate *gensim_translate;

				::llvm::LLVMContext& llvm_ctx;
				::llvm::Module *llvm_module;
				LLVMOptimiser &optimiser;

				struct {
					::llvm::Function *cpu_read_pc;
					::llvm::Function *cpu_write_pc;

					::llvm::Function *cpu_translate;
					::llvm::Function *cpu_handle_pending_action;
					::llvm::Function *cpu_return_to_safepoint;

					::llvm::Function *cpu_read_8;
					::llvm::Function *cpu_read_16;
					::llvm::Function *cpu_read_32;
					::llvm::Function *cpu_write_8;
					::llvm::Function *cpu_write_16;
					::llvm::Function *cpu_write_32;

					::llvm::Function *cpu_read_8_user;
					::llvm::Function *cpu_read_32_user;
					::llvm::Function *cpu_write_8_user;
					::llvm::Function *cpu_write_32_user;

					::llvm::Function *mem_read_8;
					::llvm::Function *mem_read_16;
					::llvm::Function *mem_read_32;

					::llvm::Function *mem_write_8;
					::llvm::Function *mem_write_16;
					::llvm::Function *mem_write_32;

					::llvm::Function *cpu_take_exception;
					::llvm::Function *cpu_push_interrupt;
					::llvm::Function *cpu_pop_interrupt;
					::llvm::Function *cpu_pend_irq;
					::llvm::Function *cpu_exec_mode;

					::llvm::Function *cpu_enter_kernel;
					::llvm::Function *cpu_enter_user;

					::llvm::Function *cpu_halt;

					::llvm::Function *dev_probe_device;
					::llvm::Function *dev_read_device;
					::llvm::Function *dev_write_device;

					::llvm::Function *debug0;
					::llvm::Function *debug1;
					::llvm::Function *debug2;
					::llvm::Function *debug_cpu;
					::llvm::Function *jit_trap;
					::llvm::Function *jit_trap_if;
					::llvm::Function *jit_assert;

					::llvm::Function *jit_checksum_page;
					::llvm::Function *jit_check_checksum;

					::llvm::Function *trace_start_insn;
					::llvm::Function *trace_end_insn;

					::llvm::Function *trace_reg_write;
					::llvm::Function *trace_reg_bank_write;
					::llvm::Function *trace_reg_read;
					::llvm::Function *trace_reg_bank_read;

					::llvm::Function *trace_mem_read_8;
					::llvm::Function *trace_mem_read_16;
					::llvm::Function *trace_mem_read_32;
					::llvm::Function *trace_mem_write_8;
					::llvm::Function *trace_mem_write_16;
					::llvm::Function *trace_mem_write_32;

					::llvm::Function *smart_return;

					::llvm::Function *sys_verify;
					::llvm::Function *sys_publish_insn;
					::llvm::Function *sys_publish_block;

					::llvm::Function *tm_flush;
					::llvm::Function *tm_flush_itlb;
					::llvm::Function *tm_flush_dtlb;
					::llvm::Function *tm_flush_itlb_entry;
					::llvm::Function *tm_flush_dtlb_entry;

					::llvm::Function *double_sqrt;
					::llvm::Function *float_sqrt;
					::llvm::Function *double_abs;
					::llvm::Function *float_abs;

					::llvm::Function *genc_adc_flags;
				} jit_functions;

				struct {
					::llvm::Function *stack_map;
				} intrinsics;

				struct {
					::llvm::Type *vtype;

					::llvm::Type *i1;
					::llvm::Type *i8;
					::llvm::Type *i16;
					::llvm::Type *i32;
					::llvm::Type *i64;

					::llvm::Type *pi1;
					::llvm::Type *pi8;
					::llvm::Type *pi16;
					::llvm::Type *pi32;
					::llvm::Type *pi64;

					::llvm::Type *f32;
					::llvm::Type *f64;

					::llvm::Type *state;
					::llvm::Type *state_ptr;

					::llvm::Type *reg_state;
					::llvm::Type *reg_state_ptr;

					::llvm::Type *cpu_ctx;

					::llvm::Type *cache_entry;
					::llvm::Type *cache_entry_ptr;
				} types;

				bool Translate(Translation*& translation, TranslationTimers& timers);

				inline ::llvm::BasicBlock *GetLLVMBlock(virt_addr_t virt_addr)
				{
					return llvm_block_map[virt_addr];
				}

				inline void AddIndirectTarget(TranslationBlockUnit& block)
				{
					indirect_targets.insert(&block);
				}

				void AddAliasAnalysisNode(::llvm::Instruction *insn, AliasAnalysisTag tag);
				void AddAliasAnalysisToRegSlotAccess(::llvm::Value *ptr, uint32_t slot_offset, uint32_t slot_size);
				void AddAliasAnalysisToRegBankAccess(::llvm::Value *ptr, ::llvm::Value *regnum, uint32_t bank_offset, uint32_t bank_stride, uint32_t bank_elements);

				::llvm::Value *GetConstantInt8(uint8_t v);
				::llvm::Value *GetConstantInt16(uint16_t v);
				::llvm::Value *GetConstantInt32(uint32_t v);
				::llvm::Value *GetConstantInt64(uint64_t v);

			private:
				bool CreateLLVMFunction(LLVMRegionTranslationContext& rtc);
				bool CreateDefaultBlocks(LLVMRegionTranslationContext& rtc);

				bool BuildEntryBlock(LLVMRegionTranslationContext& rtc, uint32_t constant_page_base);
				bool BuildChainBlock(LLVMRegionTranslationContext& rtc);
				bool BuildInterruptHandlerBlock(LLVMRegionTranslationContext& rtc);
				bool PopulateBlockMap(LLVMRegionTranslationContext& rtc);

				::llvm::Function *GetMemReadFunction(uint32_t width);
				::llvm::Function *GetMemWriteFunction(uint32_t width);

				bool Compile(::llvm::Function *fn, Translation*& translation, TranslationTimers& timers);
				bool Optimise(::llvm::ExecutionEngine *engine, ::llvm::Function *region_fn, TranslationTimers& timers);

				void DumpFunctionGraph(::llvm::Function *fn, std::string filename);

				std::map<uint32_t, ::llvm::BasicBlock *> llvm_block_map;
				std::set<TranslationBlockUnit *> indirect_targets;

				void Initialise();

				void SaveTranslation();
				bool LoadTranslation(Translation*& translation);

				bool CreateDispatcher(LLVMRegionTranslationContext& rtc, ::llvm::BasicBlock *dispatcher_block);
			};

			class LLVMBlockTranslationContext : public BlockTranslationContext<LLVMRegionTranslationContext>
			{
			public:
				LLVMBlockTranslationContext(LLVMRegionTranslationContext& parent, TranslationBlockUnit& tbu, ::llvm::BasicBlock *llvm_block);
				bool Translate();

				::llvm::BasicBlock *llvm_block;

			private:
				bool EmitInterruptCheck();
				bool EmitDirectJump(virt_addr_t jump_target, virt_addr_t fallthrough_target, bool predicated);
				bool EmitIndirectJump(virt_addr_t jump_target, virt_addr_t fallthrough_target, bool predicated);
				bool EmitSpanningControlFlow(TranslationInstructionUnit& last_insn);
			};

			class LLVMInstructionTranslationContext : public InstructionTranslationContext<LLVMBlockTranslationContext>
			{
			public:
				LLVMInstructionTranslationContext(LLVMBlockTranslationContext& parent, TranslationInstructionUnit& tiu);
				bool Translate();

				void AddAliasAnalysisNode(::llvm::Instruction *insn, AliasAnalysisTag tag);

			private:
				bool TranslatePredicated();
				bool TranslateNonPredicated();
			};
		}
	}
}

#endif	/* LLVMTRANSLATIONCONTEXT_H */

