/* gr1c -- Bison (and Yacc) grammar file
 *
 *
 * SCL; Jan 2012.
 */


%{
  #include <stdio.h>
  #include "ptree.h"
  void yyerror( char const * );

  extern ptree_t *evar_list;
  extern ptree_t *svar_list;
%}

%error-verbose
/* %define api.pure */
%pure_parser
%locations


%union {
  char * str;
  int num;
}


%token E_VARS
%token S_VARS

%token E_INIT
%token E_TRANS
%token E_GOAL

%token S_INIT
%token S_TRANS
%token S_GOAL

%token COMMENT
%token <str> VARIABLE
%token <num> NUMBER
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

exp: evar_list ';'
   | svar_list ';'
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
   | error  { printf( "Error detected on line %d.\n", @1.last_line ); YYABORT; }
;

evar_list: E_VARS
        | evar_list VARIABLE  {
              if (evar_list == NULL) {
                  evar_list = init_ptree( PT_VARIABLE, $2, 0 );
              } else {
                  append_list_item( evar_list, PT_VARIABLE, $2, 0 );
              }
          }
;

svar_list: S_VARS
        | svar_list VARIABLE  {
              if (svar_list == NULL) {
                  svar_list = init_ptree( PT_VARIABLE, $2, 0 );
              } else {
                  append_list_item( svar_list, PT_VARIABLE, $2, 0 );
              }
          }
;

transformula: SAFETY_OP tpropformula
            | SAFETY_OP tpropformula '&' transformula
;

goalformula: LIVENESS_OP propformula
           | LIVENESS_OP propformula '&' goalformula
;

propformula: TRUE_CONSTANT
           | FALSE_CONSTANT
           | VARIABLE
           | VARIABLE '=' NUMBER
           | propformula '&' propformula
           | propformula '|' propformula
           | propformula IMPLIES propformula
           | '!' propformula
           | '(' propformula ')'
;

tpropformula: TRUE_CONSTANT
	    | FALSE_CONSTANT
	    | VARIABLE
	    | VARIABLE '\''
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
