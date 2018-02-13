/*
 * File:   DecodeGenerator.cpp
 * Author: mgould1
 *
 * Created on 04 May 2012, 17:30
 */

#include "generators/DecodeGenerator.h"
#include "DecodeTree.h"
#include "Util.h"

DEFINE_DUMMY_COMPONENT(gensim::generator::DecodeGenerator, base_decode)
COMPONENT_OPTION(base_decode, class, "Decode", "The name to use for the class")
COMPONENT_OPTION(base_decode, InlineHints, "0", "Emit inline hints in front of decode behaviours - larger code size but possibly faster decode")
COMPONENT_OPTION(base_decode, OptimisationEnabled, "1", "Optimise the decode tree")
COMPONENT_OPTION(base_decode, OptimisationMinPrefixLength, "2", "The minimum prefix length a node requires to be split")
COMPONENT_OPTION(base_decode, OptimisationMinPrefixMembers, "4", "The minimum number of children of a node before it can be split")
COMPONENT_OPTION(base_decode, Debug, "0", "Causes the debug tree to emit tracing information as it is traversed.")
COMPONENT_OPTION(base_decode, GenerateDotGraph, "0", "Generate a Graphviz (dot) format graph of the optimised decode tree. This can be used in combination with the Debug option to debug instruction decoding.")

namespace gensim
{
	namespace generator
	{

		DecodeGenerator::~DecodeGenerator()
		{
			for (std::map<const isa::ISADescription *, DecodeNode *>::const_iterator DI = decode_trees.begin(), DE = decode_trees.end(); DI != DE; ++DI) {
				delete DI->second;
			}
		}

		const std::vector<std::string> DecodeGenerator::GetSources() const
		{
			std::vector<std::string> sources;
			sources.push_back(GetDecodeSourceFilename());
			return sources;
		}

		void DecodeGenerator::DrawDotNode(const DecodeNode *tree, std::stringstream &target) const
		{
			if (tree->target == NULL)
				target << "\tnode_" << tree->node_number << ";\n";
			else
				target << "\tnode_" << tree->node_number << " [penwidth=2, label=" << tree->target->Name << "];\n";
			for (std::multimap<uint8_t, DecodeTransition>::const_iterator i = tree->transitions.begin(); i != tree->transitions.end(); ++i) {
				DrawDotNode(i->second.target, target);
				target << "\tnode_" << tree->node_number << " -> node_" << i->second.target->node_number << " [label=" << util::Util::FormatBinary(i->second.value, i->second.length) << "];\n";
			}
			if (tree->unconstrained_transition) {
				DrawDotNode(tree->unconstrained_transition->target, target);
				if (tree->unconstrained_transition->length == 0)
					target << "\tnode_" << tree->node_number << " -> node_" << tree->unconstrained_transition->target->node_number << " [label=\"(x)\"]\n";
				else
					target << "\tnode_" << tree->node_number << " -> node_" << tree->unconstrained_transition->target->node_number << " [label=" << util::Util::StrDup("x", tree->unconstrained_transition->length) << "];\n";
			}
		}

		bool DecodeGenerator::GenerateDecodeDiagram(std::stringstream &stream) const
		{
			stream << "digraph Decode {\n";
			DrawDotNode(this->decode_trees.begin()->second, stream);
			stream << "}\n";
			return true;  // always succeeds; for consistency with other Generate*s.
		}

		DecodeNode *DecodeGenerator::BuildDecodeTree(GenerationSetupManager &setup, const isa::ISADescription &isa)
		{
			DecodeNode *decode_tree;

			// Build decode tree
			fprintf(stderr, "[DECODE] Building decode tree for ISA %s...", isa.ISAName.c_str());

			isa::ISADescription::InstructionDescriptionMap::const_iterator ins = isa.Instructions.begin();
			decode_tree = DecodeNode::CreateTree(*ins->second);

			ins++;
			while (ins != isa.Instructions.end()) {
				decode_tree->AddInstruction(*ins->second);
				ins++;
				specialisations[ins->second] = std::vector<uint32_t>();
			}

// look for specialisations and add them to the tree if they exist
#if 0
			SpecialisedInterpreter *interp = dynamic_cast<SpecialisedInterpreter*>(setup.GetComponent(GenerationManager::FnInterpret));
			if(interp) {
				if(interp->GetProperty("SpecialisedInstructions") != "") {
					fprintf(stderr, "[DECODE] Loading instruction specialisations...");
					std::ifstream spec_file (interp->GetProperty("SpecialisedInstructions").c_str(), std::ifstream::in);
					char buffer[16];
					SpecListType insn_specs;
					while(spec_file.good()) {
						spec_file.getline(buffer, 16);
						insn_specs.push_back(strtoul(buffer, NULL, 16));
					}

					fprintf(stderr, "done\n");

					fprintf(stderr, "[DECODE] Setting up specialised instructions...");
					for(SpecListType::const_iterator spec_i = insn_specs.begin(); spec_i != insn_specs.end(); ++spec_i) {
						DecodedInstruction *di = decode_tree->Decode(*spec_i);
						specialisations[di->insn].push_back(*spec_i);
						std::stringstream name_str;
						name_str << "spec_" << *spec_i;
						isa::InstructionDescription *id = new isa::InstructionDescription (name_str.str(), isa, di->insn->Format);
						id->BehaviourName = decode_tree->Decode(*spec_i)->insn->BehaviourName;

						std::vector<isa::InstructionDescription::DecodeConstraint> constraint_set;

						for(DecodedInstruction::ChunkMap::const_iterator chunk_ci = di->chunks.begin(); chunk_ci != di->chunks.end(); ++chunk_ci) {
							constraint_set.push_back(isa::InstructionDescription::DecodeConstraint(chunk_ci->first, isa::InstructionDescription::Constraint_Equals, chunk_ci->second));
						}

						id->Decode_Constraints.push_back(constraint_set);

						decode_tree->AddInstruction(*id);
					}

				}
			}
#endif

			fprintf(stderr, "done - %u nodes\n", decode_tree->CountNodes());
			if (GetProperty("OptimisationEnabled") == "1") {
				fprintf(stderr, "[DECODE] Optimising tree...");
				decode_tree->Optimize(atoi(GetProperty("OptimisationMinPrefixLength").c_str()), atoi(GetProperty("OptimisationMinPrefixMembers").c_str()));
				fprintf(stderr, "done - %u nodes\n", decode_tree->CountNodes());
			}

			return decode_tree;
		}

		void DecodeGenerator::Setup(GenerationSetupManager &Setup)
		{
			const arch::ArchDescription &arch = Manager.GetArch();

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				decode_trees[isa] = BuildDecodeTree(Setup, *isa);
			}
		}

		bool DecodeGenerator::GenerateDecodeEnum(util::cppformatstream &str) const
		{
			const arch::ArchDescription &Architecture = Manager.GetArch();

			for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;
				str << "#define ISA_MODE_" << isa->ISAName << " " << isa->isa_mode_id << "\n";
			}

			// generate enum for instruction types
			str << "enum " << GetProperty("class") << "_Enum \n{\n";
			for (SpecMapType::const_iterator ci = specialisations.begin(); ci != specialisations.end(); ++ci) {
				for (SpecListType::const_iterator sl_ci = ci->second.begin(); sl_ci != ci->second.end(); ++sl_ci) {
					str << "\tINST_spec_" << *sl_ci << ",\n";
				}
			}

			for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = isa->Instructions.begin(); i != isa->Instructions.end(); ++i) {
					str << "\tINST_" << isa->ISAName << "_" << i->second->Name << ",\n";
				}
			}

			str << "\t__INST_CODE_INVALID__ = -1\n";

			str << "};\n\n";  // enum

			return true;
		}


		bool DecodeGenerator::EmitInstructionFields(util::cppformatstream &stream) const
		{
			const arch::ArchDescription &Architecture = Manager.GetArch();
			std::map<std::string, std::string> field_map;

			// header_str << "//User-defined fields\n";

			for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				for (std::list<isa::FieldDescription>::const_iterator i = isa->UserFields.begin(); i != isa->UserFields.end(); ++i) {
					// header_str << "\t " << i->field_type << " " << i->field_name << ";\n";
					if (i->field_type.find("int", 0) != i->field_type.npos)
						field_map[i->field_name] = i->field_type + "_t";
					else
						field_map[i->field_name] = i->field_type;
				}
			}

			for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				std::map<std::string, uint32_t> fields = isa->Get_Decode_Fields();

				std::set<std::string> emitted_field_names;
				for (auto Format : isa->Formats) {
					uint32_t pos = 31;
					for (auto Chunk : Format.second->GetChunks()) {

						if (!Chunk.generate_field || Chunk.is_constrained) {
							pos -= Chunk.length;
							continue;
						}

						if (!emitted_field_names.insert(Chunk.Name).second) {
							pos -= Chunk.length;
							continue;
						}
						field_map[Chunk.Name] = util::Util::TypeString(fields[Chunk.Name]);

						pos -= Chunk.length;
					}
				}
			}

			for (std::map<std::string, std::string>::iterator FI = field_map.begin(), FE = field_map.end(); FI != FE; ++FI) {
				stream << "\t " << FI->second << " " << FI->first << ";\n";
			}
			return true;
		}


		bool DecodeGenerator::Generate() const
		{
			bool success = true;
			util::cppformatstream header_str;
			util::cppformatstream source_str;
			std::stringstream diagram_str;

			// generate the source streams
			success &= GenerateDecodeHeader(header_str);
			success &= GenerateDecodeSource(source_str);
			if (GetProperty("GenerateDotGraph") == "1") {
				success &= GenerateDecodeDiagram(diagram_str);
			}

			if (success) {
				WriteOutputFile(GetDecodeHeaderFilename(), header_str);
				WriteOutputFile(GetDecodeSourceFilename(), source_str);
				WriteOutputFile("decode.dot", diagram_str);
			}
			return success;
		}

	}  // namespace generator
}  // namespace gensim
