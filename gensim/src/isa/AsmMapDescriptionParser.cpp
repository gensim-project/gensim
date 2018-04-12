/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "isa/AsmMapDescriptionParser.h"

#include <archc/archcParser.h>
#include <archc/archcLexer.h>

using namespace gensim::isa;


void ParseGroup(pANTLR3_BASE_TREE node, uint8_t *first, uint8_t *second)
{

	pANTLR3_BASE_TREE fromNode = (pANTLR3_BASE_TREE)node->getChild(node, 0);
	pANTLR3_BASE_TREE toNode = (pANTLR3_BASE_TREE)node->getChild(node, 1);

	char *str1 = (char *)fromNode->getText(fromNode)->chars;
	char *str2 = (char *)toNode->getText(toNode)->chars;
	*first = atoi(str1);
	*second = atoi(str2);
}

AsmMapDescription AsmMapDescriptionParser::Parse(void *pnode)
{
	AsmMapDescription output;
	
	pANTLR3_BASE_TREE node = (pANTLR3_BASE_TREE)pnode;
	pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)(node->getChild(node, 0));
	output.Name = (char *)nameNode->getText(nameNode)->chars;

	for (uint32_t i = 1; i < node->getChildCount(node); i++) {
		pANTLR3_BASE_TREE mapNode = (pANTLR3_BASE_TREE)(node->getChild(node, i));
		switch (mapNode->getType(mapNode)) {
			case GROUP: {
				// fromNode is a GROUP_LRULE node which contains a string followed by a GROUPING
				pANTLR3_BASE_TREE fromNode = (pANTLR3_BASE_TREE)(mapNode->getChild(mapNode, 0));
				// toNode is a GROUPING node
				pANTLR3_BASE_TREE toNode = (pANTLR3_BASE_TREE)(mapNode->getChild(mapNode, 1));

				std::pair<uint8_t, uint8_t> to_group;
				ParseGroup(toNode, &to_group.first, &to_group.second);

				pANTLR3_BASE_TREE fromStringNode = (pANTLR3_BASE_TREE)(fromNode->getChild(fromNode, 0));
				pANTLR3_BASE_TREE fromGroupNode = (pANTLR3_BASE_TREE)(fromNode->getChild(fromNode, 1));

				std::pair<uint8_t, uint8_t> from_group;
				ParseGroup(fromGroupNode, &from_group.first, &from_group.second);

				// check that the groups have the same width:
				uint8_t width = to_group.second - to_group.first;
				uint8_t from_width = from_group.second - from_group.first;
				if (from_width != width) {
					fprintf(stderr, "Error: ASM Map groups have differing widths (%u and %u) on line %u\n", from_width, width, toNode->getLine(toNode));
					break;
				}

				// add the actual mappings to our map

				// first, strip the quotes from the name string
				std::string text = (char *)(fromStringNode->getText(fromStringNode)->chars);
				text = text.substr(1, text.length() - 2);

				for (uint32_t i = 0; i <= width; i++) {
					std::stringstream str;
					str << text;
					str << (uint32_t)(from_group.first + i);
					output.mapping[to_group.first + i] = str.str();
				}

				break;
			}
			case STRINGS_LRULE: {
				// direct maps are simple: just map the text of the from node to the number represented by
				// the text of the to node.
				pANTLR3_BASE_TREE fromNode = (pANTLR3_BASE_TREE)(mapNode->getChild(mapNode, 0));
				pANTLR3_BASE_TREE toNode = (pANTLR3_BASE_TREE)(mapNode->getChild(mapNode, 1));

				std::string text = (char *)(fromNode->getText(fromNode)->chars);
				text = text.substr(1, text.length() - 2);

				output.mapping[atoi((char *)(toNode->getText(toNode)->chars))] = text;

				break;
			}
		}
	}
	
	return output;
}
