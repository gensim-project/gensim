/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * BlockJitTranslate.cpp
 *
 *  Created on: 24 Aug 2015
 *      Author: harry
 */

#include "gensim/gensim_decode.h"
#include "gensim/gensim_translate.h"

#include "core/MemoryInterface.h"

#include "blockjit/BlockJitTranslate.h"
#include "blockjit/BlockProfile.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/block-compiler/lowering/NativeLowering.h"
#include "blockjit/translation-context.h"

#include "translate/jit_funs.h"
#include "translate/profile/Region.h"

#include "util/LogContext.h"
#include "abi/devices/MMU.h"
#include <wutils/tick-timer.h>
#include "blockjit/PerfMap.h"
#include "blockjit/IRPrinter.h"

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

	SetDecodeContext(processor->GetEmulationModel().GetNewDecodeContext(*processor));

	TranslationContext ctx;
	captive::shared::IRBuilder builder;

	builder.SetContext(&ctx);
	builder.SetBlock(ctx.alloc_block());

	InitialiseFeatures(processor);
	InitialiseIsaMode(processor);

	wutils::tick_timer timer (0, stdout);
	timer.reset();

	// Build the IR for this block
	if(!build_block(processor, block_address, builder)) {
		LC_ERROR(LogBlockJit) << "Failed to build block";
		delete _decode_ctx;
		return false;
	}
	timer.tick("build");

	// Optimise the IR and lower it to instructions
	out_txln.Invalidate();
	if(!compile_block(processor, block_address, ctx, out_txln, allocator)) {
		LC_ERROR(LogBlockJit) << "Failed to compile block";
		delete _decode_ctx;
		return false;
	}

//	ctx.trim();
//	fprintf(stderr, "*** %08x = %u %u\n", block_address.Get(), (uint32_t)ctx.size_bytes(), out_txln.GetSize());
	ctx.free_ir_buffer();

	delete _decode_ctx;

	// Fill in the output translation data structure with the translated
	// function and feature vector
	AttachFeaturesTo(out_txln);

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

void BaseBlockJITTranslate::AttachFeaturesTo(archsim::blockjit::BlockTranslation& txln) const
{
	for(auto i : _read_feature_levels) {
		txln.AddRequiredFeature(i, _initial_feature_levels.at(i));
	}
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

bool BaseBlockJITTranslate::emit_instruction(archsim::core::thread::ThreadInstance* cpu, archsim::Address pc, gensim::BaseDecode*& insn, captive::shared::IRBuilder& builder)
{
	_decode_ctx->Reset(cpu);
	auto fault = _decode_ctx->DecodeSync(cpu->GetFetchMI(), pc, GetIsaMode(), insn);
	_decode_ctx->WriteBackState(cpu);
	assert(!fault);

	if(insn->Instr_Code == 65535) {
		LC_DEBUG1(LogBlockJit) << "Invalid instruction! 0x" << std::hex << insn->ir;
		std::cout << "Invalid instruction! PC:" << std::hex << pc.Get() << ": IR:" << std::hex << insn->ir << " ISAMODE:" << (uint32_t)insn->isa_mode << "\n";
		return false;
	}

	return emit_instruction_decoded(cpu, pc, insn, builder);
}


bool BaseBlockJITTranslate::emit_instruction_decoded(archsim::core::thread::ThreadInstance *processor, Address pc, const gensim::BaseDecode *decode, captive::shared::IRBuilder &builder)
{
	LC_DEBUG4(LogBlockJit) << "Translating instruction " << std::hex << pc.Get() << " " << decode->Instr_Code << " " << decode->ir;

	if(archsim::options::Verbose) {
		builder.count(IROperand::const64((uint64_t)processor->GetMetrics().InstructionCount.get_ptr()), IROperand::const64(1));
		builder.count(IROperand::const64((uint64_t)processor->GetMetrics().JITInstructionCount.get_ptr()), IROperand::const64(1));
	}

	if(archsim::options::InstructionTick) {
		builder.call(IROperand::func((void*)cpuInstructionTick), IROperand::const64((uint64_t)(void*)processor));
	}

	if(archsim::options::Profile) {
		builder.count(IROperand::const64((uint64_t)processor->GetMetrics().OpcodeHistogram.get_value_ptr_at_index(decode->Instr_Code)), IROperand::const64(1));
	}
	if(archsim::options::ProfilePcFreq) {
		builder.count(IROperand::const64((uint64_t)processor->GetMetrics().PCHistogram.get_value_ptr_at_index(pc.Get())), IROperand::const64(1));
	}
	if(archsim::options::ProfileIrFreq) {
		builder.count(IROperand::const64((uint64_t)processor->GetMetrics().InstructionIRHistogram.get_value_ptr_at_index(decode->ir)), IROperand::const64(1));
	}

	if(processor->GetTraceSource()) {
		IRRegId pc_reg = builder.alloc_reg(8);
		builder.ldpc(IROperand::vreg(pc_reg, 8));
		builder.bitwise_and(IROperand::const64(0xfffffffffffff000ull), IROperand::vreg(pc_reg, 8));
		builder.bitwise_or(IROperand::const64(pc.GetPageOffset()), IROperand::vreg(pc_reg, 8));
		builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceInstruction), IROperand::vreg(pc_reg, 8), IROperand::const32(decode->GetIR()), IROperand::const8(decode->isa_mode), IROperand::const8(0));
	}

	translate_instruction(decode, builder, processor->GetTraceSource() != nullptr);

	if(decode_txlt_ctx == nullptr) {
		if(!GetComponentInstance(processor->GetArch().GetName(), decode_txlt_ctx)) {
			throw std::logic_error("Could not get DTC");
		}
	}

	decode_txlt_ctx->Translate(processor, *decode, *_decode_ctx, builder);

	if(processor->GetTraceSource()) {
		builder.call(IROperand::const32(0), IROperand::func((void*)cpuTraceInsnEnd));
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

	JumpInfo info;
	_jumpinfo->GetJumpInfo(decode, pc, info);

	// If this isn't a direct jump, then we can't chain
	if(info.IsIndirect) return false;
	if(info.IsConditional) return false;

	// Return true if this jump lands within the same page as it starts
	if(info.JumpTarget.GetPageIndex() == pc.GetPageIndex()) return true;
	else return false;
}

archsim::Address BaseBlockJITTranslate::get_jump_target(archsim::core::thread::ThreadInstance *processor, BaseDecode *decode, Address pc)
{
	JumpInfo info;
	_jumpinfo->GetJumpInfo(decode, pc, info);

	return Address(info.JumpTarget);
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
//					if(archsim::options::Verify && archsim::options::VerifyBlocks) builder.verify(IROperand::pc(pc.Get()));
					return emit_block(processor, target, builder, block_heads);
				}
			}

			break;
		} else {
			pc += _decode->Instr_Length;
		}
	}

//	if(archsim::options::Verify && archsim::options::VerifyBlocks) builder.verify(IROperand::pc(pc.Get()));

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
		Address target_pc = 0_ga, fallthrough_pc = 0_ga;

		JumpInfo jump_info;
		// TODO: change target_pc_naked to actually be an Address instead of a uint32
		_jumpinfo->GetJumpInfo(decode, pc, jump_info);
		fallthrough_pc = pc + decode->Instr_Length;

		// Can only chain on a direct jump
		if(!jump_info.IsIndirect) {
			auto target_page = jump_info.JumpTarget.GetPageBase();
			auto fallthrough_page = fallthrough_pc.GetPageBase();

			// Can only chain if the target of the jump is on the same page as the rest of the block
			if(target_page == pc.GetPageBase()) {
				// If instruction is not predicated, don't use a fallthrough pc
				// XXX ARM HAX
				if(!decode->GetIsPredicated()) {
					fallthrough_pc = 0_ga;
				}

				//need to translate target and fallthrough PCs

				Address target_ppc(0), fallthrough_ppc(0);
				if(target_pc != Address::NullPtr) processor->GetFetchMI().PerformTranslation(target_pc, target_ppc, false, true, false);
				if(fallthrough_pc != Address::NullPtr) processor->GetFetchMI().PerformTranslation(fallthrough_pc, fallthrough_ppc, false, true, false);

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
					builder.dispatch(IROperand::const32(target_pc.Get()), IROperand::const32(fallthrough_pc.Get()), IROperand::const64((uint64_t)target_fn_ptr), IROperand::const64((uint64_t)fallthrough_fn_ptr));
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

bool BaseBlockJITTranslate::compile_block(archsim::core::thread::ThreadInstance *cpu, archsim::Address block_address, captive::arch::jit::TranslationContext &ctx, archsim::blockjit::BlockTranslation &fn, wulib::MemAllocator &allocator)
{
	BlockCompiler compiler (ctx, block_address.Get(), allocator, false, true);

	bool dump = (archsim::options::Debug) || _should_be_dumped;

	auto result = compiler.compile(dump);

	if(!result.Success) {
		return false;
	}

	auto lowering = captive::arch::jit::lowering::NativeLowering(ctx, allocator, cpu->GetArch(), cpu->GetStateBlock().GetDescriptor(), result);
	fn.SetFn(lowering.Function);

	if(dump) {
		ctx.trim();

		std::ostringstream ir_filename;
		ir_filename << "blkjit-" << std::hex << block_address.Get() << ".txt";
		std::ofstream ir_file (ir_filename.str().c_str());
		archsim::blockjit::IRPrinter printer;
		printer.DumpIR(ir_file, ctx);

		std::ostringstream str;
		str << "blkjit-" << std::hex << block_address.Get() << ".bin";
		FILE *outfile = fopen(str.str().c_str(), "w");
		fwrite((void*)lowering.Function, lowering.Size, 1, outfile);
		fclose(outfile);
	}


	PerfMap &pmap = PerfMap::Singleton;
	if(pmap.Enabled()) {
		pmap.Acquire();
		pmap.Stream() << std::hex << (size_t)lowering.Function << " " << lowering.Size << " JIT_" << std::hex << block_address.Get() << std::endl;
		pmap.Release();
	}

	fn.SetSize(lowering.Size);
	return lowering.Size != 0;
}
