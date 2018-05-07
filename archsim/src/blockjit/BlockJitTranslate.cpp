/*
 * BlockJitTranslate.cpp
 *
 *  Created on: 24 Aug 2015
 *      Author: harry
 */

#include "gensim/gensim_processor_blockjit.h"
#include "gensim/gensim_decode.h"
#include "gensim/gensim_translate.h"

#include "core/MemoryInterface.h"

#include "blockjit/BlockJitTranslate.h"
#include "blockjit/BlockProfile.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"

#include "translate/jit_funs.h"
#include "translate/profile/Region.h"

#include "util/LogContext.h"
#include "abi/devices/MMU.h"
#include "util/wutils/tick-timer.h"
#include "blockjit/PerfMap.h"

#include <stdio.h>

UseLogContext(LogTranslate);
DeclareChildLogContext(LogBlockJit, LogTranslate, "BlockJit");

using namespace gensim;
using namespace gensim::blockjit;

using namespace captive::arch::jit;
using namespace captive::shared;

using archsim::Address;

BaseBlockJITTranslate::BaseBlockJITTranslate() : _supportChaining(!archsim::options::JitDisableBranchOpt), _supportProfiling(false), _txln_mgr(NULL), _jumpinfo(NULL), _decode(NULL), _should_be_dumped(false), decode_txlt_ctx(nullptr)
{

}

BaseBlockJITTranslate::~BaseBlockJITTranslate()
{
	if(_decode) delete _decode;
	if(_jumpinfo) delete _jumpinfo;
}

bool BaseBlockJITTranslate::translate_block(archsim::core::thread::ThreadInstance *processor, archsim::Address block_address, archsim::blockjit::BlockTranslation &out_txln, wulib::MemAllocator &allocator)
{
	// Initialise the translation context
	_should_be_dumped = false;
	
	_decode_ctx = processor->GetEmulationModel().GetNewDecodeContext(*processor);
	
	TranslationContext ctx;
	captive::shared::IRBuilder builder;
	
	builder.SetContext(&ctx);
	builder.SetBlock(ctx.alloc_block());
	
	InitialiseFeatures(processor);
	InitialiseIsaMode(processor);

	tick_timer timer (0, stdout);
	timer.reset();

	// Build the IR for this block
	if(!build_block(processor, block_address, builder)) {
		LC_ERROR(LogBlockJit) << "Failed to build block";
		delete _decode_ctx;
		return false;
	}
	timer.tick("build");

	// Optimise the IR and lower it to instructions
	block_txln_fn fn;
	if(!compile_block(processor, block_address, ctx, fn, allocator)) {
		LC_ERROR(LogBlockJit) << "Failed to compile block";
		delete _decode_ctx;
		return false;
	}

	ctx.free_ir_buffer();

	delete _decode_ctx;

	// Fill in the output translation data structure with the translated
	// function and feature vector
	out_txln.Invalidate();
	out_txln.SetFn(fn);
	for(auto i : _read_feature_levels) {
		out_txln.AddRequiredFeature(i, _initial_feature_levels.at(i));
	}

	timer.tick("compile");

	timer.dump("blockjit ");
	return true;
}

void BaseBlockJITTranslate::setSupportChaining(bool enable)
{
	_supportChaining = enable;
}

void BaseBlockJITTranslate::setSupportProfiling(bool enable)
{
	_supportProfiling = enable;
}
void BaseBlockJITTranslate::setTranslationMgr(archsim::translate::TranslationManager *txln_mgr)
{
	_txln_mgr = txln_mgr;
}

void BaseBlockJITTranslate::InitialiseFeatures(const archsim::core::thread::ThreadInstance *cpu)
{
	_feature_levels.clear();
	_initial_feature_levels.clear();
	_read_feature_levels.clear();

	for(const auto feature : cpu->GetFeatures()) {
		_feature_levels[feature.first] = feature.second;
	}

	_initial_feature_levels = _feature_levels;

	_features_valid = true;
}

void BaseBlockJITTranslate::SetFeatureLevel(uint32_t feature, uint32_t level, captive::shared::IRBuilder& builder)
{
	_feature_levels[feature] = level;
	builder.set_cpu_feature(IROperand::const32(feature), IROperand::const32(level));
}

uint32_t BaseBlockJITTranslate::GetFeatureLevel(uint32_t feature)
{
	assert(_feature_levels.count(feature));
	_read_feature_levels.insert(feature);
	return _feature_levels.at(feature);
}

void BaseBlockJITTranslate::InvalidateFeatures()
{
	_features_valid = false;
}

archsim::ProcessorFeatureSet BaseBlockJITTranslate::GetProcessorFeatures() const
{
	archsim::ProcessorFeatureSet features;
	for(auto i : _feature_levels) {
		features.AddFeature(i.first);
		features.SetFeatureLevel(i.first, i.second);
	}

	return features;
}

void BaseBlockJITTranslate::InitialiseIsaMode(const archsim::core::thread::ThreadInstance* cpu)
{
	_isa_mode_valid = true;
	_isa_mode = cpu->GetModeID();
}

void BaseBlockJITTranslate::SetIsaMode(const captive::shared::IROperand& val, captive::shared::IRBuilder& builder)
{
	if(val.is_constant()) {
		_isa_mode = val.value;
	} else {
		InvalidateIsaMode();
	}
	builder.set_cpu_mode(val);
}

uint32_t BaseBlockJITTranslate::GetIsaMode()
{
	assert(_isa_mode_valid);
	return _isa_mode;
}

void BaseBlockJITTranslate::InvalidateIsaMode()
{
	_isa_mode_valid = false;
}

bool BaseBlockJITTranslate::build_block(archsim::core::thread::ThreadInstance *processor, archsim::Address block_address, captive::shared::IRBuilder &builder)
{	
	// initialise some state, and then recursively emit blocks until we come
	// to a branch that we can't resolve statically
	if(!_decode)_decode = processor->GetArch().GetISA(processor->GetModeID()).GetNewDecode();
	if(!_jumpinfo)_jumpinfo = processor->GetArch().GetISA(processor->GetModeID()).GetNewJumpInfo();
	std::unordered_set<Address> block_heads;
	return emit_block(processor, block_address, builder, block_heads);
}

bool BaseBlockJITTranslate::emit_instruction(archsim::core::thread::ThreadInstance *processor, Address pc, gensim::BaseDecode *decode, captive::shared::IRBuilder &builder)
{
	if(archsim::options::Verbose)builder.count(IROperand::const64((uint64_t)processor->GetMetrics().InstructionCount.get_ptr()), IROperand::const64(1));


	if(archsim::options::Profile) {
		UNIMPLEMENTED;
//		builder.count(IROperand::const64((uint64_t)processor->metrics.opcode_freq_hist.get_value_ptr_at_index(decode->Instr_Code)), IROperand::const64(1));
	}

	auto fault = _decode_ctx->DecodeSync(pc, GetIsaMode(), *decode);
	assert(!fault);

	LC_DEBUG4(LogBlockJit) << "Translating instruction " << std::hex << pc.Get() << " " << decode->Instr_Code << " " << decode->ir;

	if(decode->Instr_Code == 65535) {
		LC_DEBUG1(LogBlockJit) << "Invalid instruction! 0x" << std::hex << decode->ir;
		std::cout << "Invalid instruction! PC:" << std::hex << pc.Get() << ": IR:" << std::hex << decode->ir << " ISAMODE:" << (uint32_t)decode->isa_mode << "\n";
		return false;
	}

	IRInstruction b = IRInstruction::barrier();
	b.operands[0] = IROperand::const32(decode->GetIR());
	builder.add_instruction(b);

	b = IRInstruction::barrier();
	b.operands[0] = IROperand::const32(pc.Get());
	builder.add_instruction(b);

	if(archsim::options::Verify && !archsim::options::VerifyBlocks) builder.verify(IROperand::pc(pc.Get()));
	if(archsim::options::InstructionTick) {
		builder.call(IROperand::func((void*)cpuInstructionTick), IROperand::const64((uint64_t)(void*)processor));
	}

	if(archsim::options::ProfilePcFreq) {
		UNIMPLEMENTED;
//		builder.count(IROperand::const64((uint64_t)processor->metrics.pc_freq_hist.get_value_ptr_at_index(pc.Get())), IROperand::const64(1));
	}
	if(archsim::options::ProfileIrFreq) {
		UNIMPLEMENTED;
//		builder.count(IROperand::const64((uint64_t)processor->metrics.inst_ir_freq_hist.get_value_ptr_at_index(decode->ir)), IROperand::const64(1));
	}
	
	if(processor->GetTraceSource()) {
		IRRegId pc_reg = builder.alloc_reg(4);
		builder.ldpc(IROperand::vreg(pc_reg, 4));
		builder.bitwise_and(IROperand::const32(0xfffff000), IROperand::vreg(pc_reg, 4));
		builder.bitwise_or(IROperand::const32(pc.GetPageOffset()), IROperand::vreg(pc_reg, 4));
		builder.call(IROperand::func((void*)cpuTraceInstruction), IROperand::vreg(pc_reg, 4), IROperand::const32(decode->GetIR()), IROperand::const8(decode->isa_mode), IROperand::const8(0));
	}

	translate_instruction(decode, builder, processor->GetTraceSource() != nullptr);
	
	if(decode_txlt_ctx == nullptr) {
		if(!GetComponentInstance(processor->GetArch().GetName(), decode_txlt_ctx)) {
			throw std::logic_error("Could not get DTC");
		}
	}
	
	decode_txlt_ctx->Translate(*decode, *_decode_ctx, builder);

	if(processor->GetTraceSource()) {
		builder.call(IROperand::func((void*)cpuTraceInsnEnd));
	}

	return true;
}

bool BaseBlockJITTranslate::can_merge_jump(archsim::core::thread::ThreadInstance *processor, BaseDecode *decode, Address pc)
{
	// only end of block instructions are jumps
	assert(decode->GetEndOfBlock());

	if(!_supportChaining) {
		return false;
	}

	// We can never merge predicated jumps
	if(decode->GetIsPredicated()) return false;

	// can only merge if the isa mode is still valid
	if(!_isa_mode_valid) return false;

	bool indirect = false, direct = false;
	uint32_t target;
	_jumpinfo->GetJumpInfo(decode, pc.Get(), indirect, direct, target);

	// If this isn't a direct jump, then we can't chain
	if(!direct) return false;

	// Return true if this jump lands within the same page as it starts
	if(archsim::Address(target).GetPageIndex() == pc.GetPageIndex()) return true;
	else return false;
}

archsim::Address BaseBlockJITTranslate::get_jump_target(archsim::core::thread::ThreadInstance *processor, BaseDecode *decode, Address pc)
{
	bool indirect = false, direct = false;
	uint32_t target;

	_jumpinfo->GetJumpInfo(decode, pc.Get(), indirect, direct, target);

	assert(direct || indirect);

	return Address(target);
}

bool BaseBlockJITTranslate::emit_block(archsim::core::thread::ThreadInstance *processor, archsim::Address block_address, captive::shared::IRBuilder &builder, std::unordered_set<Address> &block_heads)
{

	Address pc = block_address;
	uint32_t pc_page = block_address.GetPageBase();

	uint32_t phys_page = 0;
	if(archsim::options::ProfilePcFreq || _supportProfiling) {
		UNIMPLEMENTED;
		//		processor->GetFetchMI().PerformTranslation(pc.GetPageBase(), phys_page, MMUACCESSINFO(processor->in_kernel_mode(), false, true));
	}

	if(_supportProfiling) {
		UNIMPLEMENTED;
		
//		_txln_mgr->GetRegion(phys_page).TraceBlock(*processor, pc.Get());
//		auto &rgn = _txln_mgr->GetRegion(phys_page);
//
//		builder.profile(IROperand::const64((uint64_t)&rgn), IROperand::const32(block_address.GetPageOffset()));
	}

	LC_DEBUG3(LogBlockJit) << "Translating block " << std::hex << pc.Get() << " " << _decode->Instr_Code << " " << _decode->ir;

	bool success = true;

	// emit the IR for instructions found in this block
	// keep track of which blocks we've emitted to avoid getting stuck in a lop[]
	block_heads.insert(pc);

	uint32_t count = 0;
	// only emit IR for a fixed number of guest instructions to stop the IR
	// getting too large
	uint32_t max_count = 32;

	// make sure that we stay on the same page, and that the isa mode or feature vector doesn't
	// change unexpectedly
	while(pc.GetPageBase() == pc_page && count < max_count && _isa_mode_valid && _features_valid) {
		count++;

		// emit the IR for the instruction pointed to by the virtual PC
		if(!emit_instruction(processor, pc, _decode, builder)) {
			success = false;
			break;
		}

		// If this instruction is an end of block, potentially merge the next block
		if(_decode->GetEndOfBlock()) {
			if(can_merge_jump(processor, _decode, pc)) {
				Address target = get_jump_target(processor, _decode, pc);
				if(!block_heads.count(target)) {
					block_heads.insert(target);
					if(archsim::options::Verify && archsim::options::VerifyBlocks) builder.verify(IROperand::pc(pc.Get()));
					return emit_block(processor, target, builder, block_heads);
				}
			}

			break;
		} else {
			pc += _decode->Instr_Length;
		}
	}

	if(archsim::options::Verify && archsim::options::VerifyBlocks) builder.verify(IROperand::pc(pc.Get()));

	// attempt to chain (otherwise return)
	emit_chain(processor, pc, _decode, builder);

	return success;
}

bool BaseBlockJITTranslate::emit_chain(archsim::core::thread::ThreadInstance *processor, archsim::Address pc, gensim::BaseDecode *decode, captive::shared::IRBuilder &builder)
{
	// If this instruction is an end of block (rather than the start of the next page)
	// then we might be able to chain

	// First, figure out if we should try to chain. We should only try to chain
	// if we are at the end of a block, if chaining is enabled, and if the
	// context is valid (isa mode and features).)
	if(decode->GetEndOfBlock() && _supportChaining && _isa_mode_valid && _features_valid) {
		uint32_t target_pc = 0, fallthrough_pc = 0;

		bool direct = false, indirect = false;

		// Try to figure out where this branch will end up.
		_jumpinfo->GetJumpInfo(decode, pc.Get(), indirect, direct, target_pc);

		fallthrough_pc = pc.Get() + decode->Instr_Length;

		// Can only chain on a direct jump
		if(direct) {
			uint32_t target_page = archsim::translate::profile::RegionArch::PageBaseOf(target_pc);
			uint32_t fallthrough_page = archsim::translate::profile::RegionArch::PageBaseOf(fallthrough_pc);

			// Can only chain if the target of the jump is on the same page as the rest of the block
			if(target_page == pc.GetPageBase()) {
				// If instruction is not predicated, don't use a fallthrough pc
				// XXX ARM HAX
				if(!decode->GetIsPredicated()) {
					fallthrough_pc = 0;
				}

				//need to translate target and fallthrough PCs

				Address target_ppc(0), fallthrough_ppc(0);
				if(target_pc) processor->GetFetchMI().PerformTranslation(Address(target_pc), target_ppc, false, true, false);
				if(fallthrough_pc) processor->GetFetchMI().PerformTranslation(Address(fallthrough_pc), fallthrough_ppc, false, true, false);

				archsim::blockjit::BlockTranslation target_fn;
				archsim::blockjit::BlockTranslation fallthrough_fn;

				block_txln_fn target_fn_ptr = nullptr, fallthrough_fn_ptr = nullptr;

				if(target_ppc.Get() != 0) {
//					processor->GetTranslatedBlock(target_ppc, GetProcessorFeatures(), target_fn);
				}
				if(fallthrough_ppc.Get() != 0) {
//					processor->GetTranslatedBlock(fallthrough_ppc, GetProcessorFeatures(), fallthrough_fn);
				}

				// actually get functions
				if(fallthrough_fn.IsValid(GetProcessorFeatures())) {
					fallthrough_fn_ptr = fallthrough_fn.GetFn();
				}

				if(target_fn.IsValid(GetProcessorFeatures())) {
					target_fn_ptr = target_fn.GetFn();
				}

				if(fallthrough_fn_ptr != nullptr || target_fn_ptr != nullptr) {
					builder.dispatch(IROperand::const32(target_pc), IROperand::const32(fallthrough_pc), IROperand::const64((uint64_t)target_fn_ptr), IROperand::const64((uint64_t)fallthrough_fn_ptr));
					return true;
				}
			} else {
//				fprintf(stderr, "*** Couldn't chain due to cross page jump\n");
			}
		} else {
//			fprintf(stderr, "*** Couldn't chain due to indirect jump\n");
		}
	}
	// If we couldn't find any way to chain, then just return
	builder.ret();
	return true;
}

bool BaseBlockJITTranslate::compile_block(archsim::core::thread::ThreadInstance *cpu, archsim::Address block_address, captive::arch::jit::TranslationContext &ctx, captive::shared::block_txln_fn &fn, wulib::MemAllocator &allocator)
{
	BlockCompiler compiler (ctx, block_address.Get(), allocator, false, true);
	compiler.set_cpu(cpu);

	bool dump = (archsim::options::Debug) || _should_be_dumped;

	size_t size = compiler.compile(fn, dump);

	if(dump) {
		std::ostringstream str;
		str << "blkjit-" << std::hex << block_address.Get() << ".bin";
		FILE *outfile = fopen(str.str().c_str(), "w");
		fwrite((void*)fn, size, 1, outfile);
		fclose(outfile);

		str.str("");
		str << "blkjit-" << std::hex << block_address.Get() << ".txt";
		outfile = fopen(str.str().c_str(), "w");
		str.str("");
		compiler.dump_ir(str);
		std::string ir_str = str.str();
		fwrite(ir_str.c_str(), ir_str.length(), 1, outfile);
		fclose(outfile);
	}


	PerfMap &pmap = PerfMap::Singleton;
	if(pmap.Enabled()) {
		pmap.Acquire();
		pmap.Stream() << std::hex << (size_t)fn << " " << size << " JIT_" << std::hex << block_address.Get() << std::endl;
		pmap.Release();
	}

	return size != 0;
}
