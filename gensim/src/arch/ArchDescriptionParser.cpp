/*
 * ArchDescriptionParser.cpp
 *
 *  Created on: 18 May 2015
 *      Author: harry
 */

#include "arch/ArchDescriptionParser.h"
#include "isa/ISADescriptionParser.h"
#include "genC/ssa/SSAContext.h"
#include "genC/Parser.h"

#include <archc/archcLexer.h>
#include <archc/archcParser.h>

#include <antlr3.h>

#include "antlr-ver.h"

#include <string.h>
#include <fstream>

using namespace gensim;
using namespace gensim::arch;

ArchDescriptionParser::ArchDescriptionParser(DiagnosticContext &diag_ctx) : diag_ctx(diag_ctx), arch(new ArchDescription())
{

}

bool ArchDescriptionParser::ParseFile(std::string filename)
{
	pANTLR3_UINT8 fname_str = (pANTLR3_UINT8)filename.c_str();
	pANTLR3_INPUT_STREAM pts = antlr3AsciiFileStreamNew(fname_str);
	if (!pts) {
		diag_ctx.Error(Format("Couldn't open architecture file '%s': %s.", filename.c_str(), strerror(errno)), DiagNode(filename));
		return false;
	}
	parchcLexer lexer = archcLexerNew(pts);
	pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
	parchcParser parser = archcParserNew(tstream);

	archcParser_arch_return arch_r = parser->arch(parser);

	if (parser->pParser->rec->getNumberOfSyntaxErrors(parser->pParser->rec) > 0 || lexer->pLexer->rec->getNumberOfSyntaxErrors(lexer->pLexer->rec)) {
		return false;
	}

	pANTLR3_COMMON_TREE_NODE_STREAM nodes = antlr3CommonTreeNodeStreamNewTree(arch_r.tree, ANTLR3_SIZE_HINT);

	if(!load_from_arch_node(nodes->root)) {
		return false;
	}

	nodes->free(nodes);
	parser->free(parser);
	tstream->free(tstream);
	lexer->free(lexer);
	pts->free(pts);

	return true;
}

ArchDescription *ArchDescriptionParser::Get()
{
	return arch;
}

bool ArchDescriptionParser::load_from_arch_node(pANTLR3_BASE_TREE node)
{
	pANTLR3_BASE_TREE NameNode = (pANTLR3_BASE_TREE)node->children->get(node->children, 0);
	arch->Name = (char *)(NameNode->getText(NameNode)->chars);

	for (uint32_t i = 1; i < node->getChildCount(node); i++) {
		pANTLR3_BASE_TREE childNode = (pANTLR3_BASE_TREE)node->children->get(node->children, i);
		switch (childNode->getType(childNode)) {

			case AC_REGSPACE:
				if(!load_regspace(childNode)) return false;
				break;

			case AC_WORDSIZE: {
				pANTLR3_BASE_TREE sizeNode = (pANTLR3_BASE_TREE)childNode->getChild(childNode, 0);
				arch->wordsize = atoi((char *)(sizeNode->getText(sizeNode)->chars));
				break;
			}
			case ARCH_CTOR: {
				if(!load_arch_ctor(childNode)) return false;
				break;
			}
			case AC_FEATURES: {
				if(!load_feature_set(childNode)) return false;
				break;
			}
			default: {
				assert("Unknown node type!" && false);
				return false;
			}
		}
	}

	if(!arch->GetRegFile().Resolve(diag_ctx)) return false;

	return true;
}


RegBankViewDescriptor *load_reg_view_bank(pANTLR3_BASE_TREE node, RegSpaceDescriptor *space)
{
	assert(node->getChildCount(node) == 8);

	pANTLR3_BASE_TREE id_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
	pANTLR3_BASE_TREE type_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);
	pANTLR3_BASE_TREE offset_node = (pANTLR3_BASE_TREE)node->getChild(node, 2);
	pANTLR3_BASE_TREE rcount_node = (pANTLR3_BASE_TREE)node->getChild(node, 3);
	pANTLR3_BASE_TREE rstride_node = (pANTLR3_BASE_TREE)node->getChild(node, 4);
	pANTLR3_BASE_TREE ecount_node = (pANTLR3_BASE_TREE)node->getChild(node, 5);
	pANTLR3_BASE_TREE esize_node = (pANTLR3_BASE_TREE)node->getChild(node, 6);
	pANTLR3_BASE_TREE estride_node = (pANTLR3_BASE_TREE)node->getChild(node, 7);

	assert(id_node->getType(id_node) == AC_ID);
	assert(type_node->getType(type_node) == AC_ID);
	assert(offset_node->getType(offset_node) == AC_INT);
	assert(rcount_node->getType(rcount_node) == AC_INT);
	assert(rstride_node->getType(rstride_node) == AC_INT);
	assert(ecount_node->getType(ecount_node) == AC_INT);
	assert(esize_node->getType(esize_node) == AC_INT);
	assert(estride_node->getType(estride_node) == AC_INT);

	std::string id = std::string((char*)(id_node->getText(id_node)->chars));
	std::string type = std::string((char*)(type_node->getText(type_node)->chars));
	uint32_t offset = strtol((char*)offset_node->getText(offset_node)->chars, NULL, 10);
	uint32_t rcount = strtol((char*)rcount_node->getText(rcount_node)->chars, NULL, 10);
	uint32_t rstride = strtol((char*)rstride_node->getText(rstride_node)->chars, NULL, 10);
	uint32_t ecount = strtol((char*)ecount_node->getText(ecount_node)->chars, NULL, 10);
	uint32_t esize = strtol((char*)esize_node->getText(esize_node)->chars, NULL, 10);
	uint32_t estride = strtol((char*)estride_node->getText(estride_node)->chars, NULL, 10);

	return new RegBankViewDescriptor(space, id, offset, rcount, rstride, ecount, esize, estride, type);
}

RegSlotViewDescriptor *load_reg_view_slot(pANTLR3_BASE_TREE node, RegSpaceDescriptor *space)
{
	assert(node->getChildCount(node) == 4 || node->getChildCount(node) == 5);

	pANTLR3_BASE_TREE id_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
	pANTLR3_BASE_TREE type_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);
	pANTLR3_BASE_TREE width_node = (pANTLR3_BASE_TREE)node->getChild(node, 2);
	pANTLR3_BASE_TREE offset_node = (pANTLR3_BASE_TREE)node->getChild(node, 3);

	assert(id_node->getType(id_node) == AC_ID);
	assert(type_node->getType(type_node) == AC_ID);
	assert(width_node->getType(width_node) == AC_INT);
	assert(offset_node->getType(offset_node) == AC_INT);

	std::string id = std::string((char*)(id_node->getText(id_node)->chars));
	std::string type = std::string((char*)(type_node->getText(type_node)->chars));
	uint32_t width = strtol((char*)width_node->getText(width_node)->chars, NULL, 10);
	uint32_t offset = strtol((char*)offset_node->getText(offset_node)->chars, NULL, 10);

	if(node->getChildCount(node) == 5) {
		pANTLR3_BASE_TREE tag_node = (pANTLR3_BASE_TREE)node->getChild(node, 4);
		assert(tag_node->getType(tag_node) == AC_ID);

		std::string tag = std::string((char*)(tag_node->getText(tag_node)->chars));
		return new RegSlotViewDescriptor(space, id, type, width, offset, tag);
	} else {
		return new RegSlotViewDescriptor(space, id, type, width, offset);
	}
}

// generate a register space from a declaration
bool ArchDescriptionParser::load_regspace(pANTLR3_BASE_TREE childNode)
{
	pANTLR3_BASE_TREE sizeNode = (pANTLR3_BASE_TREE)childNode->getChild(childNode, 0);
	uint32_t space_size = strtoul((char*)(sizeNode->getText(sizeNode)->chars), NULL, 10);

	RegSpaceDescriptor *regspace = new RegSpaceDescriptor(&arch->GetRegFile(), space_size);

	arch->GetRegFile().AddRegisterSpace(regspace);

	for(uint32_t i = 1; i < childNode->getChildCount(childNode); ++i) {
		pANTLR3_BASE_TREE viewNode = (pANTLR3_BASE_TREE)childNode->getChild(childNode, i);
		switch(viewNode->getType(viewNode)) {
			case AC_REG_VIEW_BANK: {
				RegBankViewDescriptor *descriptor = load_reg_view_bank(viewNode, regspace);
				if(!descriptor)
					return false;

				if(!arch->GetRegFile().AddRegisterBank(regspace, descriptor))
					return false;

				break;
			}
			case AC_REG_VIEW_SLOT: {
				RegSlotViewDescriptor *descriptor = load_reg_view_slot(viewNode, regspace);
				if(!descriptor)
					return false;

				if(!arch->GetRegFile().AddRegisterSlot(regspace, descriptor))
					return false;

				break;
			}
		}
	}

	return true;
}

bool ArchDescriptionParser::load_feature_set(pANTLR3_BASE_TREE featuresNode)
{
	assert(featuresNode->getType(featuresNode) == AC_FEATURES);

	for(uint32_t i = 0; i < featuresNode->getChildCount(featuresNode); ++i) {
		pANTLR3_BASE_TREE featureNode = (pANTLR3_BASE_TREE)featuresNode->getChild(featuresNode, i);
		pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)featureNode->getChild(featureNode, 0);

		assert(nameNode->getType(nameNode) == AC_ID);

		std::string featureName = std::string((const char*)nameNode->getText(nameNode)->chars);
		if(!arch->GetFeatures().AddFeature(featureName)) {
			diag_ctx.Error("Could not add feature", DiagNode("", featureNode));
			return false;
		}
	}

	return true;
}

// walk the arch_ctor section

bool ArchDescriptionParser::load_arch_ctor(pANTLR3_BASE_TREE ctorNode)
{
	int next_isa_id = 0;
	bool success = true;

	// walk the ARCH_CTOR subtree
	for (uint32_t j = 0; j < ctorNode->getChildCount(ctorNode); j++) {
		pANTLR3_BASE_TREE archNode = (pANTLR3_BASE_TREE)ctorNode->getChild(ctorNode, j);

		switch (archNode->getType(archNode)) {
			case AC_ISA: {
				pANTLR3_BASE_TREE fileNameNode = (pANTLR3_BASE_TREE)archNode->getChild(archNode, 0);
				std::string filename = (const char *)fileNameNode->getText(fileNameNode)->chars;
				filename = filename.substr(1, filename.length() - 2);

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

			case AC_SET_FEATURE: {
				pANTLR3_BASE_TREE id_node = (pANTLR3_BASE_TREE)archNode->getChild(archNode, 0);
				pANTLR3_BASE_TREE level_node = (pANTLR3_BASE_TREE)archNode->getChild(archNode, 1);

				uint32_t level = atoi((char*)level_node->getText(level_node)->chars);
				auto *feature = arch->GetFeatures().GetFeature(std::string((char*)id_node->getText(id_node)->chars));
				if(!feature) {
					diag_ctx.Error("Could not find feature", DiagNode("", archNode));
					success = false;
				} else {
					feature->SetDefaultLevel(level);
				}

				break;
			}

			case SET_ENDIAN: {
				break;
			}
		}
	}
	return success;

}

