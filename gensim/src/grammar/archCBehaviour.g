grammar archCBehaviour;

options
{
	language=C;
	output = AST;
	ASTLabelType = pANTLR3_BASE_TREE;
}

tokens
{
    BE_ARCHC_BEHAVIOUR;
    
    BE_ARGS;
    BE_ARG;
    
    BE_CONSTANT;
    BE_DECODE;
    BE_EXECUTE;
    BE_HELPER;
    BE_BEHAVIOUR;
    
    SIG;
}

BE_WS  :   ( ' '
        | '\t'
        | '\r'
        | '\n'
        ) {$channel=HIDDEN;}
    ;

BE_FN_BODY : 
    '{'
        (
            options {greedy=false;} : 
                ('{') => BE_FN_BODY 
              | .
        )*
    '}';

BE_COMMENT
    :   '//' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    |   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
    ;
    
    
BE_ID : ('a'..'z' | 'A'..'Z' | '_') ('a'..'z' | 'A'..'Z' | '0'..'9' | '_')*;


fragment
HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F') ;
BE_INT_CONST	:	'-'?('0'..'9')+
	;

BE_HEX_VAL	:	'0x' HEX_DIGIT+;


behaviour_file
    :   behaviour_entry* EOF -> ^(BE_ARCHC_BEHAVIOUR behaviour_entry*);

behaviour_entry
    :   constant | decode_behaviour | instruction_behaviour | helper_fn | behaviour;

constant_value: BE_HEX_VAL | BE_INT_CONST;
constant: 'const' BE_ID BE_ID '=' constant_value ';';

decode_behaviour
    :   'decode' '(' BE_ID ')' BE_FN_BODY -> ^(BE_DECODE BE_ID BE_FN_BODY);

instruction_behaviour
    :   'execute' '(' BE_ID ')' BE_FN_BODY -> ^(BE_EXECUTE BE_ID BE_FN_BODY);

behaviour
    :   'behaviour' '(' BE_ID ')' fn_attr* BE_FN_BODY -> ^(BE_BEHAVIOUR BE_ID BE_FN_BODY);

vector_spec: '[' constant_value ']';

type
    :   'const'? 'struct'? BE_ID vector_spec? ('*'|'&')? 'const'?;
    
arg
    :   type BE_ID | BE_ID;
    
args
    : (arg (',' arg)*)?;

modifier
    : 'inline';
    
fn_attr : BE_ID -> ^(BE_ID);

fn_sig
    :   modifier* type BE_ID '(' args ')' fn_attr* -> ^(SIG modifier* type BE_ID '(' args ')' fn_attr*);

helper_scope : 'private' | 'public' | 'internal';
    
helper_fn
    :   helper_scope? 'helper' fn_sig BE_FN_BODY -> ^(BE_HELPER fn_sig BE_FN_BODY);
