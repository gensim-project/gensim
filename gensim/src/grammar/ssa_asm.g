grammar ssa_asm;

options
{
	language=C;
	output = AST;
	ASTLabelType = pANTLR3_BASE_TREE;
	k=5;
}

tokens {
    ACTION = 'action';
    ACTION_PARAMS;
    ACTION_PARAMETER;
    ACTION_SYMBOLS;
    ACTION_BLOCK_DEF_LIST;
    ACTION_BLOCKS;
    ACTION_ATTRIBUTE_LIST;

ACTION_EXTERNAL;

STATEMENT;

CONTEXT;
BLOCK;
BLOCK_STATEMENTS;

SSAASM_TYPE_REF;
SSAASM_TYPE_VECTOR;

STATEMENT_BANKREGREAD = 'bankregread';
STATEMENT_BANKREGWRITE = 'bankregwrite';
STATEMENT_BINOP = 'binary';
STATEMENT_CALL = 'call';
STATEMENT_CAST = 'cast';
STATEMENT_CONSTANT = 'constant';
STATEMENT_IF = 'if';
STATEMENT_INTRINSIC = 'intrinsic';
STATEMENT_JUMP= 'jump';
STATEMENT_MEMREAD = 'memread';
STATEMENT_MEMWRITE = 'memwrite';

STATEMENT_PHI = 'phi';

STATEMENT_DEVREAD = 'devread';
STATEMENT_DEVWRITE = 'devwrite';

STATEMENT_READ = 'read';
STATEMENT_REGREAD = 'regread';
STATEMENT_REGWRITE = 'regwrite';
STATEMENT_RETURN = 'return';
STATEMENT_RAISE = 'raise';
STATEMENT_STRUCT = 'struct';
STATEMENT_SELECT = 'select';
STATEMENT_SWITCH = 'switch';
STATEMENT_UNOP = 'unary';
STATEMENT_VINSERT = 'vinsert';
STATEMENT_VEXTRACT = 'vextract';
STATEMENT_WRITE = 'write';

BINOP_PLUS = '+';
NEGATIVE = '-';
BINOP_MUL = '*';
BINOP_DIV = '/';
BINOP_MOD = '%';
BINOP_OR = '|';
BINOP_XOR = '^';
BINOP_SHL = '<<';
BINOP_SHR = '>>';
BINOP_SAR = '->>';
BINOP_ROL = '<<<';
BINOP_ROR = '>>>';

AMPERSAND = '&';
EXCLAMATION = '!';
COMPLEMENT = '~';

CMP_EQUALS = '==';
CMP_NOTEQUALS = '!=';
CMP_GT = '>';
CMP_GTE = '>=';
CMP_LT = '<';
CMP_LTE = '<=';

ATTRIBUTE_NOINLINE = 'noinline';
ATTRIBUTE_HELPER = 'helper';
EXTERNAL = 'external';
}

SSAASM_TYPE : 'uint8' | 'uint16' | 'uint32' | 'uint64' | 'sint8' | 'sint16' | 'sint32' | 'sint64' | 'float' | 'double' | 'void' | 'Instruction';

vector_size : '[' constant_value ']' -> ^(SSAASM_TYPE_VECTOR constant_value);

ref : AMPERSAND -> SSAASM_TYPE_REF;

type : SSAASM_TYPE vector_size? ref? -> ^(SSAASM_TYPE vector_size? ref?);

SSAASM_ID : ('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'0'..'9'|'_')*;

unary_op : EXCLAMATION | COMPLEMENT | NEGATIVE;

WS  :   ( ' '
        | '\t'
        | '\r'
        | '\n'
        ) {$channel=HIDDEN;};

SSAASM_INT : ('0'..'9')+;

SSAASM_DOUBLE : ('0'..'9')+ '.' ('0'..'9')+;
SSAASM_FLOAT : ('0'..'9')+ '.' ('0'..'9')+ 'f';

binary_op : BINOP_PLUS | NEGATIVE | BINOP_MUL | BINOP_DIV | BINOP_MOD | AMPERSAND | BINOP_OR | BINOP_XOR | BINOP_SHR | BINOP_SAR | BINOP_SHL | CMP_EQUALS | CMP_NOTEQUALS | CMP_GT | CMP_GTE | CMP_LT | CMP_LTE | BINOP_ROL | BINOP_ROR;

constant_value : SSAASM_INT | SSAASM_FLOAT | SSAASM_DOUBLE;

asm_file : context ;

context : context_entry* -> ^(CONTEXT context_entry*);

context_entry : action | action_external;

action : ACTION type SSAASM_ID action_attribute_list action_parameter_list action_symbol_list action_block_def_list action_block_list -> ^(ACTION SSAASM_ID type action_attribute_list action_parameter_list action_symbol_list action_block_def_list action_block_list);

action_external : ACTION EXTERNAL type SSAASM_ID action_attribute_list action_external_parameter_list ';' -> ^(ACTION_EXTERNAL SSAASM_ID type action_attribute_list action_external_parameter_list);

action_attribute : ATTRIBUTE_NOINLINE | ATTRIBUTE_HELPER;

action_attribute_list : action_attribute* -> ^(ACTION_ATTRIBUTE_LIST action_attribute*);
action_parameter_list : '(' action_parameter* ')' -> ^(ACTION_PARAMS action_parameter*);
action_external_parameter_list : '(' action_external_parameter* ')' -> ^(ACTION_PARAMS action_external_parameter*);
action_symbol_list : '[' action_symbol* ']' -> ^(ACTION_SYMBOLS action_symbol*);
action_block_def_list : '<' SSAASM_ID+ '>' -> ^(ACTION_BLOCK_DEF_LIST SSAASM_ID+);
action_block_list : '{' block+ '}' -> ^(ACTION_BLOCKS block+);



action_symbol : type SSAASM_ID;
action_parameter : type SSAASM_ID;
action_external_parameter : type;

block: 'block' SSAASM_ID block_statement_list -> ^(BLOCK SSAASM_ID block_statement_list);

block_statement_list : '{' statement* '}' -> ^(BLOCK_STATEMENTS statement*);

statement : (valued_statement | valueless_statement) ';'!;

valued_statement : SSAASM_ID '=' valued_statement_body -> ^(STATEMENT SSAASM_ID valued_statement_body);

valueless_statement : SSAASM_ID ':' valueless_statement_body -> ^(STATEMENT SSAASM_ID valueless_statement_body);

valued_statement_body : binary_statement | call_statement | constant_statement | cast_statement | dev_read_statement | dev_write_statement | register_read_statement | bank_register_read_statement | intrinsic_statement | phi_statement | select_statement | struct_member_statement | unary_statement | variable_read_statement | vector_insert_statement | vector_extract_statement;

binary_statement : STATEMENT_BINOP binary_op SSAASM_ID SSAASM_ID -> ^(STATEMENT_BINOP binary_op SSAASM_ID SSAASM_ID);

constant_statement : STATEMENT_CONSTANT type constant_value -> ^(STATEMENT_CONSTANT type constant_value);

cast_statement : STATEMENT_CAST cast_type=SSAASM_ID cast_option=SSAASM_ID? type source_stmt=SSAASM_ID -> ^(STATEMENT_CAST type $source_stmt $cast_type $cast_option?);

dev_read_statement : STATEMENT_DEVREAD SSAASM_ID SSAASM_ID SSAASM_ID -> ^(STATEMENT_DEVREAD SSAASM_ID SSAASM_ID SSAASM_ID);

dev_write_statement : STATEMENT_DEVWRITE SSAASM_ID SSAASM_ID SSAASM_ID -> ^(STATEMENT_DEVWRITE SSAASM_ID SSAASM_ID SSAASM_ID);

register_read_statement : STATEMENT_REGREAD constant_value -> ^(STATEMENT_REGREAD constant_value);

bank_register_read_statement : STATEMENT_BANKREGREAD constant_value SSAASM_ID -> ^(STATEMENT_BANKREGREAD constant_value SSAASM_ID);

intrinsic_statement : STATEMENT_INTRINSIC constant_value SSAASM_ID* -> ^(STATEMENT_INTRINSIC constant_value SSAASM_ID*);

phi_statement : STATEMENT_PHI type '[' SSAASM_ID* ']' -> ^(STATEMENT_PHI type SSAASM_ID*);

select_statement : STATEMENT_SELECT SSAASM_ID SSAASM_ID SSAASM_ID -> ^(STATEMENT_SELECT SSAASM_ID SSAASM_ID SSAASM_ID);

struct_member_statement : STATEMENT_STRUCT SSAASM_ID SSAASM_ID -> ^(STATEMENT_STRUCT SSAASM_ID SSAASM_ID);

unary_statement : STATEMENT_UNOP unary_op SSAASM_ID -> ^(STATEMENT_UNOP unary_op SSAASM_ID);

variable_read_statement: STATEMENT_READ SSAASM_ID -> ^(STATEMENT_READ SSAASM_ID);

vector_insert_statement : STATEMENT_VINSERT SSAASM_ID '[' SSAASM_ID ']' SSAASM_ID -> ^(STATEMENT_VINSERT SSAASM_ID SSAASM_ID SSAASM_ID);

vector_extract_statement : STATEMENT_VEXTRACT SSAASM_ID '[' SSAASM_ID ']' -> ^(STATEMENT_VEXTRACT SSAASM_ID SSAASM_ID);

valueless_statement_body : if_statement | jump_statement | mem_read_statement | mem_write_statement | register_write_statement | banked_register_write_statement | return_statement | raise_statement | switch_statement | variable_write_statement | call_statement | intrinsic_statement;

if_statement : STATEMENT_IF SSAASM_ID SSAASM_ID SSAASM_ID -> ^(STATEMENT_IF SSAASM_ID SSAASM_ID SSAASM_ID);

jump_statement: STATEMENT_JUMP SSAASM_ID -> ^(STATEMENT_JUMP SSAASM_ID);

mem_read_statement: STATEMENT_MEMREAD SSAASM_INT SSAASM_ID SSAASM_ID -> ^(STATEMENT_MEMREAD SSAASM_INT SSAASM_ID SSAASM_ID);

mem_write_statement: STATEMENT_MEMWRITE SSAASM_INT SSAASM_ID SSAASM_ID -> ^(STATEMENT_MEMWRITE SSAASM_INT SSAASM_ID SSAASM_ID);

register_write_statement : STATEMENT_REGWRITE constant_value SSAASM_ID -> ^(STATEMENT_REGWRITE constant_value SSAASM_ID);

banked_register_write_statement : STATEMENT_BANKREGWRITE constant_value SSAASM_ID SSAASM_ID -> ^(STATEMENT_BANKREGWRITE constant_value SSAASM_ID SSAASM_ID);

return_statement : STATEMENT_RETURN SSAASM_ID? -> ^(STATEMENT_RETURN SSAASM_ID?);

raise_statement : STATEMENT_RAISE -> ^(STATEMENT_RAISE);

switch_statement : STATEMENT_SWITCH SSAASM_ID SSAASM_ID '[' (SSAASM_ID SSAASM_ID)+ ']' -> ^(STATEMENT_SWITCH SSAASM_ID*);

variable_write_statement: STATEMENT_WRITE SSAASM_ID SSAASM_ID -> ^(STATEMENT_WRITE SSAASM_ID SSAASM_ID);

call_statement : STATEMENT_CALL SSAASM_ID SSAASM_ID* -> ^(STATEMENT_CALL SSAASM_ID*);
