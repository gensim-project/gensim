/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "isa/AsmMapDescriptionParser.h"

#include <archc/archcParser.h>
#include <archc/archcLexer.h>

using namespace gensim::isa;


void ParseGroup(ArchC::AstNode &node, uint8_t *first, uint8_t *second)
{

	auto &fromNode = node[0];
	auto &toNode = node[1];

	*first = fromNode.GetInt();
	*second = toNode.GetInt();
}

AsmMapDescription AsmMapDescriptionParser::Parse(const ArchC::AstNode &pnode)
{
	AsmMapDescription output;

	output.Name = pnode[0][0].GetString();

	auto &mapListNode = pnode[1];
	for (auto childPtr : mapListNode) {
		auto &child = *childPtr;

		switch (child[0].GetType()) {
			case ArchCNodeType::AsmMapGroupLRule: {
				// fromNode is a GROUP_LRULE node which contains a string followed by a GROUPING
				auto &fromNode = child[0];
				// toNode is a GROUPING node
				auto &toNode = child[1];

				std::pair<uint8_t, uint8_t> to_group;
				ParseGroup(toNode, &to_group.first, &to_group.second);

				auto &fromStringNode = fromNode[0];
				auto &fromGroupNode = fromNode[1];

				std::pair<uint8_t, uint8_t> from_group;
				ParseGroup(fromGroupNode, &from_group.first, &from_group.second);

				// check that the groups have the same width:
				uint8_t width = to_group.second - to_group.first;
				uint8_t from_width = from_group.second - from_group.first;
				if (from_width != width) {
					fprintf(stderr, "Error: ASM Map groups have differing widths (%u and %u)\n", from_width, width);
					break;
				}

				// add the actual mappings to our map

				// first, strip the quotes from the name string
				std::string text = fromStringNode.GetString();
				text = text.substr(1, text.length() - 2);

				for (uint32_t i = 0; i <= width; i++) {
					std::stringstream str;
					str << text;
					str << (uint32_t)(from_group.first + i);
					output.mapping[to_group.first + i] = str.str();
				}

				break;
			}
			case ArchCNodeType::STRING: {
				// direct maps are simple: just map the text of the from node to the number represented by
				// the text of the to node.
				std::string from = child[0].GetString();
				int to = child[1].GetInt();

				from = from.substr(1, from.length() - 2);

				output.mapping[to] = from;

				break;
			}
		}
	}

	return output;
}
