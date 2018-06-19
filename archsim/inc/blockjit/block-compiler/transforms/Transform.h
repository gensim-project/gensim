/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * Transform.h
 *
 *  Created on: 7 Oct 2015
 *      Author: harry
 */

#ifndef BLOCKJIT_BLOCK_COMPILER_TRANSFORM_H_
#define BLOCKJIT_BLOCK_COMPILER_TRANSFORM_H_

#include "blockjit/translation-context.h"
#include <wutils/vbitset.h>

#include <map>
#include <vector>

namespace captive
{
	namespace arch
	{
		namespace jit
		{

			namespace transforms
			{

				class Transform
				{
				public:
					virtual ~Transform();
					virtual bool Apply(TranslationContext& ctx) = 0;
				}; 
				
				class AllocationWriterTransform : public Transform
				{
				public:
					typedef std::map<captive::shared::IRRegId, std::pair<captive::shared::IROperand::IRAllocationMode, uint16_t>> allocations_t;
					
					AllocationWriterTransform(const allocations_t &allocations);
					virtual ~AllocationWriterTransform();
					virtual bool Apply(TranslationContext &ctx) override;
					
				private:
					allocations_t allocations_;
				};

				class MovEliminationTransform : public Transform
				{
				public:
					virtual ~MovEliminationTransform();
					virtual bool Apply(TranslationContext &ctx) override;
					
				};
				
				class DeadStoreElimination : public Transform
				{
				public:
					virtual ~DeadStoreElimination();
					virtual bool Apply(TranslationContext &ctx) override;
				};
				
				class GlobalRegisterReuseTransform : public Transform
				{
				public:
					GlobalRegisterReuseTransform(const wutils::vbitset &used_registers, int max_regs);
					virtual ~GlobalRegisterReuseTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				private:
					wutils::vbitset used_pregs_;
					int max_regs_;
				};
				
				class GlobalRegisterAllocationTransform : public Transform
				{
				public:
					GlobalRegisterAllocationTransform(uint32_t num_allocable_registers);
					virtual ~GlobalRegisterAllocationTransform();
					virtual bool Apply(TranslationContext &ctx) override;
					
					uint32_t GetStackFrameSize() const;
					wutils::vbitset GetUsedPhysRegs() const;
				private:
					uint32_t stack_frame_size_;
					uint32_t num_allocable_registers_;
					wutils::vbitset used_phys_regs_;
				};
				
				class RegisterAllocationTransform : public Transform
				{
				public:
					RegisterAllocationTransform(uint32_t num_allocable_registers);
					virtual ~RegisterAllocationTransform();
					virtual bool Apply(TranslationContext &ctx) override;
					
					uint32_t GetStackFrameSize() const;
					wutils::vbitset GetUsedPhysRegs() const;
					
				private:
					uint32_t stack_frame_size_;
					uint32_t number_allocable_registers_;
					wutils::vbitset used_phys_regs_;
				};
				
				class StackToRegTransform : public Transform
				{
				public:
					StackToRegTransform(wutils::vbitset used_phys_regs);
					virtual ~StackToRegTransform();
					
					virtual bool Apply(TranslationContext &ctx) override;
					wutils::vbitset GetUsedPhysRegs() const;
					
				private:
					wutils::vbitset used_phys_regs_;
				};
				
				class ReorderBlocksTransform : public Transform
				{
				public:
					virtual ~ReorderBlocksTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class ThreadJumpsTransform : public Transform
				{
				public:
					virtual ~ThreadJumpsTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class MergeBlocksTransform : public Transform
				{
				public:
					virtual ~MergeBlocksTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				protected:
					virtual bool merge_block(TranslationContext &ctx, captive::shared::IRBlockId merge_from, captive::shared::IRBlockId merge_into);
				};

				class SortIRTransform : public Transform
				{
				public:
					virtual ~SortIRTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class DeadBlockEliminationTransform : public Transform
				{
				public:
					virtual ~DeadBlockEliminationTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class ConstantPropTransform : public Transform
				{
				public:
					virtual ~ConstantPropTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class PeepholeTransform : public Transform
				{
				public:
					virtual ~PeepholeTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class RegValueReuseTransform : public Transform
				{
				public:
					virtual ~RegValueReuseTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class RegStoreEliminationTransform : public Transform
				{
				public:
					virtual ~RegStoreEliminationTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};


				class Peephole2Transform : public Transform
				{
				public:
					virtual ~Peephole2Transform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class ValueRenumberingTransform : public Transform
				{
				public:
					virtual ~ValueRenumberingTransform();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class PostAllocateDCE : public Transform
				{
				public:
					virtual ~PostAllocateDCE();
					virtual bool Apply(TranslationContext &ctx) override;
				};

				class PostAllocatePeephole : public Transform
				{
				public:
					virtual ~PostAllocatePeephole();
					virtual bool Apply(TranslationContext &ctx) override;
				};

			};

		}
	}
}



#endif /* BLOCKJIT_BLOCK_COMPILER_TRANSFORM_H_ */
