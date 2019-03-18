/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <flexbison_harness.h>
#include <genC.tabs.h>
#include <genC.l.h>


int main(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "Usage: genc-test [input gc file]\n");
		return 1;
	}

	FILE *f = fopen(argv[1], "r");

	yyscan_t scanner;

	yylex_init(&scanner);
	yyset_in(f, scanner);

	astnode<GenCNodeType> *root = CreateNode(GenCNodeType::ROOT);

	yyparse(scanner, root);

	yylex_destroy(scanner);

	root->Dump();

	fclose(f);

	0 + 0 ? 0 ? 0 : 0 : 0;
}
