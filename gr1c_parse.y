/* gr1c -- Bison (and Yacc) grammar file
 *
 * As visual feedback during parsing, a dot is written to the screen
 * for each variable name recognized.
 *
 *
 * SCL; Jan 2012.
 */


%{
  #include <stdio.h>
  void yyerror( char const * );
%}

%error-verbose
/* %define api.pure */
%pure_parser
%locations


%token E_VARS
%token S_VARS

%token E_INIT
%token E_TRANS
%token E_GOAL

%token S_INIT
%token S_TRANS
%token S_GOAL

%token COMMENT
%token VARIABLE
%token NUMBER
%token TRUE_CONSTANT
%token FALSE_CONSTANT

%left IMPLIES
%left SAFETY_OP
%left LIVENESS_OP

%left '&' '|' '!' '=' '\''

%%

input: /* empty */
     | input exp
;

exp: var_list ';'
   | E_INIT ';'
   | E_INIT propformula ';'
   | E_TRANS ';'
   | E_TRANS transformula ';'
   | E_GOAL ';'
   | E_GOAL goalformula ';'
   | S_INIT ';'
   | S_INIT propformula ';'
   | S_TRANS ';'
   | S_TRANS transformula ';'
   | S_GOAL ';'
   | S_GOAL goalformula ';'
   | error { printf( "\nError detected on line %d.\n", @1.last_line ); YYABORT; }
;

var_list: E_VARS
        | S_VARS
        | var_list VARIABLE
;

transformula: SAFETY_OP tpropformula
            | SAFETY_OP tpropformula '&' transformula
;

goalformula: LIVENESS_OP propformula
           | LIVENESS_OP propformula '&' goalformula
;

propformula: TRUE_CONSTANT
           | FALSE_CONSTANT
           | VARIABLE  { printf( "." ); }
           | VARIABLE '=' NUMBER
           | propformula '&' propformula
           | propformula '|' propformula
           | propformula IMPLIES propformula
           | '!' propformula
           | '(' propformula ')'
;

tpropformula: TRUE_CONSTANT
	    | FALSE_CONSTANT
	    | VARIABLE  { printf( "." ); }
	    | VARIABLE '\''  { printf( "." ); }
	    | VARIABLE '=' NUMBER
	    | tpropformula '&' tpropformula
	    | tpropformula '|' tpropformula
	    | tpropformula IMPLIES tpropformula
	    | '!' tpropformula
	    | '(' tpropformula ')'
;


%%

void yyerror( char const *s )
{
	fprintf( stderr, "%s\n", s );
}
