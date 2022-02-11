%{
/* This file is part of the balancer
 * Author: Dan Lovinger, del+@cmu.edu
 */

/*
 *        Copyright 1993 by Carnegie Mellon University
 * 
 *                    All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of CMU not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "y.tab.h"

#include <string.h>

#include "balutil.h"

extern char *infile;
unsigned lineno = 1;

%}

ws          [ \t]+
comment     #.*
string      [^ \t\n;]+
nl	    \n

%s A_NAME A_ARGS

%%

{comment} ;
{ws}      ;
{nl}	  { BEGIN INITIAL; lineno++; }

fs                 { return FS; }
cell               { return CELL; }
runtime            { return RUNTIME; }
procs              { return PROCS; }
transactions       { return TRANSACTIONS; }
voltransactions    { return VOLTRANSACTIONS; }
use_quota          { return UQUOTA; }
over_use_quota     { return OUQUOTA; }
agent              { BEGIN A_NAME; return AGENT; }

<A_NAME>{string} { BEGIN A_ARGS; 
		   yylval.string = strsav(yytext);
		   return AGENTNAME;
}

<A_ARGS>[^\n;]* { yylval.string = strsav(yytext);
                  return AGENTARGS;
}

[=;<>+\-]     { return yytext[0]; }

{string}  { yylval.string = strsav(yytext);
	    return STRING;
}

%%

void yyerror(text)
char *text;
{
    fail(FAIL_EXIT, "%s line %d: %s at symbol %s\n", infile, lineno, text, yytext);
}
