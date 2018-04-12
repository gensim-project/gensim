grammar archc;

options
{
	language=C;
	output = AST;
	ASTLabelType = pANTLR3_BASE_TREE;
	k=3;
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
	NOTEQUALS = '!=';
    PLUS = '+';
    MINUS = '-';
    PERCENT = '%';
    AMPERSAND = '&';
    XOR = '^';
    AT = '@';
    
    SEMICOLON = ';';
    COLON = ':';
    PERIOD = '.';
    COMMA = ',';
    GROUPING = '..';
    
	AC_ARCH = 'AC_ARCH';
	AC_FORMAT='ac_format';
	AC_WORDSIZE='ac_wordsize';
	AC_FETCHSIZE='ac_fetchsize';
	AC_PREDICATED='ac_predicated';
    AC_MEM = 'ac_mem';
    AC_TLM_PORT = 'ac_tlm_port';
    AC_INTR_PORT = 'ac_intr_port';
    AC_INCLUDE = 'include';
    
    AC_REGSPACE = 'ac_regspace';
    AC_REG_VIEW_SLOT = 'slot';
    AC_REG_VIEW_BANK = 'bank';
    
    AC_PIPE = 'ac_pipe';
    AC_PC = 'ac_pc';
        
    ARCH_CTOR = 'ARCH_CTOR';
    
    AC_ISA = 'ac_isa';
    SET_ENDIAN = 'set_endian';
    
    AC_ISA_BLOCK = 'AC_ISA';
    
    AC_INSTR = 'ac_instr';
    AC_FIELD = 'ac_field';
    AC_BEHAVIOUR = 'ac_behaviour';
    
    ISA_CTOR = 'ISA_CTOR';
    
    AC_ASM_MAP = 'ac_asm_map';
    
    SET_ASM = 'set_asm';
    SET_DECODER = 'set_decoder';
	SET_HAS_LIMM = 'set_has_limm';
    SET_BEHAVIOUR = 'set_behaviour';
	SET_EOB = 'set_end_of_block';
	SET_JUMP_FIXED = 'set_fixed_jump';
	SET_JUMP_FIXED_PREDICATED = 'set_fixed_predicated_jump';
	SET_JUMP_VARIABLE = 'set_variable_jump';
	SET_USES_PC = 'set_reads_pc';
	SET_BLOCK_COND = 'set_block_cond';

	FIXED = 'FIXED';
	RELATIVE = 'RELATIVE';
	
    ASSEMBLER = 'assembler';
    SET_COMMENT = 'set_comment';
    SET_LINE_COMMENT = 'set_line_comment';
    
    AC_MODIFIERS = 'ac_modifiers';
    
    AC_BEHAVIOURS = 'ac_behaviours';
    AC_DECODES = 'ac_decode';
    AC_EXECUTE = 'ac_execute';
    AC_SYSTEM = 'ac_system';
    
    AC_UARCH = 'ac_uarch';
        
    AC_FEATURES = 'ac_features';
    AC_FEATURE_LEVEL = 'feature';
    AC_FEATURE_FLAG = 'flag';
    
    AC_SET_FEATURE = 'set_feature';
        
    PSEUDO_INSTR = 'pseudo_instr';
    
	GROUP;
	GROUP_LRULE;
	STRINGS_LRULE;
    
    CONSTRAINT;
    ARG;
    
    FNCALL;
}

AC_ID  :	('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'0'..'9'|'_')*
    ;

AC_INT	:	('0'..'9')+
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
    :  QUOTE ( ESC_SEQ | ~('\\'|'\n'|QUOTE) )* QUOTE
    ;

fragment
AC_HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F') ;

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
    :   '\\' 'u' AC_HEX_DIGIT AC_HEX_DIGIT AC_HEX_DIGIT AC_HEX_DIGIT
    ;

AC_HEX_VAL	:	HEX_PREFIX AC_HEX_DIGIT+;


SIZE	:	AC_INT ('k'|'m'|'g'|'K'|'M'|'G')?;

def_attr : AT AC_ID -> ^(AC_ID);

arch	:	AC_ARCH OPAREN AC_ID CPAREN OBRACE arch_resource* arch_ctor CBRACE SEMICOLON -> ^(AC_ARCH AC_ID arch_resource* arch_ctor);

arch_resource
	:	(ar_wordsize | format | ar_mem | ar_tlm_port | ar_int_port | ar_regspace | ar_pipe | ar_pc | ar_features) ;

ar_features
	:	AC_FEATURES OBRACE ar_feature_def+ CBRACE -> ^(AC_FEATURES ar_feature_def+);

ar_feature_def
	:	AC_FEATURE_LEVEL id=AC_ID SEMICOLON -> ^(AC_FEATURE_LEVEL $id);

ar_wordsize
	:	AC_WORDSIZE AC_INT SEMICOLON -> ^(AC_WORDSIZE AC_INT);
	
ar_fetchsize
	:	AC_FETCHSIZE AC_INT SEMICOLON -> ^(AC_FETCHSIZE AC_INT);

ar_predicated
	:	AC_PREDICATED STRING SEMICOLON -> ^(AC_PREDICATED STRING);

ar_include
    :   AC_INCLUDE OPAREN STRING CPAREN SEMICOLON -> ^(AC_INCLUDE STRING);	

ar_regspace
	:	AC_REGSPACE OPAREN AC_INT CPAREN OBRACE ar_reg_view+ CBRACE -> ^(AC_REGSPACE AC_INT ar_reg_view+);

ar_reg_view
	:	ar_reg_view_bank | ar_reg_view_slot;

ar_reg_view_slot
	:	AC_REG_VIEW_SLOT id=AC_ID OPAREN type=AC_ID COMMA high=AC_INT COMMA low=AC_INT CPAREN tag=AC_ID? SEMICOLON -> ^(AC_REG_VIEW_SLOT $id $type $high $low $tag);  

// bank NAME (TYPE, OFFSET, COUNT, REG-STRIDE, # ELEMS, ELEM-SIZE, ELEM-STRIDE)

ar_reg_view_bank
	:	AC_REG_VIEW_BANK id=AC_ID OPAREN type=AC_ID COMMA offset=AC_INT COMMA regcnt=AC_INT COMMA regstride=AC_INT COMMA elemcnt=AC_INT COMMA elemsz=AC_INT COMMA elemstride=AC_INT CPAREN SEMICOLON -> ^(AC_REG_VIEW_BANK $id $type $offset $regcnt $regstride $elemcnt $elemsz $elemstride); 

format
	:	def_attr* AC_FORMAT AC_ID EQUALS STRING SEMICOLON -> ^(AC_FORMAT AC_ID STRING def_attr*);

ar_mem
	:	AC_MEM AC_ID OPAREN address_width=AC_INT COMMA word_width=AC_INT COMMA endian=AC_ID COMMA is_fetch=AC_INT CPAREN SEMICOLON -> ^(AC_MEM AC_ID $address_width $word_width $endian $is_fetch);

ar_tlm_port
	:	AC_TLM_PORT AC_ID COLON SIZE SEMICOLON -> ^(AC_TLM_PORT AC_ID SIZE); 

ar_int_port
	:	AC_INTR_PORT AC_ID SEMICOLON -> ^(AC_INTR_PORT AC_ID);

ar_pc
    :   AC_PC AC_ID OBRACKET AC_INT CBRACKET SEMICOLON -> ^(AC_PC AC_ID AC_INT);

ar_pipe
	:	AC_PIPE name=AC_ID EQUALS OBRACE AC_ID (COMMA AC_ID)* CBRACE -> ^(AC_PIPE $name AC_ID+);

arch_ctor
	:	ARCH_CTOR OPAREN AC_ID CPAREN OBRACE (arch_ctor_line SEMICOLON)* CBRACE SEMICOLON -> ^(ARCH_CTOR AC_ID arch_ctor_line*);

arch_ctor_line
	:	ac_isa | ac_endianness | ac_uarch | ac_feature_set;

ac_feature_set
	:	AC_SET_FEATURE OPAREN name=AC_ID COMMA level=AC_INT CPAREN -> ^(AC_SET_FEATURE $name $level);

ac_isa
	:	AC_ISA OPAREN STRING CPAREN -> ^(AC_ISA STRING);

ac_uarch
	:	AC_UARCH OPAREN STRING CPAREN -> ^(AC_UARCH STRING);
	
ac_endianness
	:	SET_ENDIAN OPAREN STRING CPAREN -> ^(SET_ENDIAN STRING);

arch_isa
	:	AC_ISA_BLOCK OPAREN AC_ID CPAREN OBRACE isa_resource* CBRACE SEMICOLON -> ^(AC_ISA_BLOCK AC_ID isa_resource*);

isa_resource
	:	(ar_fetchsize | ar_include | format | isa_instruction | isa_field | isa_behaviour | isa_CTOR | isa_asm_map | ar_predicated);
	
isa_instruction
	:	AC_INSTR OANGLE AC_ID CANGLE AC_ID (COMMA AC_ID)* SEMICOLON -> ^(AC_INSTR AC_ID*);

field_type
    :   AC_ID '*'? -> ^(AC_ID '*'?);
    
field_qualifier
    : '::' AC_ID -> ^('::' AC_ID);
    
isa_field
    :   AC_FIELD OANGLE field_type CANGLE field_qualifier? name=AC_ID SEMICOLON -> ^(AC_FIELD field_qualifier? $name field_type);
    
isa_behaviour
    :   AC_BEHAVIOUR AC_ID (COMMA AC_ID)* SEMICOLON -> ^(AC_BEHAVIOUR AC_ID*);
    
isa_CTOR
	:	ISA_CTOR OPAREN AC_ID CPAREN OBRACE ctor_resource* CBRACE SEMICOLON -> ^(ISA_CTOR AC_ID ctor_resource*);

isa_asm_map
	:	AC_ASM_MAP AC_ID OBRACE isa_mapping* CBRACE -> ^(AC_ASM_MAP AC_ID isa_mapping*);

group
	:	OBRACKET from=AC_INT GROUPING to=AC_INT CBRACKET -> ^(GROUPING $from $to);

isa_mapping
	:	(isa_mapping_group | isa_mapping_strings) SEMICOLON!;
	
isa_mapping_group_lrule
	:	STRING group -> ^(GROUP_LRULE STRING group);
	
isa_mapping_group
	:	isa_mapping_group_lrule EQUALS group -> ^(GROUP isa_mapping_group_lrule group);

isa_mapping_strings_lrule
	:	STRING (COMMA STRING)* -> ^(STRINGS_LRULE STRING STRING*);

isa_mapping_strings
	:	isa_mapping_strings_lrule EQUALS AC_INT -> ^( isa_mapping_strings_lrule AC_INT);

asmResource
	:	AC_ID PERIOD SET_ASM OPAREN STRING (COMMA (asmArg | asmConstraint))* CPAREN -> ^(SET_ASM AC_ID STRING asmArg* asmConstraint*);
    
asmArg	:	addexpr -> ^(ARG addexpr);

addexpr :   val PLUS addexpr -> ^(PLUS val addexpr) | subexpr;

subexpr	:	val MINUS subexpr -> ^(MINUS val subexpr) | fncall_expr;

fncall_expr
	:	AC_ID OPAREN (addexpr (',' addexpr)*)? CPAREN -> ^(FNCALL AC_ID addexpr*)
	|	val
    | '(' addexpr ')';

val 	:	AC_ID | AC_INT | AC_HEX_VAL;

asmConstraint
	:	AC_ID EQUALS addexpr -> ^(CONSTRAINT AC_ID addexpr);

	//Todo: support != operator here (requires substantial changes to InstructionDescription::GetBitString
decodeOperator
	: EQUALS
  | AMPERSAND
  | NOTEQUALS
  | XOR;
	
decodeConstraint
    :	AC_ID decodeOperator addexpr -> ^(AC_ID addexpr decodeOperator);
    
decoderResource
	:	AC_ID PERIOD SET_DECODER OPAREN (decodeConstraint (COMMA decodeConstraint)*)? CPAREN -> ^(SET_DECODER AC_ID decodeConstraint*);

limmResource
	:	AC_ID PERIOD SET_HAS_LIMM OPAREN AC_INT CPAREN -> ^(SET_HAS_LIMM AC_ID AC_INT);
	
behaviourResource
    :   instr=AC_ID PERIOD SET_BEHAVIOUR OPAREN behaviour=AC_ID CPAREN -> ^(SET_BEHAVIOUR $instr $behaviour);
    
eobResource
	:	instr=AC_ID PERIOD SET_EOB OPAREN (decodeConstraint (COMMA decodeConstraint)*)? CPAREN -> ^(SET_EOB $instr decodeConstraint*);
	
jumpVariableResource
	:	instr = AC_ID PERIOD SET_JUMP_VARIABLE OPAREN CPAREN -> ^(SET_JUMP_VARIABLE $instr);
	
blockCondResource
	:	instr = AC_ID PERIOD SET_BLOCK_COND OPAREN CPAREN -> ^(SET_BLOCK_COND $instr);	
	
fixed_jump_type
	:	FIXED | RELATIVE;
	
jumpOffset
	:	MINUS? AC_INT;
	
jumpFixedResource
	:	instr=AC_ID PERIOD SET_JUMP_FIXED OPAREN field=AC_ID COMMA fixed_jump_type (COMMA jumpOffset)? CPAREN -> ^(SET_JUMP_FIXED $instr $field fixed_jump_type jumpOffset);

jumpFixedPredicatedResource
	:	instr=AC_ID PERIOD SET_JUMP_FIXED_PREDICATED OPAREN field=AC_ID COMMA fixed_jump_type (COMMA jumpOffset)? CPAREN -> ^(SET_JUMP_FIXED_PREDICATED $instr $field fixed_jump_type jumpOffset);
	
usesPcResource
	:	instr = AC_ID PERIOD SET_USES_PC OPAREN (decodeConstraint (COMMA decodeConstraint)*)? CPAREN -> ^(SET_USES_PC $instr decodeConstraint*);
	
instr_resource
	:	(decoderResource | asmResource | behaviourResource | eobResource | jumpVariableResource | jumpFixedResource | jumpFixedPredicatedResource | usesPcResource | limmResource | blockCondResource) SEMICOLON!;

assembler_resource
	:	ASSEMBLER PERIOD (rsc=SET_COMMENT | rsc=SET_LINE_COMMENT) OPAREN STRING CPAREN SEMICOLON -> ^(ASSEMBLER $rsc STRING);
	
modifiers_resource
    :   AC_MODIFIERS OPAREN STRING CPAREN SEMICOLON -> ^(AC_MODIFIERS STRING);
    
behaviours_resource
    :   AC_BEHAVIOURS OPAREN STRING CPAREN SEMICOLON -> ^(AC_BEHAVIOURS STRING);
    
decodes_resource
	:	AC_DECODES OPAREN STRING CPAREN SEMICOLON -> ^(AC_DECODES STRING);
	
execute_resource
	:	AC_EXECUTE OPAREN STRING CPAREN SEMICOLON -> ^(AC_EXECUTE STRING);

system_resource
	:	AC_SYSTEM OPAREN STRING CPAREN SEMICOLON -> ^(AC_SYSTEM STRING);

ctor_resource
	:	instr_resource
	|	assembler_resource
    |   modifiers_resource
    |   behaviours_resource
    |   decodes_resource 
    |   execute_resource 
    |   system_resource
	|	PSEUDO_INSTR OPAREN STRING CPAREN OBRACE (STRING SEMICOLON)+ CBRACE -> ^(PSEUDO_INSTR STRING*);

