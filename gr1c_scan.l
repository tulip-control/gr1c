/* gr1c -- lexical analyzer (or "scanner")
 *
 * SCL; 26 Jan 2012.
 */

%{
#include "y.tab.h"
%}

%option bison-bridge bison-locations
%option noyywrap
%option warn nodefault


identifier  [a-zA-Z][a-zA-z0-9_\.]*
number  [0-9]+([.][0-9]+)?

%%

"->"  { return IMPLIES; }
"[]<>"  { return LIVENESS_OP; }
"[]"  { return SAFETY_OP; }

[()&|!='\n]  { return yytext[0]; }

"True"  { return TRUE_CONSTANT; }
"False"  { return FALSE_CONSTANT; }

^("ENV:") { return E_VARS; }
^("SYS:") { return S_VARS; }

^("ENVINIT:") { return E_INIT; }
^("ENVTRANS:") { return E_TRANS; }
^("ENVGOAL:") {return E_GOAL; }

^("SYSINIT:") { return S_INIT; }
^("SYSTRANS:") { return S_TRANS; }
^("SYSGOAL:") {return S_GOAL; }

{identifier}  { return VARIABLE; }
{number}  { return NUMBER; }

^("#").*  { return COMMENT; }

[ \t]+  {} /* Ignore whitespace */
