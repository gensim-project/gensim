grammar archcasm;

options
{
    output=AST;
    language=C;
    ASTLabelType = pANTLR3_BASE_TREE;
 
}

tokens
{
    ASM_STMT;
    ASM_ARG;
    ASM_LOWER;
    ASM_HIGHER;
    ASM_REL;
    ASM_ALIGN;
    ASM_BIT_LIST;
    ASM_MODIFIER;
    
    ASM_ESC_SEQ = '\\%';
}

//although the archc assembler generator doesn't care about whitespace, we do. therefore we must keep track of it.
ASM_WS : (' ' | '\t')+;

ASM_INT : ('0' .. '9')+;

ASM_ID_CHAR : 'a'..'z' | 'A'..'Z';

ASM_ID_STRING : (ASM_ID_CHAR) (ASM_ID_CHAR | ASM_INT)*;


asmStmt : mnemonic_string operand_string -> ^(ASM_STMT mnemonic_string operand_string?);

soloChar : '[' | ']' | ',' | ';' | ':' | '!' | '^' | '&' | '*' | '(' | ')' | '#' | '+' | '-' | '{' | '}' | '@' | '_' | '$' | '.' | '<' | '>' | '?' | '/' | '\\';

mnemonic_string : ASM_ID_STRING | ASM_ID_CHAR | arg_string;

operand_string : (op_text | arg_string)*;

op_text : ASM_ESC_SEQ | ASM_ID_STRING | ASM_ID_CHAR | soloChar | ASM_WS | ASM_INT;

arg_string : '%'! (brkt_arg | arg_text);

brkt_arg : ('[') => ('[' arg_text ']') -> arg_text;

//arg_text may include the modifier string, we'll handle this later since the grammar becomes unmanageable otherwise
arg_text : ASM_ID_STRING (('(' | '...') => modifier_string)? -> ^(ASM_ARG ASM_ID_STRING modifier_string?);

modifier_string : '...' -> ASM_BIT_LIST;
