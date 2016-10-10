/* gr1c -- lexical analyzer (or "scanner")
 *
 * SCL; 2012-2015
 */

%top{
  #define _POSIX_C_SOURCE 200809L
  #include <string.h>
  #include <stdlib.h>
}
%{
  #include "y.tab.h"
  #define YY_USER_ACTION yylloc.last_column += yyleng;
%}

%option noyywrap
%option warn nodefault


identifier  [a-zA-Z_][a-zA-Z0-9_\.]*
number  [0-9]+([.][0-9]+)?

%%

"&"[ \t]*"[]<>"  { return AND_LIVENESS_OP; }
"&"[ \t]*"[]"  { return AND_SAFETY_OP; }

"[]<>"  { return LIVENESS_OP; }
"[]"  { return SAFETY_OP; }
"<>"  { return EVENTUALLY_OP; }

"<="  { return LE_OP; }  /* less than or equal to */
">="  { return GE_OP; }  /* greater than or equal to */
"!="  { return NOT_EQUALS; }

"<->"  { return EQUIV; }
"->"  { return IMPLIES; }
[()&|!=';,<>]  { return yytext[0]; }

\n  {
        yylloc.first_column = yylloc.last_column;
        yylloc.last_column = 0;
        yylloc.first_line = yylloc.last_line;
        ++(yylloc.last_line);
    }

"True"  { return TRUE_CONSTANT; }
"False"  { return FALSE_CONSTANT; }

^([ \t\r]*)("ENV:")  { return E_VARS; }
^([ \t\r]*)("SYS:")  { return S_VARS; }

^([ \t\r]*)("ENVINIT:")  { return E_INIT; }
^([ \t\r]*)("ENVTRANS:")  { return E_TRANS; }
^([ \t\r]*)("ENVGOAL:")  { return E_GOAL; }

^([ \t\r]*)("SYSINIT:")  { return S_INIT; }
^([ \t\r]*)("SYSTRANS:")  { return S_TRANS; }
^([ \t\r]*)("SYSGOAL:")  { return S_GOAL; }

{identifier}  { yylval.str = strdup( yytext ); return VARIABLE; }
{number}  { yylval.num = strtol( yytext, NULL, 10 ); return NUMBER; }

"#".*"\n"  { unput( '\n' ); }  /* Comments.
                                  unput is used to preserve line count. */

[ \t\r]+  {}  /* Ignore whitespace besides \n */

.  { return yytext[0]; }  /* Pass anything unknown to parser, as-is,
                             hence a syntax error. */
