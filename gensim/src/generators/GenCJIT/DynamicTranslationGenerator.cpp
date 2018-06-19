/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <vector>

#include "clean_defines.h"
#include "define.h"

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"

#include "genC/Parser.h"
#include "genC/ir/IRAction.h"
#include "genC/ir/IRType.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/printers/SSAContextPrinter.h"
#include "genC/ssa/printers/SSAActionCFGPrinter.h"

#include "generators/GenCJIT/DynamicTranslationGenerator.h"
#include "generators/MakefileGenerator.h"
#include "generators/InterpretiveExecutionEngineGenerator.h"
#include "generators/ClangLLVMTranslationGenerator.h"

DEFINE_COMPONENT(gensim::generator::DynamicTranslationGenerator, translate_dynamic)
COMPONENT_INHERITS(translate_dynamic, translate)
COMPONENT_OPTION(translate_dynamic, Clang, "clang", "Path to the clang binary to use when compiling the precompiled LLVM module")
COMPONENT_OPTION(translate_dynamic, ArcsimPath, "archsim", "Path to the archsim build directory")
COMPONENT_OPTION(translate_dynamic, Optimize, "0", "Optimisation level to use for precompiled portion of JIT")
COMPONENT_OPTION(translate_dynamic, Debug, "0", "If set to 1, emit a text representation of the SSA used for the JIT.")
COMPONENT_OPTION(translate_dynamic, EmitGraphs, "0", "If set to 1, emit dot format graphs for the final form of each execute action.")
COMPONENT_OPTION(translate_dynamic, SmartRegAlloc, "1", "If set to 0, emit a separate llvm stack entry for each variable rather than reusing existing entries.")
COMPONENT_OPTION(translate_dynamic, TranslationChunks, "5", "Number of source files to split translation functions in to, in order to allow parallel builds.")

#define NOLIMM

namespace gensim
{
	namespace generator
	{
		using namespace genc::ssa;

		bool DynamicTranslationGenerator::EmitFixedFunction(util::cppformatstream &cstream, SSAFormAction &action, std::string name_prefix /* = "Execute_" */) const
		{
			// emit a translation method for the action:
			// emit the method header
			cstream << action.GetPrototype().GetIRSignature().GetType().GetCType() << " Translate::" << name_prefix << action.GetPrototype().GetIRSignature().GetName() << "(const Decode * const __instr, uint32_t __PC, llvm::Function *__function, llvm::Module *__module, llvm::IRBuilder<> &__irBuilder, bool __trace";
			for (const auto param : action.GetPrototype().GetIRSignature().GetParams()) {
				cstream << "," << param.GetType().GetCType() << " CV_top_" << param.GetName();
			}
			cstream << "){";

			// we don't want to manually keep track of fixed variables, so have our C++ compiler do it
			// for us. emit declarations for ALL symbols here.
			for (SSASymbol *sym : action.Symbols()) {
				// skip any parameters
				if (sym->SType == genc::Symbol_Parameter || sym->GetType().Reference) continue;
				cstream << sym->GetType().GetCType() << " CV_" << sym->GetName() << ";";
			}

			// loop through each block and if it is fixed or sometimes fixed, emit code for it
			for (std::list<SSABlock *>::const_iterator block_iterator = action.Blocks.begin(); block_iterator != action.Blocks.end(); ++block_iterator) {
				SSABlock &block = **block_iterator;
				GenerateFixedBlockEmitter(cstream, block);
			}

			cstream << "done:;";

			cstream << "}";
			return true;
		}

		bool DynamicTranslationGenerator::GeneratePredicateEmitters(util::cppformatstream& cfile, util::cppformatstream& hfile) const
		{
			const arch::ArchDescription &arch = Manager.GetArch();

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				const auto act = dynamic_cast<const SSAFormAction *> (isa->GetSSAContext().GetAction("instruction_predicate"));
				assert(act);

				std::stringstream s;
				s << "Helper_" << isa->ISAName << "_";

				EmitDynamicEmitter(cfile, hfile, *act, s.str());
			}

			hfile << "bool EmitPredicate(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, llvm::Value*& __result, bool trace);";

			cfile << "bool gensim::" << arch.Name << "::Translate::EmitPredicate(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, llvm::Value*& __result, bool trace)";
			cfile << "{";
			cfile << "switch (ctx.tiu.GetDecode().isa_mode) {";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				cfile << "case ISA_MODE_" << isa->ISAName << ": ";
				cfile << "Helper_" << isa->ISAName << "_instruction_predicate(ctx, __result, trace); return true;";
			}

			cfile << "default: return false;";

			cfile << "}";
			cfile << "}";

			return true;
		}

		bool DynamicTranslationGenerator::EmitDynamicEmitter(util::cppformatstream &cstream, util::cppformatstream &hstream, const SSAFormAction &action, std::string name_prefix /* = "Execute_" */) const
		{
			hstream << "void " << name_prefix << action.GetPrototype().GetIRSignature().GetName() << "(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, llvm::Value*& __result, bool __trace);";

			// emit a translation method for the action:
			// emit the method header
			cstream << "void gensim::" << Manager.GetArch().Name << "::Translate::" << name_prefix << action.GetPrototype().GetIRSignature().GetName() << "(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, llvm::Value*& __result, bool __trace) {";

			// keep a map of non-fixed block numbers to llvm basic blocks. We only want to add a block if
			// we have a chance of executing it so don't add anything here yet.
			cstream << "__result = NULL;"
			        "     std::map<uint16, llvm::BasicBlock*> dynamic_blocks;"
			        "     std::list<uint16> dynamic_block_queue;";

			// first, number the blocks. we'll use the numbers later on when emitting code dynamically
			std::map<SSABlock *, uint16_t> block_numbers;
			uint16_t blocks = 0;
			for (std::list<SSABlock *>::const_iterator block_i = action.Blocks.begin(); block_i != action.Blocks.end(); ++block_i) {
				block_numbers[*block_i] = blocks;
				cstream << "    const int __block_" << (*block_i)->GetName() << " = " << blocks << ";";
				blocks++;
			}

			// emit our execute function argument - the instruction we are JITing
			cstream << "const Decode* const CV_top_inst = (Decode*)(&ctx.tiu.GetDecode());";
			cstream << "llvm::IRBuilder<>& __irBuilder = ctx.block.region.builder;";

			// we don't want to manually keep track of fixed variables, so have our C++ compiler do it
			// for us. emit declarations for ALL symbols here.
			int i = 0;
			for (SSASymbol *sym : action.Symbols()) {
				// skip the inst field since we already have that
				// also skip references
				if (sym->GetName() == "top_inst" || sym->GetType().Reference) {
					continue;
				}

				cstream << "const uint32_t __idx_" << sym->GetName() << " = " << i++ << ";\n";
				cstream << sym->GetType().GetCType() << " CV_" << sym->GetName() << ";";
			}

			cstream << "llvm_registers.resize(0, NULL);";
			cstream << "llvm_registers.resize(" << i << ", NULL);";

			// loop through each block and if it is fixed or sometimes fixed, emit code for it
			for (std::list<SSABlock *>::const_iterator block_iterator = action.Blocks.begin(); block_iterator != action.Blocks.end(); ++block_iterator) {
				SSABlock &block = **block_iterator;
				if (block.IsFixed() == BLOCK_ALWAYS_CONST) GenerateDynamicBlockEmitter(cstream, block);
			}

			// first, count the dynamic blocks to see if we should emit a dynamic block emitter loop
			uint32_t dynamic_block_count = 0;
			for (std::list<SSABlock *>::const_iterator block_i = action.Blocks.begin(); block_i != action.Blocks.end(); ++block_i) {
				if ((*block_i)->IsFixed() != BLOCK_ALWAYS_CONST) dynamic_block_count++;
			}
			if (dynamic_block_count) {
				cstream << ""
				        "fixed_done:;"
				        "if(dynamic_block_queue.size()) "
				        "{"
				        // if we're emitting dynamic blocks, we assume that we're going to jump out of the block we
				        // started in so we should emit an 'end of instruction' 'block
				        "     llvm::BasicBlock *__end_of_instruction = llvm::BasicBlock::Create(txln_ctx.llvm_ctx, \"\", ctx.block.region.region_fn);"
				        "     std::set<uint16> emitted_blocks;"
				        "     while(dynamic_block_queue.size()) "
				        "     {"
				        "             uint16_t block_index = dynamic_block_queue.front();"
				        "             dynamic_block_queue.pop_front();"
				        "             if(emitted_blocks.find(block_index) != emitted_blocks.end()) continue;"
				        "             emitted_blocks.insert(block_index);"
				        "             switch(block_index)"
				        "             {";

				// loop over each sometimes_fixed and never_fixed block and emit an emitter for that block
				for (std::list<SSABlock *>::const_iterator block_i = action.Blocks.begin(); block_i != action.Blocks.end(); ++block_i) {
					SSABlock *block = *block_i;
					if (block->IsFixed() == BLOCK_ALWAYS_CONST) continue;
					cstream << ""
					        "case __block_" << block->GetName() << ": // BLOCK START LINE " << block->GetStartLine() << ", END LINE " << block->GetEndLine() << "\n"
					        "{"
					        "   ctx.block.region.builder.SetInsertPoint(dynamic_blocks[__block_" << block->GetName() << "]);";

					DynamicTranslationNodeWalkerFactory factory;

					for (auto s_i = block->GetStatements().begin(); s_i != block->GetStatements().end(); ++s_i) {
						SSAStatement *stmt = *s_i;
						cstream << "// STMT: LINE " << stmt->GetDiag().Line() << "\n";
						cstream << "// " << stmt->ToString() << "\n";
						if (stmt->IsFixed())
							factory.GetOrCreate(stmt)->EmitFixedCode(cstream, "dynamic_done", false);
						else
							factory.GetOrCreate(stmt)->EmitDynamicCode(cstream, "dynamic_done", false);
					}

					cstream << ""
					        "    break;"
					        "}";
				}
				cstream << ""
				        "}"
				        "dynamic_done:;"
				        "ctx.block.region.builder.SetInsertPoint(__end_of_instruction);"
				        "}"
				        "}";
			}

			// Need a semicolon here in case we don't have any variables to free
			cstream << "free_variables:;";
			// Now safely free any allocated registers
			for (SSASymbol *sym : action.Symbols()) {
				// skip the inst field since we already have that
				// also skip references
				if (sym->GetName() == "top_inst" || sym->GetType().Reference) {
					continue;
				}

				if (sym->GetType().VectorWidth != 1) {
					cstream << "SafeFreeVector(__idx_" << sym->GetName() << ");";
					continue;
				}

				switch (sym->GetType().BaseType.PlainOldDataType) {
					case gensim::genc::IRPlainOldDataType::INT8:
						cstream << "SafeFreeVR8(__idx_" << sym->GetName() << ");";
						break;
					case gensim::genc::IRPlainOldDataType::INT16:
						cstream << "SafeFreeVR16(__idx_" << sym->GetName() << ");";
						break;
					case gensim::genc::IRPlainOldDataType::INT32:
						cstream << "SafeFreeVR32(__idx_" << sym->GetName() << ");";
						break;
					case gensim::genc::IRPlainOldDataType::INT64:
						cstream << "SafeFreeVR64(__idx_" << sym->GetName() << ");";
						break;
						
					default:
						throw std::logic_error("Unhandled");
				}
			}

			cstream << "}";
			return true;
		}

		const std::vector<std::string> DynamicTranslationGenerator::GetSources() const
		{
			return sources;
		}

		bool DynamicTranslationGenerator::GenerateNonInlineFunctions(util::cppformatstream &cfile)const
		{
			for (auto isa : Manager.GetArch().ISAs) {
				for (const auto& helper_item : isa->GetSSAContext().Actions()) {
					if (!helper_item.second->HasAttribute(genc::ActionAttribute::Helper)) continue;

					auto helper = dynamic_cast<const SSAFormAction *> (helper_item.second);

					const genc::IRHelperAction *irhelper = dynamic_cast<const genc::IRHelperAction*> (helper->GetAction());
					if (irhelper && irhelper->GetSignature().HasAttribute(genc::ActionAttribute::NoInline)) {
						cfile << "extern \"C\" " << irhelper->GetSignature().GetType().GetCType() << " txln_shunt_" << irhelper->GetSignature().GetName() << "(uint8_t* cpu_ctx";

						int i = 0;
						std::stringstream param_str;
						for (auto param : irhelper->GetSignature().GetParams()) {
							cfile << ", " << param.GetType().GetCType() << " p" << i;
							if (i) param_str << ", ";
							param_str << "p" << i;

							i++;
						}

						cfile << "){ ((Processor*)cpu_ctx)->" << irhelper->GetSignature().GetName() << "(" << param_str.str() << "); }";
					}
				}
			}
			return true;
		}

		bool DynamicTranslationGenerator::GenerateNonInlineFunctionPrototypes(util::cppformatstream &cfile)const
		{
			for (auto isa : Manager.GetArch().ISAs) {
				for (const auto& helper_item : isa->GetSSAContext().Actions()) {
					if (!helper_item.second->HasAttribute(genc::ActionAttribute::Helper)) continue;

					auto helper = dynamic_cast<const SSAFormAction *> (helper_item.second);

					const genc::IRHelperAction *irhelper = dynamic_cast<const genc::IRHelperAction*> (helper->GetAction());
					if (irhelper && irhelper->GetSignature().HasAttribute(genc::ActionAttribute::NoInline)) {
						cfile << "extern \"C\" " << irhelper->GetSignature().GetType().GetCType() << " txln_shunt_" << irhelper->GetSignature().GetName() << "(uint8_t* cpu_ctx";

						int i = 0;
						std::stringstream param_str;
						for (auto param : irhelper->GetSignature().GetParams()) {
							cfile << ", " << param.GetType().GetCType() << " p" << i;
							if (i) param_str << ", ";
							param_str << "p" << i;

							i++;
						}

						cfile << ");";
					}
				}
			}
			return true;
		}

		bool DynamicTranslationGenerator::Generate() const
		{
			std::ostringstream err_str;

			//      genc::GenCContext *context = new genc::GenCContext(Manager.GetArch());

			// const genc::ssa::SSAContext *ssa_ctx = Manager.GetArch().GetSSACtx();

			if (GetProperty("EmitGraphs") == "1") GenerateGraphs();
			if (GetProperty("Debug") == "1") {
				std::ostringstream debug_str;

				genc::ssa::printers::SSAContextPrinter context_printer(Manager.GetArch().ISAs.front()->GetSSAContext());
				context_printer.Print(debug_str);

				puts(debug_str.str().c_str());
			}
			// open the output files
			std::ostringstream hfile;
			hfile << Manager.GetTarget() << "/translate.h";
			std::ostringstream cfile;
			cfile << Manager.GetTarget() << "/translate.cpp";

			sources.push_back("translate.cpp");

			util::cppformatstream hstream;
			util::cppformatstream cstream;

			// emit the file headers
			hstream << ""
			        "#ifndef __HEADER_TRANSLATE_" << Manager.GetArch().Name << "\n"
			        "#define __HEADER_TRANSLATE_" << Manager.GetArch().Name << "\n"
			        ""

			        // Includes
			        //
			        "#include <abi/memory/MemoryTranslationModel.h>\n"
			        "#include <gensim/gensim_translate.h>\n"
			        "#include <translate/TranslationWorkUnit.h>\n"
			        "#include <translate/llvm/LLVMTranslationContext.h>\n"
			        "#include <translate/llvm/LLVMAliasAnalysis.h>\n"
			        "#include <llvm/IR/Intrinsics.h>\n"
			        "#include <llvm/IR/IRBuilder.h>\n"
			        "#include <llvm/IR/Module.h>\n"
			        "#include <string>\n"
			        "#include <list>\n"
			        "#include <map>\n"

			        // Definition related to differing versions of llvm using different APIs
			        // TODO: Remove or replace with typedef?
			        "#define LLVM_TYPE_PTR llvm::Type*\n"
			        "namespace gensim { "

			        // Forward decls
			        //
			        "class BaseDecode;\n"
			        "class Processor;\n "
			        "namespace " << Manager.GetArch().Name << "{ \n"

			        "class Translate : public gensim::BaseLLVMTranslate {\n"
			        "public: \n"

			        // Constructors
			        //
			        "Translate(const gensim::Processor& cpu, archsim::translate::llvm::LLVMTranslationContext& ctx);"

			        // Deprecated C translation functions
			        // TODO: remove?
			        //
			        "bool TranslateInstruction(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, llvm::Value*& __result, bool trace);\n"
			        "std::string GetStateMacros();\n"

			        // Instruction information
			        // TODO: Move to decode?
			        //
			        "void GetJumpInfo(const gensim::BaseDecode *instr, uint32, bool &indirect_jump, bool &direct_jump, uint32_t &jump_target);\n"
			        "bool GetUsesPc(const gensim::BaseDecode *instr);\n"

			        // Precompiled bitcode access
			        //
			        "uint8_t *GetPrecompBitcode();\n"
			        "uint32_t GetPrecompSize();\n"

			        // Code emission functions
			        //
			        "private:"
			        "std::vector<llvm::Value*> llvm_registers;"
			        "std::vector<llvm::Value*> llvm_free_registers_8;"
			        "std::vector<llvm::Value*> llvm_free_registers_16;"
			        "std::vector<llvm::Value*> llvm_free_registers_32;"
			        "std::vector<llvm::Value*> llvm_free_registers_64;"
			        "std::vector<llvm::Value*> llvm_free_registers_double;"
			        "std::vector<llvm::Value*> llvm_free_registers_float;"

			        "inline void QueueDynamicBlock(archsim::translate::llvm::LLVMRegionTranslationContext& ctx, std::map<uint16, llvm::BasicBlock*> &block_map, std::list<uint16> &block_queue, uint32_t block_id)"
			        "{"
			        "  if(block_map.find(block_id) == block_map.end())"
			        "  {"
			        "    llvm::Value *block = block_map[block_id] = llvm::BasicBlock::Create(ctx.txln.llvm_ctx, \"\", ctx.region_fn);"
			        "    block_queue.push_back(block_id);"
			        "  }"
			        "}"

			        // Register access functions
			        // TODO: Should we also have memory access functions?
			        //

			        "inline llvm::Value* GetBankedRegisterPointer(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, uint32_t bank, llvm::Value *regnum) {"
			        "  if(llvm::ConstantInt *constant = llvm::dyn_cast<llvm::ConstantInt>(regnum)) {"
			        "    return ctx.block.region.RegSlots[std::make_pair(bank, constant->getZExtValue())];"
			        "  } else {"
			        "  llvm::Value *ptr = ctx.block.region.values.reg_state_val;"
			        "  llvm::Type *reg_type = NULL;"
			        "  uint32_t bank_offset = 0, stride = 0, bank_elements = 0, element_size = 0;"

			        "  switch(bank) {";
			int i = 0;
			for (const auto &bank : Manager.GetArch().GetRegFile().GetBanks()) {
				hstream << "case " << i++ << ": bank_offset = " << bank->GetRegFileOffset() << "; reg_type = " << bank->GetRegisterIRType().GetLLVMType() << "; stride = " << bank->GetRegisterStride() << "; bank_elements = " << bank->GetRegisterCount() << "; element_size = " << bank->GetRegisterWidth() << "; break;";
			}

			hstream <<
			        "  default: assert(false);"
			        "  }"
			        "  llvm::IRBuilder<> &builder = ctx.block.region.builder;"
			        "  llvm::Value *stride_val = llvm::ConstantInt::get(txln_ctx.types.i32, element_size);"
			        "  llvm::Value *regnum_val = builder.CreateZExt(regnum,txln_ctx.types.i32);"

			        "  llvm::Value *offset_in_bank = builder.CreateMul(regnum_val, stride_val);"
			        "  llvm::Value *reg_offset = builder.CreateAdd(llvm::ConstantInt::get(txln_ctx.types.i32, bank_offset), offset_in_bank);"

			        "  std::vector<llvm::Value*> gep_indices = {llvm::ConstantInt::get(txln_ctx.types.i32, 0), llvm::ConstantInt::get(txln_ctx.types.i32, 0), reg_offset};"

			        "  ptr = builder.CreateGEP(ptr, gep_indices);"
			        "  ptr = builder.CreatePointerCast(ptr, llvm::PointerType::get(reg_type, 0));"

			        "  txln_ctx.AddAliasAnalysisToRegBankAccess(ptr, regnum, bank_offset, element_size, bank_elements);"
			        "  return ptr;"
			        "  }"
			        "}"

			        // Slots can only be directly indexed, so we can return the reg slot pointer immediately
			        "inline ::llvm::Value *GetRegisterPointer(archsim::translate::llvm::LLVMInstructionTranslationContext &ctx, uint32_t slot) {"
			        "  return ctx.block.region.RegSlots[std::make_pair(-1, slot)];"
			        "}"

			        "inline void EmitBankedRegisterWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, uint32_t bank, llvm::Value *regnum, llvm::Value *value, bool __trace)"
			        "{"
			        "  llvm::IRBuilder<> &builder = ctx.block.region.builder;"
			        "  llvm::Value *ptr = GetBankedRegisterPointer(ctx, bank, regnum);"
			        "  ctx.block.region.builder.CreateStore(value, ptr);"
			        //	        "  if(__trace){"
			        //	        "    llvm::Value *val = ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, value, txln_ctx.types.i32);"
			        //	        "    regnum = ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, regnum, txln_ctx.types.i32);"
			        //	        "    ctx.block.region.builder.CreateCall4(txln_ctx.jit_functions.trace_reg_bank_read, ctx.block.region.values.cpu_ctx_val, llvm::ConstantInt::get(txln_ctx.types.i8, bank), regnum, value);"
			        //	        "  }"
			        "}"

			        "inline void EmitRegisterWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, uint32_t bank, llvm::Value *value, bool __trace)"
			        "{"
			        "  llvm::Value *ptr = GetRegisterPointer(ctx, bank);"
			        "  ctx.block.region.builder.CreateStore(value, ptr);"
			        "  if(__trace){"
			        "    value = ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, value, txln_ctx.types.i32);"
			        "    ctx.block.region.builder.CreateCall3(txln_ctx.jit_functions.trace_reg_write, ctx.block.region.values.cpu_ctx_val, llvm::ConstantInt::get(txln_ctx.types.i8, bank), value);"
			        "  }"
			        "}";

			hstream <<
			        "inline void EmitBankedRegisterRead(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, llvm::Value*& output, uint32_t bank, llvm::Value *regnum, bool __trace)"
			        "{"
			        "  llvm::IRBuilder<> &builder = ctx.block.region.builder;"
			        "  llvm::Value *ptr = GetBankedRegisterPointer(ctx, bank, regnum);"
			        "  output = ctx.block.region.builder.CreateLoad(ptr);"
			        "  if(__trace){"
			        "    llvm::Value *value = ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, output, txln_ctx.types.i32);"
			        "    regnum = ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, regnum, txln_ctx.types.i32);"
			        "    ctx.block.region.builder.CreateCall4(txln_ctx.jit_functions.trace_reg_bank_read, ctx.block.region.values.cpu_ctx_val, llvm::ConstantInt::get(txln_ctx.types.i8, bank), regnum, value);"
			        "  }"
			        "}";

			hstream <<
			        "inline void EmitRegisterRead(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, llvm::Value *& output, uint32_t bank, bool __trace)"
			        "{"
			        "  llvm::Value *ptr = GetRegisterPointer(ctx, bank);"

			        "  output = ctx.block.region.builder.CreateLoad(ptr);"
			        "  if(__trace){"
			        "    llvm::Value *value = ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, output, txln_ctx.types.i32);"
			        "    ctx.block.region.builder.CreateCall3(txln_ctx.jit_functions.trace_reg_read, ctx.block.region.values.cpu_ctx_val, llvm::ConstantInt::get(txln_ctx.types.i8, bank),value);"
			        "  }"
			        "}";

			cstream << "#include <sstream>\n"
			        "#include <iomanip>\n"
			        "#include \"decode.h\"\n"
			        "#include \"translate.h\"\n"
			        "#include \"processor.h\"\n"
			        "#include <llvm/IR/Module.h>\n"
			        "#include <llvm/IR/IRBuilder.h>\n"
			        "namespace gensim { namespace " << Manager.GetArch().Name << " { \n"
			        "Translate::Translate(const gensim::Processor& cpu, archsim::translate::llvm::LLVMTranslationContext& ctx) : BaseLLVMTranslate(cpu, ctx) {}"
			        "std::string Translate::GetStateMacros() {return \"\";}\n"
			        "extern \"C\" { extern uint8_t _binary_precompiled_insns_bc_start; extern uint8_t _binary_precompiled_insns_bc_end;}"
			        "uint8_t *Translate::GetPrecompBitcode() { return &_binary_precompiled_insns_bc_start; }"
			        "uint32_t Translate::GetPrecompSize() { return &_binary_precompiled_insns_bc_end - &_binary_precompiled_insns_bc_start; }"


			        // add a block to the dynamic block queue
			        //"void Translate::QueueDynamicBlock(std::map<uint16, llvm::BasicBlock*> &block_map, std::list<uint16> &block_queue, llvm::Function *function, uint32 block_id)"
			        ;

			util::cppformatstream str;
			GenerateJumpInfo(str);
			cstream << str.str();
			if (!GeneratePrecompiledModule()) return false;

			emit_llvm_post_caller(cstream);
			emit_llvm_predicate_caller(cstream);
			emit_llvm_pre_caller(cstream);

			GenerateInstructionFunctionSelector(cstream, hstream);

			fprintf(stderr, "[TRANSLATE] Generating dynamic code JIT...");

			GenerateVirtualRegisterFunctions(cstream, hstream);

			GeneratePredicateEmitters(cstream, hstream);

			GenerateNonInlineFunctions(cstream);

			const int chunk_count = strtol(GetProperty("TranslationChunks").c_str(), NULL, 10);
			util::cppformatstream action_chunks[chunk_count];

			for (int i = 0; i < chunk_count; i++) {
				std::ostringstream chunk_file_name;
				chunk_file_name << "translate-chunk-" << i << ".cpp";

				sources.push_back(chunk_file_name.str());

				action_chunks[i] << "#include <sstream>\n"
				                 "#include <iomanip>\n"
				                 "#include \"decode.h\"\n"
				                 "#include \"translate.h\"\n"
				                 "#include \"processor.h\"\n"
				                 "#include <llvm/IR/Module.h>\n"
				                 "#include <llvm/IR/IRBuilder.h>\n";

				GenerateNonInlineFunctionPrototypes(action_chunks[i]);
			}



			int action_index = 0;
			for (std::list<isa::ISADescription *>::const_iterator II = Manager.GetArch().ISAs.begin(), IE = Manager.GetArch().ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;
				std::stringstream prefix;

				prefix << "Execute_" << isa->ISAName << "_";

				// loop through each emittable action
				for (const auto& execute_item : isa->GetSSAContext().Actions()) {
					if (execute_item.second->HasAttribute(genc::ActionAttribute::Helper)) continue;
					if (execute_item.second->HasAttribute(genc::ActionAttribute::External)) continue;

					auto execute = dynamic_cast<const SSAFormAction *>(execute_item.second);
					EmitDynamicEmitter(action_chunks[action_index % chunk_count], hstream, *execute, prefix.str());

					action_index++;
				}

			}

			hstream << "};"
			        "}}\n"
			        "#endif";

			cstream << "}}";

			std::ofstream hfilestream(hfile.str().c_str());
			hfilestream << hstream.str();

			std::ofstream cfilestream(cfile.str().c_str());
			cfilestream << cstream.str();

			for (int i = 0; i < chunk_count; i++) {
				std::ostringstream chunk_file_name;
				chunk_file_name << Manager.GetTarget() << "/translate-chunk-" << i << ".cpp";

				std::ofstream chunk_file_stream(chunk_file_name.str().c_str());
				chunk_file_stream << action_chunks[i].str();
			}

			fprintf(stderr, "(done)\n");

			return true;
		}

		bool DynamicTranslationGenerator::GenerateFixedBlockEmitter(util::cppformatstream &cfile, SSABlock &block) const
		{
			// we don't emit blocks like this for blocks with only one predecessor since we just emit them directly instead
			if (block.GetPredecessors().size() == 1) return true;

			if (block.IsFixed() != BLOCK_NEVER_CONST) {
				cfile << "block_" << block.GetName() << ":\n // BLOCK: START LINE " << block.GetStartLine() << ", END LINE " << block.GetEndLine() << "\n {";

				DynamicTranslationNodeWalkerFactory factory;

				for (auto statement_iterator = block.GetStatements().begin(); statement_iterator != block.GetStatements().end(); statement_iterator++) {
					SSAStatement &stmt = **statement_iterator;
					cfile << "// STMT: LINE " << stmt.GetDiag().Line() << "\n";
					cfile << "// " << stmt.ToString() << "\n";
					factory.GetOrCreate(&stmt)->EmitFixedCode(cfile, "fixed_done", true);
				}
				cfile << "}";
			}
			return true;
		}

		bool DynamicTranslationGenerator::GenerateDynamicBlockEmitter(util::cppformatstream &cfile, SSABlock &block) const
		{
			// we don't emit blocks like this for blocks with only one predecessor since we just emit them directly instead
			if (block.GetPredecessors().size() == 1) return true;

			if (block.IsFixed() != BLOCK_NEVER_CONST) {
				cfile << "block_" << block.GetName() << ": // BLOCK: START LINE " << block.GetStartLine() << ", END LINE " << block.GetEndLine() << "\n {";

				DynamicTranslationNodeWalkerFactory factory;

				for (auto statement_iterator = block.GetStatements().begin(); statement_iterator != block.GetStatements().end(); statement_iterator++) {
					SSAStatement &stmt = **statement_iterator;

					cfile << "// STMT: LINE " << stmt.GetDiag().Line() << "\n";
					cfile << "// " << stmt.ToString() << "\n";
					if (stmt.IsFixed())
						factory.GetOrCreate(&stmt)->EmitFixedCode(cfile, "fixed_done", true);
					else
						factory.GetOrCreate(&stmt)->EmitDynamicCode(cfile, "fixed_done", true);
				}
				cfile << "}";
			}
			return true;
		}

		bool DynamicTranslationGenerator::GenerateVirtualRegisterFunctions(util::cppformatstream &cstream, util::cppformatstream &hstream) const
		{
			hstream << "llvm::Value *AllocateVirtualVectorRegister(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name, llvm::Type* type);";
			hstream << "void SafeFreeVector(uint32_t name);";

			cstream << "llvm::Value *Translate::AllocateVirtualVectorRegister(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name, llvm::Type* type) {"
			        "llvm::Value *reg = new llvm::AllocaInst(type, std::string(\"\"), &ctx.region.entry_block->front());"
			        "llvm_registers[name] = reg;"
			        "return reg;"
			        "}";
			cstream << "void Translate::SafeFreeVector(uint32_t name) {"
			        "llvm_registers[name] = NULL;"
			        "}";

			if (GetProperty("SmartRegAlloc") == "0") {
				// For now, do no register management - allocate space separately for each variable and let
				// llvm sort it out
				for (int i = 8; i <= 64; i <<= 1) {
					cstream << "llvm::Value *Translate::AllocateVirtualRegister" << i << "(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name)"
					        "{"
					        "llvm::Value *reg = new llvm::AllocaInst(llvm::Type::getInt" << i << "Ty(txln_ctx.llvm_ctx), std::string(\"\"), &ctx.region.entry_block->front());"
					        "llvm_registers[name] = reg;"
					        "return reg;"
					        "}"

					        // We do nothing when freeing registers, just let LLVM sort it out later.
					        "#define FreeVirtualRegister" << i << "(x)\n"
					        "void Translate::SafeFreeVR" << i << "(uint32_t name)"
					        "{"
					        "}";

					hstream << "llvm::Value* AllocateVirtualRegister" << i << "(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name);";
					hstream << "llvm::Value* SafeFreeVR" << i << "(uint32_t name);";
				}

				// we also need regs for doubles/floats
				cstream << "llvm::Value* Translate::AllocateVirtualRegisterDouble(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name)"
				        "{"
				        "llvm::Value *reg = new llvm::AllocaInst(llvm::Type::getDoubleTy(txln_ctx.llvm_ctx), std::string(\"\"), &ctx.region.entry_block->front());"
				        "llvm_registers[name] = reg;"
				        "return reg;"
				        "}"
				        "void Translate::FreeVirtualRegisterDouble(uint32_t name)"
				        "{"
				        "}";

				hstream << "llvm::Value* AllocateVirtualRegisterDouble(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name);"
				        "void FreeVirtualRegisterDouble(uint32_t name);";

				cstream << "llvm::Value* Translate::AllocateVirtualRegisterFloat(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name)"
				        "{"
				        "llvm::Value *reg = new llvm::AllocaInst(llvm::Type::getFloatTy(txln_ctx.llvm_ctx), std::string(\"\"), &ctx.region.entry_block->front());"
				        "llvm_registers[name] = reg;"
				        "return reg;"
				        "}"
				        "void Translate::FreeVirtualRegisterFloat(uint32_t name)"
				        "{"
				        "}";

				hstream << "llvm::Value* AllocateVirtualRegisterFloat(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name);"
				        "void FreeVirtualRegisterFloat(uint32_t name);";
			} else { // Do slightly smarter stack frame allocation (only create a new stack frame if we need one)
				for (int i = 8; i <= 64; i <<= 1) {
					cstream << "llvm::Value* Translate::AllocateVirtualRegister" << i << "(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name)"
					        "{"
					        "llvm::Value *reg;"
					        "if(llvm_free_registers_" << i << ".size()) "
					        "{"
					        "   reg = llvm_free_registers_" << i << ".back(); "
					        "   llvm_free_registers_" << i << ".pop_back(); "
					        "} "
					        "else "
					        "  reg = new llvm::AllocaInst(llvm::Type::getInt" << i << "Ty(txln_ctx.llvm_ctx), std::string(\"\"), &ctx.region.entry_block->front());"
					        "llvm_registers[name] = reg;"
					        "return reg;"
					        "}"
					        "void Translate::FreeVirtualRegister" << i << "(uint32_t name)"
					        "{"
					        "}"
					        "void Translate::SafeFreeVR" << i << "(uint32_t name)"
					        "{"
					        "llvm::Value *val = llvm_registers[name];"
					        "llvm_registers[name] = NULL;"
					        "if(val) llvm_free_registers_" << i << ".push_back(val);"
					        "}";

					hstream << "llvm::Value* AllocateVirtualRegister" << i << "(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name);"
					        "void FreeVirtualRegister" << i << "(uint32_t name);"
					        "void SafeFreeVR" << i << "(uint32_t name);";
				}

				// we also need regs for doubles/floats
				cstream << "llvm::Value* Translate::AllocateVirtualRegisterDouble(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name)"
				        "{"
				        "llvm::Value *reg;"
				        "if(llvm_free_registers_double.size()) "
				        "{"
				        "   reg = llvm_free_registers_double.back(); "
				        "   llvm_free_registers_double.pop_back(); "
				        "} "
				        "else "
				        "  reg = new llvm::AllocaInst(llvm::Type::getDoubleTy(txln_ctx.llvm_ctx), std::string(\"\"), &ctx.region.entry_block->front());"
				        "llvm_registers[name] = reg;"
				        "return reg;"
				        "}"
				        "void Translate::FreeVirtualRegisterDouble(uint32_t name)"
				        "{"
				        "}";

				hstream << "llvm::Value* AllocateVirtualRegisterDouble(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name);"
				        "void FreeVirtualRegisterDouble(uint32_t name);";

				cstream << "llvm::Value* Translate::AllocateVirtualRegisterFloat(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name)"
				        "{"
				        "llvm::Value *reg;"
				        "if(llvm_free_registers_float.size()) "
				        "{"
				        "   reg = llvm_free_registers_float.back(); "
				        "   llvm_free_registers_float.pop_back(); "
				        "} "
				        "else "
				        "  reg = new llvm::AllocaInst(llvm::Type::getFloatTy(txln_ctx.llvm_ctx), std::string(\"\"), &ctx.region.entry_block->front());"
				        "llvm_registers[name] = reg;"
				        "return reg;"
				        "}"
				        "void Translate::FreeVirtualRegisterFloat(uint32_t name)"
				        "{"

				        "}";

				hstream << "llvm::Value* AllocateVirtualRegisterFloat(archsim::translate::llvm::LLVMBlockTranslationContext& ctx, uint32_t name);"
				        "void FreeVirtualRegisterFloat(uint32_t name);";
			}
			return true;
		}

		bool DynamicTranslationGenerator::GenerateInstructionFunctionSelector(util::cppformatstream &cfile, util::cppformatstream &hfile) const
		{
			const arch::ArchDescription &arch = Manager.GetArch();

			cfile << "bool Translate::TranslateInstruction(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, llvm::Value*& __result, bool trace)"
			      "{"
			      " switch(ctx.tiu.GetDecode().Instr_Code)"
			      "{";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator inst = isa->Instructions.begin(); inst != isa->Instructions.end(); ++inst) {
					if (isa->GetSSAContext().HasAction(inst->second->BehaviourName)) cfile << "case INST_" << isa->ISAName << "_" << inst->second->Name << ": Execute_" << isa->ISAName << "_" << inst->second->BehaviourName << "(ctx, __result, trace); return true;";
				}
			}

			cfile << "}"
			      "return false;"
			      "}";
			return true;
		}

		void DynamicTranslationGenerator::Reset()
		{
		}

		void DynamicTranslationGenerator::Setup(GenerationSetupManager &Setup)
		{
			// add objcopy step to makefile
			MakefileGenerator *makefile_gen = (MakefileGenerator *) Setup.GetComponent("make");
			if (makefile_gen != NULL) {
				makefile_gen->AddObjectFile("precompiled_insns.o");
				makefile_gen->AddPreBuildStep("objcopy --input binary --output elf64-x86-64 --binary-architecture i386 precompiled_insns.bc precompiled_insns.o");
			}
		}

		std::string DynamicTranslationGenerator::GetFunction() const
		{
			return GenerationManager::FnTranslate;
		}

		DynamicTranslationGenerator::~DynamicTranslationGenerator()
		{
		}

		bool DynamicTranslationGenerator::GeneratePrecompiledModule() const
		{
			const arch::ArchDescription &arch = Manager.GetArch();

			InterpretiveExecutionEngineGenerator *interp = (InterpretiveExecutionEngineGenerator *) Manager.GetComponentC(GenerationManager::FnInterpret);
			if (interp == NULL) {
				return false;
			}
			std::ofstream stream("precomp.c");

			//      InterpretiveExecutionEngineGenerator *interp = (InterpretiveExecutionEngineGenerator*) Manager.GetComponentC(GenerationManager::FnInterpret);
			stream << "#include <stdio.h>\n";
			stream << "#include <stdlib.h>\n"; // needed for abort()
			stream << "#include \"" << GetProperty("ArcsimPath") << "/inc/gensim/gensim_processor_state.h\"\n";
			//      stream << Manager.GetArch().GetIncludes();
			//
			//      stream << QUOTEME(TYPEDEF_TYPES); //todo: memory and pipeline modelling
			//      stream << QUOTEME(ENUM_MEMORY_TYPE_TAGS);

			stream << interp->GetStateStruct();
			//      stream << QUOTEME(STRUCT_CPU_STATE JIT_TYPES JIT_API) << "\n\n";


			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				if (isa->HasBehaviourAction("instruction_begin")) {
					emit_behaviour_fn(stream, *isa, "pre", isa->GetBehaviourAction("instruction_begin"), false, "void");
					emit_behaviour_fn(stream, *isa, "pre", isa->GetBehaviourAction("instruction_begin"), true, "void");
				}

				//		emit_behaviour_fn(stream, *isa, "instruction_predicate", isa->GetBehaviourAction("instruction_predicate"), false, "uint8");
				//		emit_behaviour_fn(stream, *isa, "instruction_predicate", isa->GetBehaviourAction("instruction_predicate"), true, "uint8");

				if (isa->HasBehaviourAction("instruction_end")) {
					emit_behaviour_fn(stream, *isa, "post", isa->GetBehaviourAction("instruction_end"), false, "void");
					emit_behaviour_fn(stream, *isa, "post", isa->GetBehaviourAction("instruction_end"), true, "void");
				}

				const arch::RegSlotViewDescriptor *pc_slot = Manager.GetArch().GetRegFile().GetTaggedRegSlot("PC");
				const arch::RegSlotViewDescriptor *sp_slot = Manager.GetArch().GetRegFile().GetTaggedRegSlot("SP");

				// emit PC read and write functions
				stream << "inline uint32_t " << isa->ISAName << "_fast_read_pc(struct cpuState * const __state, struct gensim_state *const regs) {\n";
				stream << "return *(" << pc_slot->GetIRType().GetCType() << "*)(((uint8_t*)regs) + " << pc_slot->GetRegFileOffset() << ");";
				stream << "\n}\n";

				stream << "inline void " << isa->ISAName << "_fast_write_pc(struct cpuState *const __state, struct gensim_state *const regs, uint32_t val) {\n";
				stream << "*(" << pc_slot->GetIRType().GetCType() << "*)(((uint8_t*)regs) + " << pc_slot->GetRegFileOffset() << ") = val;";
				stream << "\n}\n";
			}

			// emit ISA mode read and write functions
			stream << "inline uint8_t fast_get_isa_mode(struct cpuState * const __state) {\n";
			stream << "\treturn __state->isa_mode;";
			stream << "\n}\n";

			stream << "inline void fast_set_isa_mode(struct cpuState *const __state, uint8_t val) {\n";
			stream << "\t__state->isa_mode = val;";
			stream << "\n}\n";

			stream.close();

			std::stringstream cline_str;
			cline_str << GetProperty("Clang");
			std::vector<std::string> include_dirs;
			for (std::vector<std::string>::const_iterator idir = include_dirs.begin(); idir != include_dirs.end(); idir++) {
				cline_str << " -I output/" << *idir;
			}
			cline_str << " -std=gnu89 -emit-llvm -I" << GetProperty("ArcsimPath") <<  "/inc -c "
			          << "precomp.c"
			          << " -o " << Manager.GetTarget() << "/precompiled_insns.bc ";

			if (HasProperty("Optimize")) {
				if (GetProperty("Optimize") == "1")
					cline_str << " -O1";
				else if (GetProperty("Optimize") == "2")
					cline_str << " -O2";
				else if (GetProperty("Optimize") == "3")
					cline_str << " -O3";
				else
					cline_str << " -O0";
			}

			if (HasProperty("InlineThreshold")) {
				cline_str << " -mllvm -inline-threshold=" << GetProperty("InlineThreshold") << " ";
				printf("[TRANSLATE] Using inline threshold %s\n", GetProperty("InlineThreshold").c_str());
			}

			if (HasProperty("InlineHint-Threshold")) {
				cline_str << " -mllvm -inlinehint-threshold=" << GetProperty("InlineHint-Threshold") << " ";
				printf("[TRANSLATE] Using inline hint threshold %s\n", GetProperty("InlineHint-Threshold").c_str());
			}

			fprintf(stderr, "[TRANSLATE] Invoking clang with %s...", cline_str.str().c_str());

			if (system(cline_str.str().c_str())) {
				fprintf(stderr, "LLVM Compilation failed\n");
				return false;
			}
			fprintf(stderr, "(done)\n");

			return true;
		}

		void DynamicTranslationGenerator::emit_llvm_values_for_generic(util::cppformatstream &target, const std::map<std::string, std::string> fields) const
		{
			target << "const llvm::Type *cpuStateType = stateVal->getType();\n";

			target << "std::vector<llvm::Value*> values;";
			target << "values.push_back(stateVal);\n";

			target << "const llvm::Type *regStateType = regStateVal->getType();\n";
			target << "values.push_back(regStateVal);\n";

			target << "LLVM_TYPE_PTR i32Type = llvm::Type::getInt32Ty(module->getContext());";
			target << "values.push_back(llvm::ConstantInt::get(i32Type, PC, false));\n";

			for (std::map<std::string, std::string>::const_iterator field = fields.begin(); field != fields.end(); ++field) {
#ifdef ENABLE_LIMM_OPERATIONS
				// special handling for limm
				if (field->first == "LimmPtr") {
					target << "std::vector<llvm::Constant*> limm_constant;"
					       "for(int i = 0; i < instr->LimmBytes/4; i++)"
					       "{"
					       "limm_constant.push_back(llvm::ConstantInt::get(i32Type, ((uint32*)instr->LimmPtr)[i]));"
					       "}"
					       "llvm::Constant *constArray = llvm::ConstantArray::get(llvm::ArrayType::get(i32Type, instr->LimmBytes/4), limm_constant);"
					       "llvm::GlobalVariable *_LimmPtr = new llvm::GlobalVariable(constArray->getType(), true, llvm::GlobalValue::InternalLinkage, constArray);"
					       "values.push_back(_LimmPtr);"
					       "const llvm::Type *_LimmPtr_type = constArray->getType();";
				}
#else
				if (field->first == "LimmPtr") continue;
#endif
				else {
					std::ostringstream fieldName;
					fieldName << "instr->" << field->first;
					target << "llvm::Value * _" << field->first << " = (llvm::Value*)" << GetLLVMValue(field->second, fieldName.str()) << ";\n";
					target << "values.push_back(_" << field->first << ");\n";
					target << "const llvm::Type *_" << field->first << "_type = _" << field->first << "->getType();\n";
				}
			}
		}

		std::string DynamicTranslationGenerator::GetLLVMValue(std::string typeName, std::string value)
		{
			std::stringstream outStream;
			std::string typeString;

			bool is_ptr = typeName.find('*') != typeName.npos;
			std::string castTypeString = typeName;
			if (is_ptr) {
				outStream << "llvm::ConstantExpr::getIntToPtr(";
				typeName = util::Util::FindReplace(typeName, "*", "");
				castTypeString = "size_t";
			}

			if (!typeName.compare("uint8")) {
				typeString = "llvm::IntegerType::getInt8Ty(module->getContext())";
				outStream << "llvm::ConstantInt::get(" << typeString << ", (" << castTypeString << ")" << value << ", false)";
			} else if (!typeName.compare("uint16")) {
				typeString = "llvm::IntegerType::getInt16Ty(module->getContext())";
				outStream << "llvm::ConstantInt::get(" << typeString << ", (" << castTypeString << ")" << value << ", false)";
			} else if (!typeName.compare("uint32")) {
				typeString = "llvm::IntegerType::getInt32Ty(module->getContext())";
				outStream << "llvm::ConstantInt::get(" << typeString << ", (" << castTypeString << ")" << value << ", false)";
			} else if (!typeName.compare("sint32")) {
				typeString = "llvm::IntegerType::getInt32Ty(module->getContext())";
				outStream << "llvm::ConstantInt::get(" << typeString << ", (" << castTypeString << ")" << value << ", true)";
			} else {
				fprintf(stderr, "Unknown type: %s\n", typeName.c_str());
				exit(1);
			}

			if (is_ptr) outStream << ", llvm::PointerType::get(" << typeString << ",0))";

			return outStream.str();
		}

		void DynamicTranslationGenerator::emit_llvm_call(const isa::ISADescription &isa, util::cppformatstream &target, std::string function, std::string returnType, bool ret_call, bool quote_fn) const
		{
			/*	// emit_llvm_values_for_decode should already have been called by this point.
				const std::map<std::string, std::string> &fields = isa.Get_Disasm_Fields();
				// now, do a getOrInsertFunction call to get a pointer to the function we are calling
				if (quote_fn) function = "\"" + function + "\"";

				target << "llvm::Function *fn = (llvm::Function*)module->getOrInsertFunction(" << function << ", " << returnType << ", cpuStateType, regStateType, i32Type, ";
				for (std::map<std::string, std::string>::const_iterator field = fields.begin(); field != fields.end(); ++field) {
			#ifndef ENABLE_LIMM_OPERATIONS
					if (field->first == "LimmPtr") continue;  // XXX ignore limms for now
			#endif
					target << "_" << field->first << "_type, ";
				}
				target << " NULL);\n";

				// now emit a call to the function, passing the appropriate arguments
				if (ret_call) target << "return ";
				target << "irBuilder.CreateCall((llvm::Value*)fn, values, \"\");\n";*/
		}

		void DynamicTranslationGenerator::emit_llvm_pre_caller(util::cppformatstream &source) const
		{
			/*const arch::ArchDescription &arch = Manager.GetArch();

			source << "bool Translate::EmitLLVMCallPre(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, bool trace)"
				   "{\n"
				   "Decode *instr = (Decode*)base_instr;\n";

			source << "switch (instr->isa_mode) {\n";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				source << "case ISA_MODE_" << isa->ISAName << ": {\n";
				if (isa->HasBehaviourAction("instruction_begin")) {
					// first, create a value representing each instruction field
					const std::map<std::string, std::string> fields = isa->Get_Disasm_Fields();
					emit_llvm_values_for_generic(source, fields);

					// todo: emit a tracing version
					source << "if(trace) {";
					emit_llvm_call(*isa, source, "pre_trace", "llvm::Type::getVoidTy(module->getContext())", true, true);
					source << " } else {";
					emit_llvm_call(*isa, source, "pre", "llvm::Type::getVoidTy(module->getContext())", true, true);
					source << "}";
				}
				source << "} break;\n";
			}

			source << "}\n";
			source << "}\n";*/
		}

		void DynamicTranslationGenerator::emit_llvm_post_caller(util::cppformatstream &source) const
		{
			/*const arch::ArchDescription &arch = Manager.GetArch();

			source << "bool Translate::EmitLLVMCallPost(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, bool trace)"
				   "{\n"
				   "Decode *instr = (Decode*)base_instr;\n";

			source << "switch (instr->isa_mode) {\n";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				source << "case ISA_MODE_" << isa->ISAName << ": {\n";
				if (isa->HasBehaviourAction("instruction_end")) {
					// first, create a value representing each instruction field
					const std::map<std::string, std::string> fields = isa->Get_Disasm_Fields();
					emit_llvm_values_for_generic(source, fields);

					// todo: emit a tracing version
					source << "if(trace) {";
					emit_llvm_call(*isa, source, "post_trace", "llvm::Type::getVoidTy(module->getContext())", true, true);
					source << " } else {";
					emit_llvm_call(*isa, source, "post", "llvm::Type::getVoidTy(module->getContext())", true, true);
					source << "}";
				}
				source << "} break;\n";
			}

			source << "}\n";
			source << "}\n";*/
		}

		void DynamicTranslationGenerator::emit_llvm_predicate_caller(util::cppformatstream &source) const
		{
			/*const arch::ArchDescription &arch = Manager.GetArch();

			source << "llvm::CallInst * Translate::EmitLLVMCallPredicate(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, bool trace)"
				   "{\n"
				   "Decode *instr = (Decode*)base_instr;\n";

			source << "switch (instr->isa_mode) {\n";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				source << "case ISA_MODE_" << isa->ISAName << ": {\n";

				// first, create a value representing each instruction field
				const std::map<std::string, std::string> fields = isa->Get_Disasm_Fields();
				emit_llvm_values_for_generic(source, fields);

				std::stringstream fname;

				fname << isa->ISAName << "_instruction_predicate";

				// todo: emit a tracing version
				source << "if(trace) {";
				emit_llvm_call(*isa, source, fname.str(), "llvm::Type::getInt8Ty(module->getContext())", true, true);
				source << "} else {";
				emit_llvm_call(*isa, source, fname.str(), "llvm::Type::getInt8Ty(module->getContext())", true, true);
				source << "}";

				source << "} break;\n";
			}

			source << "}\n";
			source << "}\n";*/
		}

		void DynamicTranslationGenerator::GenerateGraphs() const
		{
			for (std::list<isa::ISADescription *>::const_iterator II = Manager.GetArch().ISAs.begin(), IE = Manager.GetArch().ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;
				const genc::ssa::SSAContext &ssa_ctx = isa->GetSSAContext();

				for (const auto& action_item : ssa_ctx.Actions()) {
					std::ostringstream name_stream;
					name_stream << "graphs/" << action_item.first << ".dot";
					std::ofstream out_str(name_stream.str().c_str());
					name_stream.str("");

					genc::ssa::printers::SSAActionCFGPrinter action_cfg_printer(*action_item.second);
					action_cfg_printer.Print(name_stream);

					out_str << name_stream.str();
				}
			}
		}
	}
}
