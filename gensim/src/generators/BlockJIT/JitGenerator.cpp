/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "isa/InstructionDescription.h"
#include "isa/InstructionFormatDescription.h"

#include "generators/BlockJIT/JitGenerator.h"
#include "generators/GenCInterpreter/GenCInterpreterGenerator.h"

#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ir/IRAction.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/Parser.h"

using namespace gensim;
using namespace gensim::generator;
using namespace gensim::genc::ssa;
using namespace gensim::genc;
using namespace gensim::isa;

DEFINE_COMPONENT(JitGenerator, jit)
COMPONENT_INHERITS(jit, translate)

JitGenerator::JitGenerator(GenerationManager& man) : GenerationComponent(man, "jit") { }

bool JitGenerator::Generate() const
{
	util::cppformatstream src_stream, hdr_stream;

	if (!GenerateHeader(hdr_stream))
		return false;

	if (!GenerateSource(src_stream))
		return false;

	std::ostringstream hdr_file;
	hdr_file << Manager.GetTarget() << "/jit.h";

	std::ostringstream src_file;
	src_file << Manager.GetTarget() << "/jit.cpp";

	std::ofstream hdr_file_stream(hdr_file.str());
	hdr_file_stream << hdr_stream.str();

	std::ofstream src_file_stream(src_file.str());
	src_file_stream << src_stream.str();

	sources.push_back("jit.cpp");

	GenerateJitChunks(5);

	return true;
}

bool JitGenerator::GenerateJitChunks(int count) const
{
	std::map<uint32_t, std::vector<std::pair<isa::ISADescription*, isa::InstructionDescription*> > > chunks;

	uint32_t idx = 0;
	for(const auto &isa : Manager.GetArch().ISAs) {
		for(const auto &insn : isa->Instructions) {
			chunks[idx].push_back(std::make_pair(isa, insn.second));
			idx++;
			idx %= count;
		}
	}

	for(const auto &i : chunks) {
		util::cppformatstream stream;

		stream
		        << "#include \"blockjit/translation-context.h\"\n"
		        << "#include \"blockjit/IRBuilder.h\"\n"
		        << "#include \"decode.h\"\n"
		        << "#include \"jit.h\"\n"
		        << "#include \"processor.h\"\n"

		        << "#include <queue>\n"
		        << "#include <set>\n"

		        << "using namespace captive::shared;"
		        << "using namespace captive::arch::" << Manager.GetArch().Name << ";"
		        << "using gensim::" << Manager.GetArch().Name << "::Decode;"
		        << "using namespace gensim::" << Manager.GetArch().Name << ";";

		for(auto &isa : Manager.GetArch().ISAs) {
			RegisterHelpers(isa);
		}

		std::ostringstream str;
		str << Manager.GetTarget() << "/jit-chunk-" << i.first << ".cpp";

		std::ofstream out_str (str.str());
		out_str << stream.str();

		str.str("");
		str << "jit-chunk-" << i.first << ".cpp";
		sources.push_back(str.str());
	}

	return true;
}

bool JitGenerator::RegisterHelpers(const isa::ISADescription *isa) const
{
//SSAContext::ActionListConstIterator HI = isa->GetSSAContext().HelpersBegin(), HE = isa->GetSSAContext().HelpersEnd(); HI != HE; ++HI

	GenCInterpreterGenerator interp (Manager);

	for (const auto& action_item : isa->GetSSAContext().Actions()) {
		if (!action_item.second->HasAttribute(ActionAttribute::Helper)) continue;

		auto action = dynamic_cast<const SSAFormAction *>(action_item.second);

		if (action->GetPrototype().GetIRSignature().GetName() == "instruction_predicate") continue;
		if (action->GetPrototype().GetIRSignature().GetName() == "instruction_is_predicated") continue;

		util::cppformatstream prototype_stream;
		interp.GeneratePrototype(prototype_stream, *isa, *action);

		util::cppformatstream body_stream;
		body_stream << "{";
		body_stream << "gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
		// generate helper function code inline here

		interp.GenerateExecuteBodyFor(body_stream, *action);

		body_stream << "}";

		Manager.AddFunctionEntry(FunctionEntry(prototype_stream.str(), body_stream.str(), {}, {"cstdint", "core/thread/ThreadInstance.h","util/Vector.h"}, {},true));
	}

	return true;
}

bool JitGenerator::GenerateHeader(util::cppformatstream & hdr_stream) const
{
	const arch::ArchDescription &arch = Manager.GetArch();

	hdr_stream
	        << "#ifndef _JIT_HEADER\n#define _JIT_HEADER\n"
	        << "#include \"decode.h\"\n"
	        << "#include \"arch.h\"\n"
	        << "#include <blockjit/translation-context.h>\n"
	        << "#include <blockjit/BlockJitTranslate.h>\n"
	        << "#include <util/SimOptions.h>\n"
	        << "#include <translate/jit_funs.h>\n";

	GenerateClass(hdr_stream);

	hdr_stream << "#endif\n";

	return true;
}

bool JitGenerator::GenerateClass(util::cppformatstream& str) const
{
	const arch::ArchDescription &arch = Manager.GetArch();

	str     << "using gensim::" << arch.Name << "::Decode;"

	        << "namespace captive {"
	        << "	namespace arch {\n"
	        << "		namespace " << arch.Name << " {\n"

	        << "			class JIT : public gensim::blockjit::BaseBlockJITTranslate {"
	        << "			public:\n"
	        << "				bool translate_instruction(const gensim::BaseDecode* decode_obj, captive::shared::IRBuilder &builder, bool trace) ;"

	        << "			private:\n";

	for (auto isa : arch.ISAs) {

		for (auto fmt : isa->Formats) {
			str << "captive::shared::IRRegId generate_predicate_" << isa->ISAName << "_" << fmt.second->GetName() << "(const Decode& insn, captive::shared::IRBuilder &builder, bool trace);\n";
		}

		for (auto insn : isa->Instructions) {
			str << "bool translate_" << isa->ISAName << "_" << insn.second->Name << "(const Decode& insn, captive::shared::IRBuilder &builder, bool trace);";
		}
	}

	str	<< "			};"

	    << "		}"
	    << "	}"
	    << "}";

	return true;
}


bool JitGenerator::GenerateSource(util::cppformatstream & src_stream) const
{
	const arch::ArchDescription &arch = Manager.GetArch();

	src_stream
	        << "#include \"blockjit/translation-context.h\"\n"
	        << "#include \"decode.h\"\n"
	        << "#include \"jit.h\"\n"
	        << "#include \"processor.h\"\n"

	        << "#include <queue>\n"
	        << "#include <set>\n";

	GenerateTranslation(src_stream);

	return true;
}

bool JitGenerator::GenerateTranslation(util::cppformatstream& src_stream) const
{
	const arch::ArchDescription &arch = Manager.GetArch();

	src_stream

	        << "using namespace captive::shared;"
	        << "using namespace captive::arch::" << arch.Name << ";"
	        << "using gensim::" << arch.Name << "::Decode;"
	        << "using namespace gensim::" << arch.Name << ";"

	        << "bool JIT::translate_instruction(const gensim::BaseDecode* decode_obj, captive::shared::IRBuilder &builder, bool trace)"
	        << "{"
	        << "	const Decode& insn = (const Decode&)*(const Decode *)decode_obj;"
	        << "	switch (insn.Instr_Code) {";

	for (auto isa : arch.ISAs) {
		for (auto insn : isa->Instructions) {
			src_stream << "case INST_" << isa->ISAName << "_" << insn.second->Name << ":\n";
			src_stream << "return translate_" << isa->ISAName << "_" << insn.second->Name << "((const Decode&)insn, builder, trace);";
		}
	}

	src_stream	<< "	default:\n"
	            << "		return false;"
	            << "	}"
	            << "}";

	const auto isa = arch.ISAs.front();


	for (auto isa : arch.ISAs) {
		for (auto fmt : isa->Formats) {
			if (fmt.second->CanBePredicated()) {
				GeneratePredicateFunction(src_stream, *isa, *fmt.second);
			}
		}
	}

	return true;
}


bool JitGenerator::GeneratePredicateFunction(util::cppformatstream &src_stream, const isa::ISADescription& isa, const isa::InstructionFormatDescription& fmt) const
{
	src_stream << "IRRegId JIT::generate_predicate_" << isa.ISAName << "_" << fmt.GetName() << "(const Decode& insn, captive::shared::IRBuilder &builder, bool trace)";
	src_stream << "{"
	           << "std::queue<IRBlockId> dynamic_block_queue;";
	src_stream << "IRRegId __result = builder.alloc_reg(1);";

	src_stream << "IRBlockId __exit_block = builder.alloc_block();\n";

	auto insn_predicate_action = dynamic_cast<const SSAFormAction *>(isa.GetSSAContext().GetAction("instruction_predicate"));
	EmitJITFunction(src_stream, *insn_predicate_action);

	src_stream << "	return __result;";
	src_stream << "}";

	return true;
}

bool JitGenerator::RegisterJITFunction(const ISADescription& isa, const InstructionDescription& insn) const
{
	JitNodeWalkerFactory factory;

	const arch::ArchDescription &arch = Manager.GetArch();
	auto ssa_form_action = dynamic_cast<const SSAFormAction *>(isa.GetSSAContext().GetAction(insn.BehaviourName));
	assert(ssa_form_action);

	const SSAFormAction &action = *ssa_form_action;

	util::cppformatstream src_stream;
	std::string prototype = "bool captive::arch::" + Manager.GetArch().Name + "::JIT::translate_" + isa.ISAName + "_" + insn.Name + "(const Decode&insn, captive::shared::IRBuilder &builder, bool trace)";
	src_stream  << "{"
	            << "using namespace captive::shared;"
	            << "std::queue<IRBlockId> dynamic_block_queue;";

	src_stream << "IRBlockId __exit_block = 0xf0f0f0f0;\n";

	if (insn.Format->CanBePredicated()) {
//		XXX ARM HAX
		src_stream << "if (insn.GetIsPredicated()) {";
//		src_stream << "if(((uint32_t)insn.GetIR() & 0xf0000000UL) < 0xe0000000UL) {";
		src_stream << "IRBlockId __predicate_taken = builder.alloc_block();";

		src_stream << "IRRegId predicate_result = generate_predicate_" << isa.ISAName << "_" << insn.Format->GetName() << "(insn, builder, trace);";

		src_stream << "if (insn.GetEndOfBlock()) {";
		src_stream << "IRBlockId __predicate_not_taken = builder.alloc_block();";
		src_stream << "__exit_block = builder.alloc_block();";
		src_stream << "builder.branch(IROperand::vreg(predicate_result, 1), IROperand::block(__predicate_taken), IROperand::block(__predicate_not_taken));\n";
		src_stream << "builder.SetBlock(__predicate_not_taken);";
		src_stream << "builder.increment_pc(" << (insn.Format->GetLength()/8) << ");";
		src_stream << "builder.jump(IROperand::block(__exit_block));";
		src_stream << "} else {";
		src_stream << "__exit_block = builder.alloc_block();";
		src_stream << "builder.branch(IROperand::vreg(predicate_result, 1), IROperand::block(__predicate_taken), IROperand::block(__exit_block));\n";
		src_stream << "}";

		src_stream << "builder.SetBlock(__predicate_taken);";
		src_stream << "} else {__exit_block = builder.alloc_block(); }";
	} else {
		src_stream << "__exit_block = builder.alloc_block();";
	}

	EmitJITFunction(src_stream, action);

	src_stream << "if (!insn.GetEndOfBlock()) {";
	src_stream << "builder.increment_pc(" << (insn.Format->GetLength()/8) << ");";
	src_stream << "}";

	src_stream	<< "	return true;"
	            << "}";

	Manager.AddFunctionEntry(FunctionEntry(prototype, src_stream.str(), {"ee_blockjit.h"}, {"queue", "translate/jit_funs.h"}));

	return true;
}

bool JitGenerator::EmitJITFunction(util::cppformatstream &src_stream, const SSAFormAction& action) const
{
	JitNodeWalkerFactory factory;

	for (const auto block : action.GetBlocks()) {
		src_stream << "// Block " << block->GetName() << "\n";
		src_stream << "const IRBlockId block_idx_" << block->GetName() << " = builder.alloc_block();\n";
	}

	for(const auto symbol : action.Symbols()) {
		if(symbol->GetType().Reference) {
			continue;
		}
		src_stream << "// Reg " << symbol->GetName() << std::endl;
		src_stream << symbol->GetType().GetCType() << " CV_" << symbol->GetName() << ";";
		src_stream << "const IRRegId ir_idx_" << symbol->GetName() << " = builder.alloc_reg(" << symbol->GetType().Size() << ");";
	}

	src_stream << "goto block_" << action.EntryBlock->GetName() << ";\n";
	for (const auto block : action.GetBlocks()) {
		if (block->IsFixed() != BLOCK_ALWAYS_CONST) {
			src_stream << "// BLOCK " << block->GetName() << " not fully fixed\n";
			continue;
		}

		src_stream << "block_" << block->GetName() << ": {\n";

		for (const auto stmt : block->GetStatements()) {
			auto walker = factory.Create(stmt);

			src_stream << "/* " << stmt->GetDiag() << " ";
			if (stmt->IsFixed())
				src_stream << "[F] ";
			else
				src_stream << "[D] ";
			src_stream << stmt->ToString() << " */\n";


			if (stmt->IsFixed())
				walker->EmitFixedCode(src_stream, "fixed_done", true);
			else
				walker->EmitDynamicCode(src_stream, "fixed_done", true);
		}

		src_stream << "}\n";

	}

	src_stream << "fixed_done:\n";
	src_stream << "if (dynamic_block_queue.size() == 0) {\n";
	src_stream << "builder.jump(IROperand::block(__exit_block));\n";
	src_stream << "} else {";

	src_stream << "std::set<IRBlockId> emitted_blocks;";
	src_stream << "while(dynamic_block_queue.size() > 0) {\n";

	src_stream	<< "IRBlockId block_index = dynamic_block_queue.front();"
	            << "dynamic_block_queue.pop();"
	            << "if(emitted_blocks.find(block_index) != emitted_blocks.end()) continue;"
	            << "emitted_blocks.insert(block_index);";

	for (const auto block : action.GetBlocks()) {
		if (block->IsFixed() == BLOCK_ALWAYS_CONST) continue;

		src_stream << "if (block_index == block_idx_" << block->GetName() << ") // BLOCK START LINE " << block->GetStartLine() << ", END LINE " << block->GetEndLine() << "\n";
		src_stream << "{";
		src_stream << "builder.SetBlock(block_idx_" << block->GetName() << ");";

		for (const auto stmt : block->GetStatements()) {
			src_stream << "/* " << stmt->GetDiag() << " ";
			if (stmt->IsFixed())
				src_stream << "[F] ";
			else
				src_stream << "[D] ";
			src_stream << stmt->ToString() << " */\n";

			if (stmt->IsFixed())
				factory.GetOrCreate(stmt)->EmitFixedCode(src_stream, "", false);
			else
				factory.GetOrCreate(stmt)->EmitDynamicCode(src_stream, "", false);
		}

		src_stream << "continue; }";
	}

	src_stream << "}\n";	// WHILE
	src_stream << "}\n";	// ELSE

	src_stream << "dynamic_done:\n";
	src_stream << "builder.SetBlock(__exit_block);";

	return true;
}
