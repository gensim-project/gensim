grammar genC;

options
{
	language = C;
	k = 2;
  backtrack=false;
  memoize=false;
	output = AST;
	ASTLabelType = pANTLR3_BASE_TREE;
}

tokens
{
  FILETOK;
  HELPER;
  BEHAVIOUR;
  EXECUTE;
  SYSTEM;
  CONSTANT;
  BODY;
  DECL;
  CAST;
  BITCAST;
  TYPEDEF;

  OBRACE = '{';
  CBRACE = '}';

  GENC_ATTR_FIXED='fixed';  
  GENC_ATTR_SOMETIMES_FIXED='sometimes_fixed';

  GENC_SCOPE_PRIVATE = 'private';
  GENC_SCOPE_PUBLIC = 'public';
  GENC_SCOPE_INTERNAL = 'internal';
  
  GENC_ATTR_INLINE = 'inline';
  GENC_ATTR_NOINLINE = 'noinline';
  GENC_ATTR_GLOBAL = 'global';
  GENC_ATTR_EXPORT = 'export';
  
  GENC_BASICTYPE;
  GENC_STRUCT = 'struct';
  GENC_TYPENAME = 'typename';

  PARAM;
  PARAMS;
  TYPE;
  
  CASE;
  DEFAULT;
  BREAK;
  CONTINUE;
  RETURN;
  RAISE;
  
  WHILE;
  DO;
  FOR;
  FOR_PRE;
  FOR_CHECK;
  FOR_POST;
  
  IF;
  SWITCH;
  
  IDX;
  IDX_ELEM;
  IDX_BITS;
  MEMBER='.';
  PTR='->';
  CALL;
  
  TERNARY='?';
  
  PREFIX;
  POSTFIX;
  
  VECTOR;
  
  EXPR_STMT;
}

GENC_ID  :	('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'0'..'9'|'_')* ;

INT_CONST	:	'-'?('0'..'9')+ 'U'? 'L'?
	;

FLOAT_CONST: '-'?('0'..'9')+ '.' ('0'..'9')+ 'f'? 'U'? 'L'?;
  
COMMENT
    :   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
    ;

LINE_COMMENT
    : '//' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    ;
    
WS  :   ( ' '
        | '\t'
        | '\r'
        | '\n'
        ) {$channel=HIDDEN;}
    ;

STRING: '"' ~('"')* '"';   

fragment
HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F') ;

fragment
UNICODE_ESC
    :   '\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
    ;

HEX_VAL	:	'0x' HEX_DIGIT+ 'U'? 'L'?;

base_type : 
	GENC_STRUCT GENC_ID -> ^(GENC_STRUCT GENC_ID)
 |  numeric_type -> ^(GENC_BASICTYPE numeric_type)
 |  GENC_TYPENAME GENC_ID -> ^(GENC_TYPENAME GENC_ID);
full_type_definition :
  base_type vector_annotation? reference_annotation? -> ^(TYPE base_type vector_annotation? reference_annotation?);
type :  
	full_type_definition;

numeric_type : 
  'void'
  | 'uint8'
  | 'uint16'
  | 'uint32'
  | 'uint64'
  | 'uint128'
  | 'sint8'
  | 'sint16'
  | 'sint32'
  | 'sint64'
  | 'sint128'
  | 'float'
  | 'double'
  | 'longdouble'
  ;

reference_annotation :
	'&';

vector_annotation : 
	'[' INT_CONST ']' -> ^(VECTOR INT_CONST);

param_def:
	type GENC_ID -> ^(PARAM type GENC_ID);

param_list : 
  (param_def (',' param_def)*)? -> ^(PARAMS param_def*);
  
action_file : definition+ EOF -> ^(FILETOK definition*);

definition : ((GENC_TYPENAME) => (typedef_definition)) | constant_definition | action_definition;

constant_definition: 'const' declaration '=' constant ';' -> ^(CONSTANT declaration constant);

typedef_definition: GENC_TYPENAME GENC_ID '=' type';' -> ^(TYPEDEF GENC_ID type);

action_definition : helper_definition | execution_definition;

helper_scope : GENC_SCOPE_PRIVATE | GENC_SCOPE_PUBLIC | GENC_SCOPE_INTERNAL;

helper_definition : helper_scope? 'helper'? type GENC_ID '(' param_list ')' helper_attribute* body -> ^(HELPER GENC_ID type helper_scope param_list helper_attribute*  body);

helper_attribute: GENC_ATTR_FIXED | GENC_ATTR_SOMETIMES_FIXED | GENC_ATTR_INLINE | GENC_ATTR_NOINLINE | GENC_ATTR_EXPORT | GENC_ATTR_GLOBAL;

execution_definition : 'execute' '(' GENC_ID ')' body -> ^(EXECUTE GENC_ID body);

system_definition : 'system' '(' GENC_ID ')' body -> ^(SYSTEM GENC_ID body);

//Statements

body : OBRACE statement* CBRACE -> ^(BODY statement*);

statement : expression_statement | selection_statement | iteration_statement | flow_statement | body;

expression_statement :
   (';' -> ^(BODY))
  |(expression ';' -> ^(EXPR_STMT expression))
  ;
  
flow_statement :
   ('case' constant_expr ':' body -> ^(CASE constant_expr body))
  |('default' ':' statement -> ^(DEFAULT statement))
  |('break' ';' -> ^(BREAK))
  |('continue' ';' -> ^(CONTINUE))
  |('raise' ';' -> ^(RAISE))
  |('return' ';' -> ^(RETURN))
  |('return' ternary_expression ';' -> ^(RETURN ternary_expression))
  ;

iteration_statement : 
   ('while' '(' ternary_expression ')' statement -> ^(WHILE ternary_expression statement))
  |('do' statement 'while' '(' expression ')' -> ^(DO statement expression))
  |('for' '(' pre=expression? ';' check=ternary_expression ';' post=expression? ')' statement -> ^(FOR ^(FOR_PRE $pre) ^(FOR_CHECK $check) ^(FOR_POST $post) statement))
  ;

selection_statement : 
	if_statement | switch_statement;

if_statement
	:	 'if' '(' ternary_expression ')' statement (options { greedy=true; }: 'else' statement)? -> ^(IF ternary_expression statement statement?);
	 
switch_statement
	:	  'switch' '(' ternary_expression ')' body -> ^(SWITCH ternary_expression body);


// Expressions

expression: 
   declaration_expression
 | ((left_expression assignment_operator) => left_expression assignment_operator^ right_expression)
 | ternary_expression;

right_expression : 
	vector_expression
  | ternary_expression;

left_expression: GENC_ID | call_expression | left_index_expression;

left_index_expression : GENC_ID '[' ternary_expression ']' -> ^(POSTFIX GENC_ID ^(IDX ^(IDX_ELEM ternary_expression)));

constant_expr: log_or_expression; 

argument_list: (ternary_expression (','! ternary_expression )*)?;

constant: HEX_VAL | INT_CONST | FLOAT_CONST;

vector_expression: OBRACE ternary_expression (',' ternary_expression)* CBRACE -> ^(VECTOR ternary_expression*);

primary_expression:
  call_expression | GENC_ID | constant | '('! ternary_expression ')'!;

call_expression:
  GENC_ID '(' argument_list ')' -> ^(CALL GENC_ID argument_list);

unary_operator:
    '+'
  | '-'
  | '~'
  | '!';

postfix_expression:
    (primary_expression postfix_operator) => primary_expression postfix_operator* -> ^(POSTFIX primary_expression postfix_operator*)
    | primary_expression;

index_expression
	: ternary_expression -> ^(IDX_ELEM ternary_expression)
	| F=constant ':' T=constant -> ^(IDX_BITS $F $T);

postfix_operator:
    '[' index_expression ']' -> ^(IDX index_expression)
   |'.' GENC_ID -> ^(MEMBER GENC_ID)
   |'++'
   |'--'
   ;
  
declaration: type GENC_ID -> ^(DECL type GENC_ID);

declaration_expression: declaration ('='^ right_expression)?;

unary_expression:
  postfix_expression
  | '++' unary_expression -> ^(PREFIX '++' unary_expression)
  | '--' unary_expression -> ^(PREFIX '--' unary_expression)
  | unary_operator cast_expression -> ^(PREFIX unary_operator cast_expression);
 
assignment_operator:
  '='
  |'+='
  |'-='
  |'&='
  |'*='
  |'/='
  |'%='
  |'<<='
  |'>>='
  |'^='
  |'|=';
  
ternary_expression:
	log_or_expression (TERNARY^ log_or_expression ':'! right=log_or_expression)?;

log_or_expression:
   log_and_expression ('||'^ log_and_expression)*;  

log_and_expression:
   bit_or_expression ('&&'^ bit_or_expression)*;

bit_or_expression:
   bit_xor_expression ('|'^ bit_xor_expression)*;
  
bit_xor_expression:
   bit_and_expression ('^'^ bit_and_expression)*;

bit_and_expression:
   equality_expression ('&'^ equality_expression)*;
  
equality_expression:
   comparison_expression (('=='^ | '!='^ ) comparison_expression)*;

comparison_expression:
   shift_expression (('<'^ | '>'^ | '<='^ | '>='^) shift_expression)*;

shift_expression:
   add_expression (('<<<'^ | '<<'^ | '>>'^ | '>>>'^) add_expression)*;

add_expression:
   mult_expression (('+'^ | '-'^) mult_expression)*;

mult_expression:
   concat_expression (('*'^|'/'^|'%'^) concat_expression)*;

concat_expression:
	cast_expression ('::'^ cast_expression)*;

cast_expression:
  ('(' type ')') => '('type')' cast_expression -> ^(CAST type cast_expression)
  | bitcast_expression;

bitcast_expression:
  ('<' type '>') => '<'type'>' cast_expression -> ^(BITCAST type cast_expression)
  | unary_expression;

