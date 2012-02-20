/* gr1c -- Bison (and Yacc) grammar file
 *
 *
 * SCL; Jan, Feb 2012.
 */


%{
  #include <stdlib.h>
  #include <stdio.h>
  #include "ptree.h"
  void yyerror( char const * );

  extern ptree_t *evar_list;
  extern ptree_t *svar_list;
  
  extern ptree_t *sys_init;
  extern ptree_t *env_init;

  extern ptree_t **env_trans_array;
  extern ptree_t **sys_trans_array;
  extern int et_array_len;
  extern int st_array_len;

  extern ptree_t *gen_tree_ptr;
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
%left AND_SAFETY_OP
%left LIVENESS_OP
%left AND_LIVENESS_OP

%left '&' '|' '!' '=' '\''

%%

input: /* empty */
     | input exp
;

exp: evar_list ';'
   | svar_list ';'
   | E_INIT ';'
   | E_INIT propformula ';' {
         if (env_init != NULL)
             delete_tree( env_init );
         env_init = gen_tree_ptr;
         gen_tree_ptr = NULL;
     }
   | E_TRANS ';'
   | E_TRANS etransformula ';'
   | E_GOAL ';'
   | E_GOAL goalformula ';'
   | S_INIT ';'
   | S_INIT propformula ';' {
         if (sys_init != NULL)
             delete_tree( sys_init );
         sys_init = gen_tree_ptr;
         gen_tree_ptr = NULL;
     }
   | S_TRANS ';'
   | S_TRANS stransformula ';'
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

etransformula: SAFETY_OP tpropformula  {
                   et_array_len++;
                   env_trans_array = realloc( env_trans_array, sizeof(ptree_t *)*et_array_len );
                   if (env_trans_array == NULL) {
                       perror( "gr1c_parse.y, etransformula, realloc" );
                       YYABORT;
                   }
                   *(env_trans_array+et_array_len-1) = gen_tree_ptr;
                   gen_tree_ptr = NULL;
               }
             | etransformula AND_SAFETY_OP tpropformula  {
                   et_array_len++;
                   env_trans_array = realloc( env_trans_array, sizeof(ptree_t *)*et_array_len );
                   if (env_trans_array == NULL) {
                       perror( "gr1c_parse.y, etransformula, realloc" );
                       YYABORT;
                   }
                   *(env_trans_array+et_array_len-1) = gen_tree_ptr;
                   gen_tree_ptr = NULL;
               }
;

stransformula: SAFETY_OP tpropformula  {
                   st_array_len++;
                   sys_trans_array = realloc( sys_trans_array, sizeof(ptree_t *)*st_array_len );
                   if (sys_trans_array == NULL) {
                       perror( "gr1c_parse.y, stransformula, realloc" );
                       YYABORT;
                   }
                   *(sys_trans_array+st_array_len-1) = gen_tree_ptr;
                   gen_tree_ptr = NULL;
               }
             | stransformula AND_SAFETY_OP tpropformula  {
                   st_array_len++;
                   sys_trans_array = realloc( sys_trans_array, sizeof(ptree_t *)*st_array_len );
                   if (sys_trans_array == NULL) {
                       perror( "gr1c_parse.y, stransformula, realloc" );
                       YYABORT;
                   }
                   *(sys_trans_array+st_array_len-1) = gen_tree_ptr;
                   gen_tree_ptr = NULL;
               }
;

goalformula: LIVENESS_OP propformula
           | LIVENESS_OP propformula '&' goalformula
;

propformula: TRUE_CONSTANT  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_CONSTANT, NULL, 1 );
             }
           | FALSE_CONSTANT  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_CONSTANT, NULL, 0 );
             }
           | VARIABLE  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_VARIABLE, $1, 0 );
             }
           | '!' propformula  {
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_NEG );
             }
           | propformula '&' propformula  {
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_AND );
             }
           | propformula '|' propformula  {
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_OR );
             }
           | propformula IMPLIES propformula  {
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_IMPLIES );
             }
           | VARIABLE '=' NUMBER  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_VARIABLE, $1, 0 );
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_CONSTANT, NULL, $3 );
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_EQUALS );
             }
           | '(' propformula ')'
;

tpropformula: TRUE_CONSTANT  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_CONSTANT, NULL, 1 );
             }
           | FALSE_CONSTANT  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_CONSTANT, NULL, 0 );
             }
           | VARIABLE  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_VARIABLE, $1, 0 );
             }
           | VARIABLE '\''  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_NEXT_VARIABLE, $1, 0 );
             }
           | '!' tpropformula  {
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_NEG );
             }
           | tpropformula '&' tpropformula  {
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_AND );
             }
           | tpropformula '|' tpropformula  {
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_OR );
             }
           | tpropformula IMPLIES tpropformula  {
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_IMPLIES );
             }
           | VARIABLE '=' NUMBER  {
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_VARIABLE, $1, 0 );
                 gen_tree_ptr = pusht_terminal( gen_tree_ptr, PT_CONSTANT, NULL, $3 );
                 gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_EQUALS );
             }
           | '(' tpropformula ')'
;


%%

void yyerror( char const *s )
{
	fprintf( stderr, "%s\n", s );
}
