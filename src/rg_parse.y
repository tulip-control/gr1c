/* rg -- Bison (and Yacc) grammar file for reach game formulae
 *
 *
 * SCL; 2012-2015
 */


%{
  #include <stdlib.h>
  #include <stdio.h>
  #include "common.h"
  #include "ptree.h"
  void yyerror( char const * );
  int yylex( void );

  specification_t spc;

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
       if (spc.env_init != NULL) {
       printf( "Error detected on line %d.  Duplicate ENVINIT\n",
               @1.last_line );
       YYABORT;
       }
     }
   | E_INIT propformula ';' {
       if (spc.env_init != NULL) {
           printf( "Error detected on line %d.  Duplicate ENVINIT\n",
                   @1.last_line );
           YYABORT;
       }
       spc.env_init = gen_tree_ptr;
       gen_tree_ptr = NULL;
     }
   | E_TRANS ';' {
       if (spc.et_array_len == 0) {
           spc.et_array_len = 1;
           spc.env_trans_array = malloc( sizeof(ptree_t *) );
           if (spc.env_trans_array == NULL) {
               perror( "gr1c_parse.y, etransformula, malloc" );
               exit(-1);
           }
           *spc.env_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
       }
     }
   | E_TRANS etransformula ';'
   | E_GOAL ';'
   | E_GOAL egoalformula ';'
   | S_INIT ';' {
       if (spc.sys_init != NULL) {
           printf( "Error detected on line %d.  Duplicate SYSINIT.\n",
                   @1.last_line );
           YYABORT;
       }
     }
   | S_INIT propformula ';' {
       if (spc.sys_init != NULL) {
           printf( "Error detected on line %d.  Duplicate SYSINIT.\n",
                   @1.last_line );
           YYABORT;
       }
       spc.sys_init = gen_tree_ptr;
       gen_tree_ptr = NULL;
     }
   | S_TRANS ';' {
       if (spc.st_array_len == 0) {
           spc.st_array_len = 1;
           spc.sys_trans_array = malloc( sizeof(ptree_t *) );
           if (spc.sys_trans_array == NULL) {
               perror( "gr1c_parse.y, stransformula, malloc" );
               exit(-1);
           }
           *spc.sys_trans_array = init_ptree( PT_CONSTANT, NULL, 1 );
       }
     }
   | S_TRANS stransformula ';'
   | S_GOAL ';' {
       spc.num_sgoals = 1;
       spc.sys_goals = malloc( sizeof(ptree_t *) );
       if (spc.sys_goals == NULL) {
           perror( "gr1c_parse.y, S_GOAL ';', malloc" );
           exit(-1);
       }
       /* Equivalent to []<>True */
       *spc.sys_goals = init_ptree( PT_CONSTANT, NULL, 1 );
     }
   | S_GOAL sgoalformula ';'
   | error  { printf( "Error detected on line %d.\n", @1.last_line ); YYABORT; }
;

evar_list: E_VARS
         | evar_list VARIABLE  {
             if ((spc.evar_list != NULL
                  && (find_list_item( spc.evar_list, PT_VARIABLE, $2, -1 ) != -1
                      || find_list_item( spc.evar_list, PT_VARIABLE, $2, -1 ) != -1))
                 || (spc.svar_list != NULL
                  && (find_list_item( spc.svar_list, PT_VARIABLE, $2, -1 ) != -1
                      || find_list_item( spc.svar_list, PT_VARIABLE, $2, -1 ) != -1))) {
                 fprintf( stderr,
                          "Duplicate variable detected on line %d.\n",
                          @1.last_line );
                 YYABORT;
             }
             if (spc.evar_list == NULL) {
                 spc.evar_list = init_ptree( PT_VARIABLE, $2, -1 );
             } else {
                 append_list_item( spc.evar_list, PT_VARIABLE, $2, -1 );
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
             if ((spc.evar_list != NULL
                  && (find_list_item( spc.evar_list, PT_VARIABLE, $2, $6 ) != -1
                      || find_list_item( spc.evar_list, PT_VARIABLE, $2, $6 ) != -1))
                 || (spc.svar_list != NULL
                  && (find_list_item( spc.svar_list, PT_VARIABLE, $2, $6 ) != -1
                      || find_list_item( spc.svar_list, PT_VARIABLE, $2, $6 ) != -1))) {
                 fprintf( stderr,
                          "Duplicate variable detected on line %d.\n",
                          @1.last_line );
                 YYABORT;
             }
             if (spc.evar_list == NULL) {
                 spc.evar_list = init_ptree( PT_VARIABLE, $2, $6 );
             } else {
                 append_list_item( spc.evar_list, PT_VARIABLE, $2, $6 );
             }
                 free( $2 );
           }
;

svar_list: S_VARS
         | svar_list VARIABLE  {
             if ((spc.evar_list != NULL
                  && (find_list_item( spc.evar_list, PT_VARIABLE, $2, -1 ) != -1
                      || find_list_item( spc.evar_list, PT_VARIABLE, $2, -1 ) != -1))
                 || (spc.svar_list != NULL
                  && (find_list_item( spc.svar_list, PT_VARIABLE, $2, -1 ) != -1
                      || find_list_item( spc.svar_list, PT_VARIABLE, $2, -1 ) != -1))) {
                 fprintf( stderr,
                          "Duplicate variable detected on line %d.\n",
                          @1.last_line );
                 YYABORT;
             }
             if (spc.svar_list == NULL) {
                 spc.svar_list = init_ptree( PT_VARIABLE, $2, -1 );
             } else {
                 append_list_item( spc.svar_list, PT_VARIABLE, $2, -1 );
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
             if ((spc.evar_list != NULL
                  && (find_list_item( spc.evar_list, PT_VARIABLE, $2, $6 ) != -1
                      || find_list_item( spc.evar_list, PT_VARIABLE, $2, $6 ) != -1))
                 || (spc.svar_list != NULL
                  && (find_list_item( spc.svar_list, PT_VARIABLE, $2, $6 ) != -1
                      || find_list_item( spc.svar_list, PT_VARIABLE, $2, $6 ) != -1))) {
                 fprintf( stderr,
                          "Duplicate variable detected on line %d.\n",
                          @1.last_line );
                 YYABORT;
             }
             if (spc.svar_list == NULL) {
                 spc.svar_list = init_ptree( PT_VARIABLE, $2, $6 );
             } else {
                 append_list_item( spc.svar_list, PT_VARIABLE, $2, $6 );
             }
             free( $2 );
           }
;

etransformula: SAFETY_OP tpropformula  {
                 spc.et_array_len++;
                 spc.env_trans_array = realloc( spc.env_trans_array,
                                                sizeof(ptree_t *)*spc.et_array_len );
                 if (spc.env_trans_array == NULL) {
                     perror( "gr1c_parse.y, etransformula, realloc" );
                     exit(-1);
                 }
                 *(spc.env_trans_array+spc.et_array_len-1) = gen_tree_ptr;
                 gen_tree_ptr = NULL;
               }
             | etransformula AND_SAFETY_OP tpropformula  {
                 spc.et_array_len++;
                 spc.env_trans_array = realloc( spc.env_trans_array,
                                                sizeof(ptree_t *)*spc.et_array_len );
                 if (spc.env_trans_array == NULL) {
                     perror( "gr1c_parse.y, etransformula, realloc" );
                     exit(-1);
                 }
                 *(spc.env_trans_array+spc.et_array_len-1) = gen_tree_ptr;
                 gen_tree_ptr = NULL;
               }
;

stransformula: SAFETY_OP tpropformula  {
                 spc.st_array_len++;
                 spc.sys_trans_array = realloc( spc.sys_trans_array,
                                                sizeof(ptree_t *)*spc.st_array_len );
                 if (spc.sys_trans_array == NULL) {
                     perror( "gr1c_parse.y, stransformula, realloc" );
                     exit(-1);
                 }
                 *(spc.sys_trans_array+spc.st_array_len-1) = gen_tree_ptr;
                 gen_tree_ptr = NULL;
               }
             | stransformula AND_SAFETY_OP tpropformula  {
                 spc.st_array_len++;
                 spc.sys_trans_array = realloc( spc.sys_trans_array,
                                                sizeof(ptree_t *)*spc.st_array_len );
                 if (spc.sys_trans_array == NULL) {
                     perror( "gr1c_parse.y, stransformula, realloc" );
                     exit(-1);
                 }
                 *(spc.sys_trans_array+spc.st_array_len-1) = gen_tree_ptr;
                 gen_tree_ptr = NULL;
               }
;

egoalformula: LIVENESS_OP propformula  {
                spc.num_egoals++;
                spc.env_goals = realloc( spc.env_goals,
                                         sizeof(ptree_t *)*spc.num_egoals );
                if (spc.env_goals == NULL) {
                    perror( "gr1c_parse.y, egoalformula, realloc" );
                    exit(-1);
                }
                *(spc.env_goals+spc.num_egoals-1) = gen_tree_ptr;
                gen_tree_ptr = NULL;
              }
            | egoalformula AND_LIVENESS_OP propformula  {
                spc.num_egoals++;
                spc.env_goals = realloc( spc.env_goals,
                                         sizeof(ptree_t *)*spc.num_egoals );
                if (spc.env_goals == NULL) {
                    perror( "gr1c_parse.y, egoalformula, realloc" );
                    exit(-1);
                }
                *(spc.env_goals+spc.num_egoals-1) = gen_tree_ptr;
                gen_tree_ptr = NULL;
              }
;

sgoalformula: EVENTUALLY_OP propformula  {
                spc.num_sgoals++;
                spc.sys_goals = realloc( spc.sys_goals,
                                         sizeof(ptree_t *)*spc.num_sgoals );
                if (spc.sys_goals == NULL) {
                    perror( "rg_parse.y, sgoalformula, realloc" );
                    exit(-1);
                }
                *(spc.sys_goals+spc.num_sgoals-1) = gen_tree_ptr;
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
