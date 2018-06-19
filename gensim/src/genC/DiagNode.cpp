/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <antlr3.h>

#include "genC/DiagNode.h"

gensim::DiagNode::DiagNode(const std::string& filename, pANTLR3_BASE_TREE node) : filename_(filename), line_number_(node->getLine(node)), column_(node->getCharPositionInLine(node))
{
}


std::ostream &operator<<(std::ostream &str, const gensim::DiagNode& node)
{
	str << node.Filename() << ":" << node.Line();
	return str;
}
