/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <flexbison_harness.h>
#include <flexbison_archc_ast.h>
#include <archC.tabs.h>
#include <archC.l.h>

#include <flexbison_archc.h>

#include <fstream>

int main(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "Usage: archc-test [input ac file]\n");
		return 1;
	}

	std::ifstream infile(argv[1]);

	astnode<ArchCNodeType> root_node(ArchCNodeType::ROOT);

	ArchC::ArchCScanner scanner(&infile);
	ArchC::ArchCParser parser(scanner, &root_node);

	if(parser.parse() != 0) {
		std::cerr << "Failed to parse\n";
		return 1;
	}

	root_node.Dump();

	return 0;
}
