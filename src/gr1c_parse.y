/* gr1c -- Bison (and Yacc) grammar file
 *
 *
 * SCL; 2012-2015
 */


%{
  #include <stdlib.h>
  #include <stdio.h>
  #include "ptree.h"
  void yyerror( char const * );
  int yylex( void );

  ptree_t *evar_list = NULL;
  ptree_t *svar_list = NULL;

  ptree_t *sys_init = NULL;
  ptree_t *env_init = NULL;

  ptree_t **env_goals = NULL;
  ptree_t **sys_goals = NULL;
  int num_egoals = 0;
  int num_sgoals = 0;

  ptree_t **env_trans_array = NULL;
  ptree_t **sys_trans_array = NULL;
  int et_array_len = 0;
  int st_array_len = 0;

  /* General purpose tree pointer,
     which facilitates cleaner Yacc parsing code. */
  ptree_t *gen_tree_ptr = NULL;
%}

%error-verbose
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

%token LE_OP
%token GE_OP
%token NOT_EQUALS
%left EQUIV
%left IMPLIES
%left SAFETY_OP
%left AND_SAFETY_OP
%left LIVENESS_OP
%left AND_LIVENESS_OP
%left EVENTUALLY_OP

%left '&' '|' '!' '=' '\''

%%

input: /* empty */
     | input exp
     | propformula
;

exp: evar_list ';'
   | svar_list ';'
   | E_INIT ';' {
       if (env_init != NULL) {
       printf( "Error detected on line %d.  Duplicate ENVINIT\n",
               @1.last_line );
       YYABORT;
       }
     }
   | E_INIT propformula ';' {
       if (env_init != NULL) {
           printf( "Error detected on line %d.  Duplicate ENVINIT\n",
                   @1.last_line );
           YYABORT;
       }
       env_init = gen_tree_ptr;
       gen_tree_ptr = NULL;
     }
   | E_TRANS ';' {
       if (et_array_len == 0) {
           et_array_len = 1;
           env_trans_array = malloc( sizeof(ptree_t *) );
           if (env_trans_array == NULL) {
               perror( "gr1c_parse.y, etransformula, malloc" );
               exit(-1);
           }
           *env_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
       }
     }
   | E_TRANS etransformula ';'
   | E_GOAL ';'
   | E_GOAL egoalformula ';'
   | S_INIT ';' {
       if (sys_init != NULL) {
           printf( "Error detected on line %d.  Duplicate SYSINIT.\n",
                   @1.last_line );
           YYABORT;
       }
     }
   | S_INIT propformula ';' {
       if (sys_init != NULL) {
           printf( "Error detected on line %d.  Duplicate SYSINIT.\n",
                   @1.last_line );
           YYABORT;
       }
       sys_init = gen_tree_ptr;
       gen_tree_ptr = NULL;
     }
   | S_TRANS ';' {
       if (st_array_len == 0) {
           st_array_len = 1;
           sys_trans_array = malloc( sizeof(ptree_t *) );
           if (sys_trans_array == NULL) {
               perror( "gr1c_parse.y, stransformula, malloc" );
               exit(-1);
           }
           *sys_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
       }
     }
   | S_TRANS stransformula ';'
   | S_GOAL ';' {
       num_sgoals = 1;
       sys_goals = malloc( sizeof(ptree_t *) );
       if (sys_goals == NULL) {
           perror( "gr1c_parse.y, S_GOAL ';', malloc" );
           exit(-1);
       }
       /* Equivalent to []<>True */
       *sys_goals = init_ptree( PT_CONSTANT, NULL, 1 );
     }
   | S_GOAL sgoalformula ';'
   | error  { printf( "Error detected on line %d.\n", @1.last_line ); YYABORT; }
;

evar_list: E_VARS
         | evar_list VARIABLE  {
             if (evar_list == NULL) {
                 evar_list = init_ptree( PT_VARIABLE, $2, -1 );
             } else {
                 append_list_item( evar_list, PT_VARIABLE, $2, -1 );
             }
                 free( $2 );
           }
         | evar_list VARIABLE '[' NUMBER ',' NUMBER ']'  {
             if ($4 != 0) {
                 fprintf( stderr,
                          "Error detected on line %d.  Lower bound must be"
                          " zero.\n",
                          @1.last_line );
                 YYABORT;
             } else if ($6 < 0) {
                 fprintf( stderr,
                          "Error detected on line %d.  Upper bound must be"
                          " no less than zero.\n",
                          @1.last_line );
                 YYABORT;
             }
             if (evar_list == NULL) {
                 evar_list = init_ptree( PT_VARIABLE, $2, $6 );
             } else {
                 append_list_item( evar_list, PT_VARIABLE, $2, $6 );
             }
                 free( $2 );
           }
;

svar_list: S_VARS
         | svar_list VARIABLE  {
             if (svar_list == NULL) {
                 svar_list = init_ptree( PT_VARIABLE, $2, -1 );
             } else {
                 append_list_item( svar_list, PT_VARIABLE, $2, -1 );
             }
                 free( $2 );
           }
         | svar_list VARIABLE '[' NUMBER ',' NUMBER ']'  {
             if ($4 != 0) {
                 fprintf( stderr,
                          "Error detected on line %d.  Lower bound must be"
                          " zero.\n",
                          @1.last_line );
                 YYABORT;
             } else if ($6 < 0) {
                 fprintf( stderr,
                          "Error detected on line %d.  Upper bound must be"
                          " no less than zero.\n",
                          @1.last_line );
                 YYABORT;
             }
             if (svar_list == NULL) {
                 svar_list = init_ptree( PT_VARIABLE, $2, $6 );
             } else {
                 append_list_item( svar_list, PT_VARIABLE, $2, $6 );
             }
             free( $2 );
           }
;

etransformula: SAFETY_OP tpropformula  {
                 et_array_len++;
                 env_trans_array = realloc( env_trans_array,
                                            sizeof(ptree_t *)*et_array_len );
                 if (env_trans_array == NULL) {
                     perror( "gr1c_parse.y, etransformula, realloc" );
                     exit(-1);
                 }
                 *(env_trans_array+et_array_len-1) = gen_tree_ptr;
                 gen_tree_ptr = NULL;
               }
             | etransformula AND_SAFETY_OP tpropformula  {
                 et_array_len++;
                 env_trans_array = realloc( env_trans_array,
                                            sizeof(ptree_t *)*et_array_len );
                 if (env_trans_array == NULL) {
                     perror( "gr1c_parse.y, etransformula, realloc" );
                     exit(-1);
                 }
                 *(env_trans_array+et_array_len-1) = gen_tree_ptr;
                 gen_tree_ptr = NULL;
               }
;

stransformula: SAFETY_OP tpropformula  {
                 st_array_len++;
                 sys_trans_array = realloc( sys_trans_array,
                                            sizeof(ptree_t *)*st_array_len );
                 if (sys_trans_array == NULL) {
                     perror( "gr1c_parse.y, stransformula, realloc" );
                     exit(-1);
                 }
                 *(sys_trans_array+st_array_len-1) = gen_tree_ptr;
                 gen_tree_ptr = NULL;
               }
             | stransformula AND_SAFETY_OP tpropformula  {
                 st_array_len++;
                 sys_trans_array = realloc( sys_trans_array,
                                            sizeof(ptree_t *)*st_array_len );
                 if (sys_trans_array == NULL) {
                     perror( "gr1c_parse.y, stransformula, realloc" );
                     exit(-1);
                 }
                 *(sys_trans_array+st_array_len-1) = gen_tree_ptr;
                 gen_tree_ptr = NULL;
               }
;

egoalformula: LIVENESS_OP propformula  {
                num_egoals++;
                env_goals = realloc( env_goals,
                                     sizeof(ptree_t *)*num_egoals );
                if (env_goals == NULL) {
                    perror( "gr1c_parse.y, egoalformula, realloc" );
                    exit(-1);
                }
                *(env_goals+num_egoals-1) = gen_tree_ptr;
                gen_tree_ptr = NULL;
              }
            | egoalformula AND_LIVENESS_OP propformula  {
                num_egoals++;
                env_goals = realloc( env_goals,
                                     sizeof(ptree_t *)*num_egoals );
                if (env_goals == NULL) {
                    perror( "gr1c_parse.y, egoalformula, realloc" );
                    exit(-1);
                }
                *(env_goals+num_egoals-1) = gen_tree_ptr;
                gen_tree_ptr = NULL;
              }
;

sgoalformula: LIVENESS_OP propformula  {
                num_sgoals++;
                sys_goals = realloc( sys_goals,
                                     sizeof(ptree_t *)*num_sgoals );
                if (sys_goals == NULL) {
                    perror( "gr1c_parse.y, sgoalformula, realloc" );
                    exit(-1);
                }
                *(sys_goals+num_sgoals-1) = gen_tree_ptr;
                gen_tree_ptr = NULL;
              }
            | sgoalformula AND_LIVENESS_OP propformula  {
                num_sgoals++;
                sys_goals = realloc( sys_goals,
                                     sizeof(ptree_t *)*num_sgoals );
                if (sys_goals == NULL) {
                    perror( "gr1c_parse.y, sgoalformula, realloc" );
                    exit(-1);
                }
                *(sys_goals+num_sgoals-1) = gen_tree_ptr;
                gen_tree_ptr = NULL;
              }
;

propformula: TRUE_CONSTANT  {
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_CONSTANT, NULL, 1 );
             }
           | FALSE_CONSTANT  {
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_CONSTANT, NULL, 0 );
             }
           | VARIABLE  {
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_VARIABLE, $1, 0 );
               free( $1 );
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
           | propformula EQUIV propformula  {
               gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_EQUIV );
             }
           | VARIABLE '=' NUMBER  {
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_VARIABLE, $1, 0 );
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_CONSTANT, NULL, $3 );
               gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_EQUALS );
               free( $1 );
             }
           | VARIABLE '<' NUMBER  {
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_VARIABLE, $1, 0 );
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_CONSTANT, NULL, $3 );
               gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_LT );
               free( $1 );
             }
           | VARIABLE '>' NUMBER  {
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_VARIABLE, $1, 0 );
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_CONSTANT, NULL, $3 );
               gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_GT );
               free( $1 );
             }
           | VARIABLE LE_OP NUMBER  {  /* less than or equal to */
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_VARIABLE, $1, 0 );
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_CONSTANT, NULL, $3 );
               gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_LE );
               free( $1 );
             }
           | VARIABLE GE_OP NUMBER  {  /* greater than or equal to */
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_VARIABLE, $1, 0 );
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_CONSTANT, NULL, $3 );
               gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_GE );
               free( $1 );
             }
           | VARIABLE NOT_EQUALS NUMBER  {
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_VARIABLE, $1, 0 );
               gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                              PT_CONSTANT, NULL, $3 );
               gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_NOTEQ );
               free( $1 );
             }
           | '(' propformula ')'
;

tpropformula: TRUE_CONSTANT  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, 1 );
              }
            | FALSE_CONSTANT  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, 0 );
              }
            | VARIABLE  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_VARIABLE, $1, 0 );
                free( $1 );
              }
            | VARIABLE '\''  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_NEXT_VARIABLE, $1, 0 );
                free( $1 );
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
            | tpropformula EQUIV tpropformula  {
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_EQUIV );
              }
            | VARIABLE '=' NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $3 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_EQUALS );
                free( $1 );
              }
            | VARIABLE '\'' '=' NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_NEXT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $4 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_EQUALS );
                free( $1 );
              }
            | VARIABLE '<' NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $3 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_LT );
                free( $1 );
              }
            | VARIABLE '\'' '<' NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_NEXT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $4 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_LT );
                free( $1 );
              }
            | VARIABLE '>' NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $3 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_GT );
                free( $1 );
              }
            | VARIABLE '\'' '>' NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_NEXT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $4 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_GT );
                free( $1 );
              }
            | VARIABLE LE_OP NUMBER  {  /* less than or equal to */
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $3 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_LE );
                free( $1 );
              }
            | VARIABLE '\'' LE_OP NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_NEXT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $4 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_LE );
                free( $1 );
              }
            | VARIABLE GE_OP NUMBER  {  /* greater than or equal to */
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $3 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_GE );
                free( $1 );
              }
            | VARIABLE '\'' GE_OP NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_NEXT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $4 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_GE );
                free( $1 );
              }
            | VARIABLE NOT_EQUALS NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $3 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_NOTEQ );
                free( $1 );
              }
            | VARIABLE '\'' NOT_EQUALS NUMBER  {
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_NEXT_VARIABLE, $1, 0 );
                gen_tree_ptr = pusht_terminal( gen_tree_ptr,
                                               PT_CONSTANT, NULL, $4 );
                gen_tree_ptr = pusht_operator( gen_tree_ptr, PT_NOTEQ );
                free( $1 );
              }
            | '(' tpropformula ')'
;


%%

void yyerror( char const *s )
{
    fprintf( stderr, "%s\n", s );
}
