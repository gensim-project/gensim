/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescriptionParser.h"
#include "isa/ISADescriptionParser.h"
#include "genC/ssa/SSAContext.h"
#include "genC/Parser.h"
#include "genC/ir/IRConstant.h"

#include "flexbison_archc_ast.h"
#include "flexbison_archc.h"

#include <string.h>
#include <fstream>

using namespace gensim;
using namespace gensim::arch;

ArchDescriptionParser::ArchDescriptionParser(DiagnosticContext &diag_ctx) : diag_ctx(diag_ctx), arch(new ArchDescription())
{

}

bool ArchDescriptionParser::ParseFile(std::string filename)
{
	std::ifstream file(filename);

	ArchC::ArchCScanner scanner(&file);

	ArchC::AstNode root(ArchCNodeType::ROOT);
	ArchC::ArchCParser parser(scanner, &root);

	int result = parser.parse();

	if(result != 0) {
		diag_ctx.Error("Failed to parse.", DiagNode(filename));
		return false;
	}

	auto &arch_node = root[0];
	GASSERT(arch_node.GetType() == ArchCNodeType::Arch);

	if(!load_from_arch_node(arch_node)) {
		return false;
	}

	return true;
}

ArchDescription *ArchDescriptionParser::Get()
{
	return arch;
}

bool ArchDescriptionParser::load_from_arch_node(ArchC::AstNode & node)
{
	auto &nameNode = node[0][0];
	arch->Name = nameNode.GetString();

	auto &listNode = node[1];
	for (auto childNode : listNode) {
		switch (childNode->GetType()) {
			case ArchCNodeType::RegSpace:
				if(!load_regspace(*childNode)) return false;
				break;

			case ArchCNodeType::WordSize: {
				arch->wordsize = (*childNode)[0].GetInt();
				break;
			}
			case ArchCNodeType::Features: {
				if(!load_feature_set(*childNode)) return false;
				break;
			}
			case ArchCNodeType::MemInterface: {
				if(!load_mem(*childNode)) return false;
				break;
			}
			default: {
				assert("Unknown node type!" && false);
				return false;
			}
		}
	}

	auto &ctorNode = node[2];
	if(!load_arch_ctor(ctorNode)) return false;

	if(!arch->GetRegFile().Resolve(diag_ctx)) return false;
	if(!arch->GetMemoryInterfaces().Resolve(diag_ctx)) return false;

	return true;
}


RegBankViewDescriptor *load_reg_view_bank(ArchC::AstNode & node, RegSpaceDescriptor *space)
{
	assert(node.GetChildren().size() == 8);

	auto &id_node = node[0];
	auto &type_node = node[1];
	auto &offset_node = node[2];
	auto &rcount_node = node[3];
	auto &rstride_node = node[4];
	auto &ecount_node = node[5];
	auto &esize_node = node[6];
	auto &estride_node = node[7];

	assert(id_node.GetType() == ArchCNodeType::Identifier);
	assert(type_node.GetType() == ArchCNodeType::Identifier);
	assert(offset_node.GetType() == ArchCNodeType::INT);
	assert(rcount_node.GetType() == ArchCNodeType::INT);
	assert(rstride_node.GetType() == ArchCNodeType::INT);
	assert(ecount_node.GetType() == ArchCNodeType::INT);
	assert(esize_node.GetType() == ArchCNodeType::INT);
	assert(estride_node.GetType() == ArchCNodeType::INT);

	std::string id = id_node[0].GetString(); // Dereference identifier node
	std::string type = type_node[0].GetString();  // Dereference identifier node
	uint32_t offset = offset_node.GetInt();
	uint32_t rcount = rcount_node.GetInt();
	uint32_t rstride = rstride_node.GetInt();
	uint32_t ecount = ecount_node.GetInt();
	uint32_t esize = esize_node.GetInt();
	uint32_t estride = estride_node.GetInt();

	return new RegBankViewDescriptor(space, id, offset, rcount, rstride, ecount, esize, estride, type);
}

RegSlotViewDescriptor *load_reg_view_slot(ArchC::AstNode & node, RegSpaceDescriptor *space)
{
	assert(node.GetChildren().size() == 4 || node.GetChildren().size() == 5);

	auto &id_node = node[0];
	auto &type_node = node[1];
	auto &width_node = node[2];
	auto &offset_node = node[3];

	assert(id_node.GetType() == ArchCNodeType::Identifier);
	assert(type_node.GetType() == ArchCNodeType::Identifier);
	assert(width_node.GetType() == ArchCNodeType::INT);
	assert(offset_node.GetType() == ArchCNodeType::INT);


	std::string id = id_node[0].GetString();
	std::string type = type_node[0].GetString();
	uint32_t width = width_node.GetInt();
	uint32_t offset = offset_node.GetInt();

	if(node.GetChildren().size() == 5) {
		auto &tag_node = node[4];

		std::string tag = tag_node[0].GetString();
		return new RegSlotViewDescriptor(space, id, type, width, offset, tag);
	} else {
		return new RegSlotViewDescriptor(space, id, type, width, offset);
	}
}

// generate a register space from a declaration
bool ArchDescriptionParser::load_regspace(ArchC::AstNode & childNode)
{
	auto &sizeNode = childNode[0];
	uint32_t space_size = sizeNode.GetInt();

	RegSpaceDescriptor *regspace = new RegSpaceDescriptor(&arch->GetRegFile(), space_size);

	arch->GetRegFile().AddRegisterSpace(regspace);

	auto &listNode = childNode[1];
	for(auto viewNodePtr : listNode) {
		auto &viewNode = *viewNodePtr;
		switch(viewNode.GetType()) {
			case ArchCNodeType::RegViewBank: {
				RegBankViewDescriptor *descriptor = load_reg_view_bank(viewNode, regspace);
				if(!descriptor)
					return false;

				if(!arch->GetRegFile().AddRegisterBank(regspace, descriptor))
					return false;

				break;
			}
			case ArchCNodeType::RegViewSlot: {
				RegSlotViewDescriptor *descriptor = load_reg_view_slot(viewNode, regspace);
				if(!descriptor)
					return false;

				if(!arch->GetRegFile().AddRegisterSlot(regspace, descriptor))
					return false;

				break;
			}
			default: {
				UNEXPECTED;
			}
		}
	}

	return true;
}

bool ArchDescriptionParser::load_feature_set(ArchC::AstNode & featuresNode)
{
	auto &listNode = featuresNode[0];
	for(auto featureNodePtr : listNode) {
		auto &featureNode = *featureNodePtr;
		// featureNode is an identifier

		std::string featureName = featureNode[0].GetString();
		if(!arch->GetFeatures().AddFeature(featureName)) {
			diag_ctx.Error("Could not add feature", DiagNode("", featureNode.GetLocation()));
			return false;
		}
	}

	return true;
}

bool ArchDescriptionParser::load_mem(ArchC::AstNode & memNode)
{
	auto &nameNode = memNode[0];
	auto &addrNode = memNode[1];
	auto &wordNode = memNode[2];
	auto &endianNode = memNode[3];
	auto &fetchNode = memNode[4];

	std::string name = nameNode[0].GetString();
	uint64_t addr = addrNode.GetInt();
	uint64_t word = wordNode.GetInt();
	bool big_endian = false;

	std::string endian_text = endianNode[0].GetString();
	if(endian_text == "little") {
		big_endian = false;
	} else if(endian_text == "big") {
		big_endian = true;
	} else {
		diag_ctx.Error("Unknown endianness: " + endian_text);
		return false;
	}

	bool is_fetch = fetchNode.GetInt();

	MemoryInterfaceDescription mid (name, addr, word, big_endian, arch->GetMemoryInterfaces().GetInterfaces().size());
	arch->GetMemoryInterfaces().AddInterface(mid);

	if(is_fetch) {
		arch->GetMemoryInterfaces().SetFetchInterface(name);
	}

	return true;
}


// walk the arch_ctor section

bool ArchDescriptionParser::load_arch_ctor(ArchC::AstNode & ctorNode)
{
	int next_isa_id = 0;
	bool success = true;

	auto &idNode = ctorNode[0]; // do nothing with this for now
	auto &listNode = ctorNode[1];

	// walk the ARCH_CTOR subtree
	for (auto archNodePtr : listNode) {
		auto &archNode = *archNodePtr;

		switch (archNode.GetType()) {

			case ArchCNodeType::AcIsa: {
				auto &filenameNode = archNode[0];
				std::string filename = filenameNode.GetString();
				filename = filename.substr(1, filename.length() - 2); // trim quotation marks

				std::ifstream test_file_exists_stream (filename.c_str());
				if(!test_file_exists_stream.good()) {
					fprintf(stderr, "Could not load ISA file %s\n", filename.c_str());
					success = false;
				} else {
					isa::ISADescriptionParser parser (diag_ctx, arch->ISAs.size());
					if(parser.ParseFile(filename.c_str()))
						arch->ISAs.push_back(parser.Get());
					else
						success = false;
				}
				break;
			}

			case ArchCNodeType::SetFeature: {
				auto &idNode = archNode[0][0];
				auto &valueNode = archNode[1];

				uint32_t level = valueNode.GetInt();;
				auto *feature = arch->GetFeatures().GetFeature(idNode.GetString());
				if(!feature) {
					diag_ctx.Error("Could not find feature", DiagNode("", archNode.GetLocation()));
					success = false;
				} else {
					feature->SetDefaultLevel(level);
				}

				break;
			}

			case ArchCNodeType::AcTypename: {
				auto &nameNode = archNode[0][0];
				auto &typeNode = archNode[1][0];

				std::string name = nameNode.GetString();
				std::string type = typeNode.GetString();

				if(arch->GetTypenames().count(name)) {
					diag_ctx.Error("Duplicate type " + name);
					success = false;
				} else {
					arch->GetTypenames()[name] = type;
				}

				break;
			}

			case ArchCNodeType::AcConstant: {
				auto &nameNode = archNode[0][0];
				auto &typeNode = archNode[1][0];
				auto &valueNode = archNode[2];

				std::string name = nameNode.GetString();
				std::string type = typeNode.GetString();

				uint64_t value = valueNode.GetInt();

				if(arch->GetConstants().count(name)) {
					diag_ctx.Error("Duplicate constant name " + name);
					success = false;
				} else {
					arch->GetConstants()[name] = std::make_pair(type, std::to_string(value));
				}

				break;
			}

			case ArchCNodeType::AcEndianness: {
				// do nothing
				break;
			}

			default: {
				UNEXPECTED;
			}
		}
	}
	return success;

}

