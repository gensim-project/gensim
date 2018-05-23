/*
 * Transform.h
 *
 *  Created on: 7 Oct 2015
 *      Author: harry
 */

#ifndef BLOCKJIT_BLOCK_COMPILER_TRANSFORM_H_
#define BLOCKJIT_BLOCK_COMPILER_TRANSFORM_H_

#include "blockjit/translation-context.h"
#include "util/vbitset.h"

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

				class RegisterAllocationTransform : public Transform
				{
				public:
					RegisterAllocationTransform(uint32_t num_allocable_registers);
					virtual ~RegisterAllocationTransform();
					virtual bool Apply(TranslationContext &ctx) override;
					
					uint32_t GetStackFrameSize() const;
					archsim::util::vbitset GetUsedPhysRegs() const;
					
				private:
					uint32_t stack_frame_size_;
					uint32_t number_allocable_registers_;
					archsim::util::vbitset used_phys_regs_;
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
