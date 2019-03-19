/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <flexbison_harness.h>
#include <flexbison_genc_ast.h>
#include <genC.tabs.h>
#include <genC.l.h>

#include <flexbison_genc.h>

#include <fstream>

int main(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "Usage: genc-test [input gc file]\n");
		return 1;
	}

	std::ifstream infile(argv[1]);

	astnode<GenCNodeType, GenC::location> root_node(GenCNodeType::ROOT);

	GenC::GenCScanner scanner(&infile);
	GenC::GenCParser parser(scanner, &root_node);

	if(parser.parse() != 0) {
		std::cerr << "Failed to parse\n";
		return 1;
	}

	root_node.Dump();

	return 0;
}
