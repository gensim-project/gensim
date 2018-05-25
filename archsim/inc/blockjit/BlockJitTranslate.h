/*
 * BlockJitTranslate.h
 *
 *  Created on: 20 Aug 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCKJITTRANSLATE_H_
#define INC_BLOCKJIT_BLOCKJITTRANSLATE_H_

#include "abi/Address.h"
#include "translate/profile/Region.h"
#include "blockjit/translation-context.h"
#include "blockjit/IRBuilder.h"
#include "core/thread/ProcessorFeatures.h"

#include "arch/arm/ARMDecodeContext.h"

#include <unordered_set>
#include <set>

namespace captive
{
	namespace shared
	{
		class IROperand;
	}
}

namespace archsim
{
	namespace blockjit
	{
		class BlockTranslation;
	}
}

namespace wulib
{
	class MemAllocator;
}

namespace gensim
{
	class BaseDecode;
	class BlockJitProcessor;
	class BaseJumpInfo;

	namespace blockjit
	{
		class BaseBlockJITTranslate
		{
		public:
			BaseBlockJITTranslate();
			virtual ~BaseBlockJITTranslate();

			bool translate_block(archsim::core::thread::ThreadInstance *processor, archsim::Address block_address, archsim::blockjit::BlockTranslation &out_txln, wulib::MemAllocator &allocator);

			void setSupportChaining(bool enable);
			void setSupportProfiling(bool enable);
			void setTranslationMgr(archsim::translate::TranslationManager *txln_mgr);

			void InitialiseFeatures(const archsim::core::thread::ThreadInstance *cpu);
			void SetFeatureLevel(uint32_t feature, uint32_t level, captive::shared::IRBuilder& builder);
			uint32_t GetFeatureLevel(uint32_t feature);
			void InvalidateFeatures();
			archsim::ProcessorFeatureSet GetProcessorFeatures() const;
			void AttachFeaturesTo(archsim::blockjit::BlockTranslation &txln) const;

			void InitialiseIsaMode(const archsim::core::thread::ThreadInstance *cpu);
			void SetIsaMode(const captive::shared::IROperand&, captive::shared::IRBuilder& builder);
			uint32_t GetIsaMode();
			void InvalidateIsaMode();

			bool build_block(archsim::core::thread::ThreadInstance *cpu, archsim::Address block_address, captive::shared::IRBuilder &builder);
			virtual bool translate_instruction(const BaseDecode* decode_obj, captive::shared::IRBuilder& builder, bool trace) = 0;
			
			bool emit_instruction(archsim::core::thread::ThreadInstance *cpu, archsim::Address pc, gensim::BaseDecode *insn, captive::shared::IRBuilder &builder);
			bool emit_instruction_decoded(archsim::core::thread::ThreadInstance *cpu, archsim::Address pc, const gensim::BaseDecode *insn, captive::shared::IRBuilder &builder);
			
			void SetDecodeContext(gensim::DecodeContext *dec) { _decode_ctx = dec; }
			
		private:
			
			std::map<uint32_t, uint32_t> _feature_levels;
			std::map<uint32_t, uint32_t> _initial_feature_levels;
			std::set<uint32_t> _read_feature_levels;
			bool _features_valid;

			uint32_t _isa_mode;
			bool _isa_mode_valid;

			//ARM HAX
			gensim::DecodeContext *_decode_ctx;
			gensim::DecodeTranslateContext *decode_txlt_ctx;

			bool _supportChaining;
			gensim::BaseDecode *_decode;
			gensim::BaseJumpInfo *_jumpinfo;

			archsim::translate::TranslationManager *_txln_mgr;
			bool _supportProfiling;

			bool _should_be_dumped;

			bool compile_block(archsim::core::thread::ThreadInstance *cpu, archsim::Address block_address, captive::arch::jit::TranslationContext &ctx, captive::shared::block_txln_fn &fn, wulib::MemAllocator &allocator);

			bool emit_block(archsim::core::thread::ThreadInstance *cpu, archsim::Address block_address, captive::shared::IRBuilder &ctx, std::unordered_set<archsim::Address> &block_heads);
			bool emit_chain(archsim::core::thread::ThreadInstance *cpu, archsim::Address block_address, gensim::BaseDecode *insn, captive::shared::IRBuilder &ctx);

			bool can_merge_jump(archsim::core::thread::ThreadInstance *cpu, gensim::BaseDecode *decode, archsim::Address pc);
			archsim::Address get_jump_target(archsim::core::thread::ThreadInstance *cpu, gensim::BaseDecode *decode, archsim::Address pc);
		};
	}
}

#endif /* INC_BLOCKJIT_BLOCKJITTRANSLATE_H_ */
