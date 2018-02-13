grammar archCuArch;

options
{
	language=C;
	output = AST;
	ASTLabelType = pANTLR3_BASE_TREE;
}

tokens
{
    OPAREN = '(';
    CPAREN = ')';
    OBRACE = '{';
    CBRACE = '}';
    OBRACKET = '[';
    CBRACKET = ']';
    OANGLE = '<';
    CANGLE = '>';
    
    QUOTE = '"';

    HEX_PREFIX = '0x';
    
    EQUALS = '=';
    
    SEMICOLON = ';';
    COLON = ':';
    PERIOD = '.';
    COMMA = ',';
    GROUPING = '..';
	
	CARET='^';
	LOGAND='&&';
	LOGOR='||';
	BITAND='&';
	BITOR='|';
	QMARK='?';
	
	AC_UARCH= 'ac_uarch';
	AC_PIPELINE_STAGE= 'ac_pipeline';
	AC_UNIT_TYPE = 'ac_unit_type';
	AC_FUNC_UNIT= 'ac_func_unit';
	AC_GROUP= 'ac_group';
	AC_UARCH_TYPE = 'ac_uarch_type';
	
	SET_GROUP= 'set_group';
	SET_LATENCY_CONST= 'set_latency_const';
	SET_LATENCY_PRED= 'set_latency_pred';
	SET_FLUSH_PIPELINE = 'set_flush_pipeline';
	SET_DEFAULT_UNIT = 'set_default_unit';
	SET_FUNC_UNIT = 'set_func_unit';
	
	UARCH_TYPE_INORDER = 'InOrder';
	UARCH_TYPE_SUPERSCALAR = 'Superscalar';
	UARCH_TYPE_OUTOFORDER = 'OutOfOrder';
	
	SET_SRC='set_src';
	SET_DEST='set_dest';
	
	
	
	FUNC;
	PROPERTY;
	
	UARCH_DESC;
}

ID  :	('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'0'..'9'|'_')*
    ;

INT	:	('0'..'9')+
	;

COMMENT
    :   '//' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    |   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
    ;

WS  :   ( ' '
        | '\t'
        | '\r'
        | '\n'
        ) {$channel=HIDDEN;}
    ;
    
STRING
    :  QUOTE ( ESC_SEQ | ~('\\'|QUOTE) )* QUOTE
    ;

fragment
HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F') ;

fragment
ESC_SEQ
    :   '\\' ('b'|'t'|'n'|'f'|'r'|'\"'|'\''|'\\')
    |   UNICODE_ESC
    |   OCTAL_ESC
    ;

fragment
OCTAL_ESC
    :   '\\' ('0'..'3') ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7')
    ;

fragment
UNICODE_ESC
    :   '\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
    ;

HEX_VAL	:	HEX_PREFIX HEX_DIGIT+;

UNARY_OP : '!'|'~';
MULT_OP : '*'|'/'|'%';
ADDITION_OP:'+'|'-';
SHIFT_OP:'>>'|'<<';
REL_OP:'>'|'<'|'>='|'<=';
EQ_OP:'=='|'!=';

uarch_expression: uarch_expression_logor;

uarch_expression_ternary: p=uarch_expression_mult QMARK a=uarch_expression_mult COLON b=uarch_expression_mult -> ^(QMARK $p $a $b);

uarch_expression_logor: uarch_expression_logand (LOGOR uarch_expression_logand)* -> ^(LOGOR uarch_expression_logand*);

uarch_expression_logand: uarch_expression_bitor (LOGAND uarch_expression_bitor)* -> ^(LOGAND uarch_expression_bitor*);

uarch_expression_bitor: uarch_expression_bitxor (BITOR uarch_expression_bitxor)* -> ^(BITOR uarch_expression_bitxor*);

uarch_expression_bitxor: uarch_expression_bitand (CARET uarch_expression_bitand)* -> ^(CARET uarch_expression_bitand*);

uarch_expression_bitand: uarch_expression_equality (BITAND uarch_expression_equality)* -> ^(BITAND uarch_expression_equality*);

uarch_expression_equality: uarch_expression_relation ((EQ_OP) uarch_expression_relation)* -> ^(EQ_OP uarch_expression_relation*);

uarch_expression_relation: uarch_expression_shift ((REL_OP) uarch_expression_shift)* -> ^(REL_OP uarch_expression_shift*);

uarch_expression_shift: uarch_expression_addition ((SHIFT_OP) uarch_expression_addition)* -> ^(SHIFT_OP uarch_expression_addition*);

uarch_expression_addition: uarch_expression_mult ((ADDITION_OP) uarch_expression_mult)* -> ^(ADDITION_OP uarch_expression_mult*);

uarch_expression_mult: uarch_expression_value ((MULT_OP) uarch_expression_value)* -> ^(MULT_OP uarch_expression_value*);

uarch_expression_value: uarch_expression_leafexpr | OPAREN! uarch_expression CPAREN!;

uarch_expression_leafexpr: INT | ID | uarch_expression_property | uarch_expression_func;

uarch_expression_property : (ID PERIOD) => (a=ID PERIOD p=ID) -> ^(PROPERTY $a $p);

uarch_expression_func : (ID OPAREN) => (ID OPAREN (uarch_expression (COMMA uarch_expression)*)? CPAREN) -> ^(FUNC ID uarch_expression*);

uarch: AC_UARCH OBRACE uarch_desc uarch_line* CBRACE -> ^(AC_UARCH uarch_desc uarch_line*);

uarch_desc: uarch_type_line? uarch_desc_line*;

uarch_type_line: AC_UARCH_TYPE uarch_type SEMICOLON -> ^(AC_UARCH_TYPE uarch_type);

uarch_type:  UARCH_TYPE_INORDER | UARCH_TYPE_SUPERSCALAR | UARCH_TYPE_OUTOFORDER;

uarch_desc_line: uarch_desc_line_pipeline_stage | uarch_desc_line_func_unit | uarch_desc_line_unit_type;

uarch_desc_line_pipeline_stage : AC_PIPELINE_STAGE ID COLON INT SEMICOLON -> ^(AC_PIPELINE_STAGE ID INT);

uarch_desc_line_unit_type: AC_UNIT_TYPE ID (COMMA ID)* SEMICOLON -> ^(AC_UNIT_TYPE ID+);

uarch_desc_line_func_unit: AC_FUNC_UNIT OANGLE stage=ID CANGLE list=uarch_desc_line_func_unit_list SEMICOLON -> ^(AC_FUNC_UNIT $stage $list);
uarch_desc_line_func_unit_list: ID (COMMA ID)* -> ID*;

uarch_line: uarch_line_group_decl | uarch_line_property;

uarch_line_group_decl: AC_GROUP ID SEMICOLON -> ^(AC_GROUP ID);

uarch_line_property: uarch_line_property_insn_membership | uarch_line_property_stage_latency | uarch_line_property_src | uarch_line_property_dest | uarch_line_property_src_banked | uarch_line_property_dest_banked | uarch_line_property_flush| uarch_line_property_set_default_unit | uarch_line_property_set_func_unit;

uarch_line_property_set_default_unit: stage=ID PERIOD SET_DEFAULT_UNIT OPAREN unit=ID CPAREN SEMICOLON -> ^(SET_DEFAULT_UNIT $stage $unit);

uarch_line_property_set_func_unit: insn=ID PERIOD SET_FUNC_UNIT OPAREN stage=ID COMMA unit=ID CPAREN SEMICOLON -> ^(SET_FUNC_UNIT $insn $stage $unit);

uarch_line_property_insn_membership: insn=ID PERIOD SET_GROUP OPAREN group=ID CPAREN SEMICOLON -> ^(SET_GROUP $insn $group);

uarch_line_property_stage_latency: uarch_line_property_stage_latency_const | uarch_line_property_stage_latency_predicated;

uarch_line_property_stage_latency_const: insn=ID PERIOD SET_LATENCY_CONST OPAREN stage=ID COMMA latency=uarch_expression CPAREN SEMICOLON -> ^(SET_LATENCY_CONST $insn $stage $latency);

uarch_line_property_stage_latency_predicated: insn=ID PERIOD SET_LATENCY_PRED OPAREN stage=ID COMMA latency=uarch_expression COMMA predicate=uarch_expression CPAREN SEMICOLON -> ^(SET_LATENCY_PRED $insn $stage $latency $predicate);

uarch_line_property_src: insn=ID PERIOD SET_SRC OPAREN stage=ID COMMA field=ID CPAREN SEMICOLON -> ^(SET_SRC $insn $stage $field);

uarch_line_property_src_banked: insn=ID PERIOD SET_SRC OPAREN stage=ID COMMA bank=ID COMMA num=uarch_expression CPAREN SEMICOLON -> ^(SET_SRC $insn $stage $bank  $num);

uarch_line_property_dest: insn=ID PERIOD SET_DEST OPAREN stage=ID COMMA field=ID CPAREN SEMICOLON -> ^(SET_DEST $insn $stage $field);

uarch_line_property_dest_banked: insn=ID PERIOD SET_DEST OPAREN stage=ID COMMA field=ID COMMA num=uarch_expression CPAREN SEMICOLON -> ^(SET_DEST $insn $stage $field $num);

uarch_line_property_flush : insn = ID PERIOD SET_FLUSH_PIPELINE OPAREN (predicate=uarch_expression)? CPAREN SEMICOLON -> ^(SET_FLUSH_PIPELINE $insn $predicate);
