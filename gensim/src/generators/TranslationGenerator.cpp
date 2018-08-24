/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <fstream>
#include <sstream>
#include <string>

#include <ctime>
#include <cstring>

#include "arch/ArchDescription.h"
#include "generators/TranslationGenerator.h"
#include "isa/ISADescription.h"
#include "generators/InterpretiveExecutionEngineGenerator.h"

DEFINE_DUMMY_COMPONENT(gensim::generator::TranslationGenerator, translate)
COMPONENT_OPTION(translate, DirectMemory, "0", "Enable direct memory access, which significantly improves simulation performance but causes issues with self modifying code")

namespace gensim
{
	namespace generator
	{

		TranslationGenerator::~TranslationGenerator() {}

		std::string EmissionChunk::Format(std::list<EmissionChunk> chunks)
		{
			bool quoted = !chunks.front().Quoted;
			std::ostringstream out;
			for (std::list<EmissionChunk>::iterator i = chunks.begin(); i != chunks.end(); ++i) {
				if (i->Quoted)
					out << "\"" << i->Text << "\"";
				else
					out << i->Text;
			}
			return out.str();
		}

		std::list<EmissionChunk> EmissionChunk::Parse(std::string str)
		{
			std::stringstream currChunk;

			std::list<EmissionChunk> chunks = std::list<EmissionChunk>();

			bool quoted = false;
			for (std::string::iterator i = str.begin(); i != str.end(); ++i) {
				if (*i == '\\') { // if this char is a backslash, emit this char and the next directly
					currChunk << *i;
					currChunk << *(++i);
					continue;
				}
				if (*i == '\"') {
					std::string str = currChunk.str();
					if (str.length()) {
						EmissionChunk ec = EmissionChunk(quoted, str);
						chunks.push_back(ec);
					}
					quoted = !quoted;
					currChunk.str("");
				} else
					currChunk << (char)*i;
			}
			EmissionChunk ec = EmissionChunk(true, currChunk.str());
			chunks.push_back(ec);
			return chunks;
		}

		std::string TranslationGenerator::inline_inst(const isa::ISADescription &isa, std::string source, std::string object) const
		{
			std::stringstream str;
			std::string objstr = object;
			char *chars = (char *)malloc(source.length() + 1);
			strcpy(chars, source.c_str());
			std::vector<std::string> tokens = util::Util::Tokenize(source, " \t()*&;[],", true, true);
			for (std::vector<std::string>::iterator i = tokens.begin(); i != tokens.end(); ++i) {
				std::string token = *i;
				if (token.find(objstr) != token.npos) {
					std::string member = token.substr(token.find(".") + 1);
					if (isa.Get_Disasm_Fields().find(member) == isa.Get_Disasm_Fields().end()) {
						// hacky mc hack
						if (member != "IsPredicated") {
							fprintf(stderr, "Error: Unknown instruction member %s\n", member.c_str());
							free(chars);
							return str.str();
						}
					}
					str << "(" << isa.Get_Disasm_Fields().at(member) << ")(0x\" << std::hex << (size_t)(" << object << member << ") << \")";
				} else {
					str << token;
				}
			}
			free(chars);
			return str.str();
		}

		int get_matching_bracket(std::string str, int start)
		{
			assert(str[start] == '(');
			uint32_t curr = start;
			int count = 0;
			while (curr < str.length()) {
				if (str[curr] == '(')
					count++;
				else if (str[curr] == ')')
					count--;
				if (count <= 0) return curr;
				curr++;
			}
			return str.npos;
		}

		std::string TranslationGenerator::inline_macro(std::string source, std::string macro, std::vector<std::string> args, bool trim_args) const
		{
			std::ostringstream out;

			size_t pos = 0;
			std::string macroStr = macro + "(";
			while ((pos = source.find(macroStr)) != source.npos) {
				// int end = Util::Match(source, pos, '(', ')');
				int end = get_matching_bracket(source, pos + macroStr.length() - 1);

				// first, read macro args

				int argStart = pos + macro.length() + 1;
				std::string macroArgString = source.substr(argStart, end - argStart);
				std::vector<std::string> macro_args = util::Util::Tokenize(macroArgString, ",", true);

				out << source.substr(0, pos);

				int argPos = 0;
				for (std::vector<std::string>::iterator i = args.begin(); i != args.end(); ++i) {
					if ((*i)[0] == '%') {
						uint32_t idx = atoi(&(*i)[1]);
						if (idx >= macro_args.size()) {
							printf("Not enough arguments provided to macro %s\n", macro.c_str());
							exit(1);
						}
						std::string arg = macro_args[idx];
						if (trim_args) arg = util::Util::trim_whitespace(arg);
						out << arg;
					} else
						out << *i;
				}
				source = source.substr(end + 1, source.npos);
			}
			out << source;
			return out.str();
		}

		std::string TranslationGenerator::inline_macro(std::string source, std::string macro, std::string output, bool trim_whitespace) const
		{
			return inline_macro(source, macro, util::Util::Tokenize(output, " ()[]\\\",>._", true, false), trim_whitespace);
		}

		std::string TranslationGenerator::do_macros(const std::string code, bool trace) const
		{
			std::string result = (code);
			bool changed;

			if (trace) {

				// register operations
				result = inline_macro(result, "read_register", "read_register_%0(regs)", true);
				result = inline_macro(result, "write_register", "write_register_%0(regs, %1)", true);

				result = inline_macro(result, "read_register_bank", "read_register_bank_%0(regs, %1)", true);
				result = inline_macro(result, "write_register_bank", "read_register_bank_%0(regs, %1, %2)", true);

//		result = inline_macro(result, "trace_rb_read", "cpuTraceRegBankRead(s->cpu_ctx, TRACE_ID_%1, %2, %0)", true);
//		result = inline_macro(result, "trace_rb_write", "cpuTraceRegBankWrite(s->cpu_ctx, TRACE_ID_%1, %2, %0)", true);

				// memory operations
				result = inline_macro(result, "mem_read_8", "(cpuTraceMemRead8(s->cpu_ctx, %0, & %1))", false);
				result = inline_macro(result, "mem_read_8s", "(cpuTraceMemRead8s(s->cpu_ctx, %0, & %1))", false);

				result = inline_macro(result, "mem_read_16", "(cpuTraceMemRead16(s->cpu_ctx, %0, & %1))", false);
				result = inline_macro(result, "mem_read_16s", "(cpuTraceMemRead16s(s->cpu_ctx, %0, & %1))", false);

				result = inline_macro(result, "mem_read_32", "(cpuTraceMemRead32(s->cpu_ctx, %0, & %1))", false);

				result = inline_macro(result, "mem_write_8", "(cpuTraceMemWrite8(s->cpu_ctx, %0, %1))", false);
				result = inline_macro(result, "mem_write_16", "(cpuTraceMemWrite16(s->cpu_ctx, %0, %1))", false);
				result = inline_macro(result, "mem_write_32", "(cpuTraceMemWrite32(s->cpu_ctx, %0, %1))", false);

				result = inline_macro(result, "trace_string", "(cpuTraceString(s->cpu_ctx, %0, 0))", false);
			} else {
				// register operations
				result = inline_macro(result, "read_register", "read_register_%0(regs)", true);
				result = inline_macro(result, "write_register", "write_register_%0(regs, %1)", true);

				result = inline_macro(result, "read_register_bank", "read_register_bank_%0(regs, %1)", true);
				result = inline_macro(result, "write_register_bank", "write_register_bank_%0(regs, %1, %2)", true);

				result = inline_macro(result, "trace_rb_read", "", false);
				result = inline_macro(result, "trace_rb_write", "", false);

				// memory operations


				result = inline_macro(result, "trace_string", "", false);
			}


			result = inline_macro(result, "mem_read_8", "fn_mem_read_8((gensim::Processor*)cpu->cpu_ctx, %0, & %1)", false);
			result = inline_macro(result, "mem_read_16", "fn_mem_read_16((gensim::Processor*)cpu->cpu_ctx, %0, & %1)", false);
			result = inline_macro(result, "mem_read_32", "fn_mem_read_32((gensim::Processor*)cpu->cpu_ctx, %0, & %1)", false);

			result = inline_macro(result, "mem_write_8", "fn_mem_write_8((gensim::Processor*)cpu->cpu_ctx, %0, %1)", false);
			result = inline_macro(result, "mem_write_16", "fn_mem_write_16((gensim::Processor*)cpu->cpu_ctx, %0, %1)", false);
			result = inline_macro(result, "mem_write_32", "fn_mem_write_32((gensim::Processor*)cpu->cpu_ctx, %0, %1)", false);

			result = inline_macro(result, "mem_write_8_user", "0", false);
			result = inline_macro(result, "mem_write_32_user", "0", false);
			result = inline_macro(result, "mem_read_8_user", "0", false);
			result = inline_macro(result, "mem_read_32_user", "0", false);
			result = inline_macro(result, "take_exception", "cpuTakeException((gensim::Processor*)cpu->cpu_ctx, %0, %1)", false);

			result = inline_macro(result, "emulate_syscall", "cpuEmulateTrap(s->cpu_ctx, %0, %1, %2, %3, %4, %5, %6)", false);

//	result = inline_macro(result, "registers", "regs", false);

			result = inline_macro(result, "read_pc", "__pc", false);

//	result = inline_macro(result, "halt_cpu", "cpuHalt(s->cpu_ctx)", false);
			result = inline_macro(result, "trap", "asm(\"int3\")", false);

			result = inline_macro(result, "get_cpu_mode", "0", false);
			result = inline_macro(result, "push_interrupt", "0", false);
			result = inline_macro(result, "pop_interrupt", "0", false);
			result = inline_macro(result, "pend_interrupt", "0", false);
			result = inline_macro(result, "enter_kernel_mode", "0", false);
			result = inline_macro(result, "enter_user_mode", "0", false);
			result = inline_macro(result, "set_cpu_mode", "0", false);
			result = inline_macro(result, "halt_cpu", "0", false);

			result = inline_macro(result, "flush_itlb_entry", "0", false);
			result = inline_macro(result, "flush_dtlb_entry", "0", false);

			result = inline_macro(result, "write_device", "devWriteDevice((gensim::Processor*)cpu->cpu_ctx, %0, %1, %2)", false);
			result = inline_macro(result, "read_device", "devReadDevice((gensim::Processor*)cpu->cpu_ctx, %0, %1, & %2)", false);

//	result = inline_macro(result, "read_register_nt", "(regs-> %0 )", false);
//	result = inline_macro(result, "write_register_nt", "(regs-> %0 ) = ( %1 )", false);
//
//	result = inline_macro(result, "read_register_bank_nt", "(regs-> %0 [ %1 ])", false);
//	result = inline_macro(result, "write_register_bank_nt", "regs-> %0 [ %1 ] = ( %2 )", false);

			while(result.find("inst.") != result.npos) {
				result = result.replace(result.find("inst."), 5, "__insn_");
			}

			return result;
		}

		void TranslationGenerator::emit_behaviour_fn(std::ofstream &str, const isa::ISADescription &isa, const std::string name, const std::string behaviour, bool trace, std::string ret_type) const
		{
//#define NOTRACE
#ifdef NOTRACE
			if (trace) return;
#endif
			str << " inline " << ret_type << " " << isa.ISAName << "_" << name;
			if (trace) str << "_trace";
			str << "(struct cpuState * const s, struct gensim_state * const regs, uint32_t __PC ";

			const std::map<std::string, std::string> fields = isa.Get_Disasm_Fields();
			for (std::map<std::string, std::string>::const_iterator i = fields.begin(); i != fields.end(); ++i) {
#ifdef NOLIMM
				if (i->first == "LimmPtr") {
					continue;
				}
#endif
				str << ", " << i->second << " _" << i->first;
			}

			str << ") {\n";
			str << util::Util::FindReplace(do_macros(behaviour, trace), "inst.", "_");

			str << "\n}\n";
		}

		std::string TranslationGenerator::format_code(const std::string code, const std::string inst_ref, const bool trace) const
		{
			std::string result = util::Util::clean(do_macros(code, trace));

			std::list<EmissionChunk> chunks = EmissionChunk::Parse(result);

			for (std::list<EmissionChunk>::iterator chunk = chunks.begin(); chunk != chunks.end(); ++chunk) {
				if (chunk->Quoted) {
					// chunk->Text = inline_inst(chunk->Text, inst_ref);
				}
			}

			result = EmissionChunk::Format(chunks);

			return result;
		}

		bool TranslationGenerator::Generate() const
		{
			bool success = true;
			const arch::ArchDescription &arch = Manager.GetArch();

			std::string headerStr = Manager.GetTarget();
			headerStr.append("/translate.h");

			std::string sourceStr = Manager.GetTarget();
			sourceStr.append("/translate.cpp");

			util::cppformatstream header;
			util::cppformatstream source;

			// Process of converting interpretive behaviour into JIT emitted string:
			// 1. Tokenize the behaviour string on ()s, *s, &s, and whitespace
			// 2. Directly emit tokens not related to the instruction
			// 3. Replace tokens relating to instruction with references to the instruction itself
			// 4. Emit casts for the instruction field references to the type of that field (important for pointers)
			/*
			 * e.g. (*inst.dst) = (*inst.src1) + (*inst.src2)
			 *
			 * becomes
			 *
			 * stream << "(*" << "(uint32*)" << inst.dst << ") = (*" << "(uint32*)" << inst.src1 << ") + (*" << "(uint32*)" << inst.src2 << ")";
			 *
			 */

			GenerateHeader(header);

			source << "#include <sstream>\n"
			       "#include <iomanip>\n"
			       "#include \"api.h\"\n"
			       "#include \"decode.h\"\n"
			       "#include \"translate.h\"\n"
			       "#include \"processor.h\"\n";

			source << "namespace gensim { namespace " << arch.Name << " { \n";

			GenerateInstructionTranslations(source);

			GenerateJumpInfo(source);

			source << "}}\n";

			std::ofstream header_file(headerStr.c_str());
			header_file << header.str();
			header_file.close();

			std::ofstream source_file(sourceStr.c_str());
			source_file << source.str();
			source_file.close();

			return success;
		}

		bool TranslationGenerator::GenerateHeader(std::ostringstream &target) const
		{
			target << "#ifndef __HEADER_TRANSLATE_" << Manager.GetArch().Name << "\n"
			       "#define __HEADER_TRANSLATE_" << Manager.GetArch().Name << "\n"
			       "#include <gensim/gensim_translate.h>\n"
			       "#include <string>\n"
			       "namespace gensim { "
			       "class Processor;\n "
			       "class BaseDecode;\n"
			       "namespace " << Manager.GetArch().Name << "{ \n"
			       "class JumpInfo : public gensim::BaseJumpInfo {"
			       "public:"
			       "void GetJumpInfo(const gensim::BaseDecode *instr, uint32_t pc, bool &indirect_jump, bool &direct_jump, uint32_t &jump_target) override;\n"
			       "};"
			       "class Translate : public gensim::BaseLLVMTranslate{"
			       "public: "
			       "Translate(const gensim::Processor &, archsim::translate::llvm::LLVMTranslationContext&);"

			       "virtual uint8_t *GetPrecompBitcode() override;"
			       "virtual uint32_t GetPrecompSize() override;"

			       "virtual bool EmitPredicate(archsim::translate::llvm::LLVMInstructionTranslationContext& ctx, ::llvm::Value*& __result, bool trace) override;"
			       "virtual bool TranslateInstruction(archsim::translate::llvm::LLVMInstructionTranslationContext&, llvm::Value*&, bool);"

			       "void GetJumpInfo(const gensim::BaseDecode *instr, uint32_t pc, bool &indirect_jump, bool &direct_jump, uint32_t &jump_target)  override;\n"
			       "};"
			       "}}\n"
			       "#endif";
			return true;
		}

		bool TranslationGenerator::GenerateJumpInfo(std::ostringstream &target) const
		{
			return true;
		}

		bool TranslationGenerator::GenerateInstructionTranslations(std::ostringstream &target) const
		{
			const arch::ArchDescription &arch = Manager.GetArch();
			const isa::ISADescription *static_isa = arch.ISAs.front();

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				// output behaviour code
				// loop through behaviours
				for (std::map<std::string, std::string *>::const_iterator i = isa->ExecuteActions.begin(); i != isa->ExecuteActions.end(); ++i) {
					if (i->second == NULL) {
						fprintf(stderr, "Could not find behaviour %s\n", i->first.c_str());
						target << "inline void behaviour_" << isa->ISAName << "_" << i->first << "(const Decode &inst, std::stringstream &str) {}\n";
						target << "inline void trace_behaviour_" << isa->ISAName << "_" << i->first << "(const Decode &inst, std::stringstream &str) {}\n";
					} else {
						target << "inline void behaviour_" << isa->ISAName << "_" << i->first << "(const Decode &inst, std::stringstream &str) {str << " << format_code(*(i->second), "inst.", false) << ";}\n";
						target << "inline void trace_behaviour_" << isa->ISAName << "_" << i->first << "(const Decode &inst, std::stringstream &str) {str << " << format_code(*(i->second), "inst.", true) << ";}\n";
					}
				}
			}

			target << "std::string Translate::GetStateMacros()\n {\n";
			InterpretiveExecutionEngineGenerator *interp = (InterpretiveExecutionEngineGenerator *)Manager.GetComponentC(GenerationManager::FnInterpret);
			target << "std::ostringstream s;\n";
			target << "s << \"" << util::Util::clean(interp->GetStateStruct()) << "\";\n";
			target << "return s.str();\n";
			target << "}\n\n";

			target << "std::string Translate::TranslateInstruction(const gensim::BaseDecode *instr, const gensim::Processor &processor, bool trace)\n {\n";
			target << "std::stringstream stream;";
			target << "const gensim::" << Manager.GetArch().Name << "::Processor &cpu = static_cast<const gensim::" << Manager.GetArch().Name << "::Processor &>(processor);\n";
			target << "Decode &inst = *((Decode*)instr);\n";

			if (static_isa->HasBehaviourAction("instruction_begin")) {
				target << "if(trace){";
				target << "stream << " << format_code(static_isa->GetBehaviourAction("instruction_begin"), "inst.", true) << ";\n";
				target << "}else {";
				target << "stream << " << format_code(static_isa->GetBehaviourAction("instruction_begin"), "inst.", false) << ";}\n";
			}

			target << "switch(inst.Instr_Code) {";

			for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = static_isa->Instructions.begin(); i != static_isa->Instructions.end(); ++i) {
				target << "case INST_" << i->first << ":";
				target << "if(trace) trace_behaviour_" << i->second->BehaviourName << "(inst, stream);\n";
				target << "else behaviour_" << i->second->BehaviourName << "(inst, stream);\n";
				target << "break;\n";
			}

			target << "}";

			if (static_isa->HasBehaviourAction("instruction_end")) {
				target << "if(trace){";
				target << "stream << " << format_code(static_isa->GetBehaviourAction("instruction_end"), "inst.", true) << ";\n }";
				target << "else {";
				target << "stream << " << format_code(static_isa->GetBehaviourAction("instruction_end"), "inst.", false) << ";}\n;\n";
			}

			target << "return stream.str();\n";

			target << "}\n";

			return true;
		}

		const std::vector<std::string> TranslationGenerator::GetSources() const
		{
			std::vector<std::string> sources;
			sources.push_back("translate.cpp");
			return sources;
		}

	}  // namespace generator
}  // namespace gensim
