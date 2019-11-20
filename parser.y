%{

/* Copyright 2009-2019
 * Kaz Kylheku <kaz@kylheku.com>
 * Vancouver, Canada
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <wchar.h>
#include <signal.h>
#include "config.h"
#include "alloca.h"
#include "lib.h"
#include "signal.h"
#include "unwind.h"
#include "regex.h"
#include "match.h"
#include "hash.h"
#include "struct.h"
#include "eval.h"
#include "tree.h"
#include "y.tab.h"
#include "gc.h"
#include "debug.h"
#include "txr.h"
#include "itypes.h"
#include "buf.h"
#include "parser.h"

static val sym_helper(parser_t *parser, wchar_t *lexeme, val meta_allowed);
static val repeat_rep_helper(val sym, val args, val main, val parts);
static void process_catch_exprs(val exprs);
static val define_transform(parser_t *parser, val define_form);
static val optimize_text(val text_form);
static val unquotes_occur(val quoted_form, int level);
static val rlrec(parser_t *, val form, val line);
static val rlcp_parser(parser_t *parser, val to, val from);
static wchar_t char_from_name(const wchar_t *name);
static val make_expr(parser_t *, val sym, val rest, val lineno);
static val check_parse_time_action(val spec_rev);
static void misplaced_consing_dot_check(scanner_t *scanner, val term_atom_cons);
static val uref_helper(parser_t *, val expr);
static val uoref_helper(parser_t *, val expr);
static val qref_helper(parser_t *, val lexpr, val rexpr);
static val fname_helper(parser_t *, val name);

#if YYBISON
union YYSTYPE;
int yylex(union YYSTYPE *, yyscan_t scanner);
int yyparse(scanner_t *, parser_t *);
#endif

#define rl(form, line) rlrec(parser, form, line)
#define rlc(to, from) rlcp_parser(parser, to, from)
#define mkexp(sym, rest, lineno) make_expr(parser, sym, rest, lineno)
#define symhlpr(lexeme, meta_allowed) sym_helper(parser, lexeme, meta_allowed)
#define yyerr(msg) yyerror(scnr, parser, msg)
#define yybadtok(tok, context) yybadtoken(parser, tok, context)
#define ifnign(expr) (parser->ignore ? nil : (expr))

INLINE val expand_forms_ver(val forms, int ver)
{
  if (!opt_compat || opt_compat >= ver)
    return expand_forms(forms, nil);
  return forms;
}

INLINE val expand_form_ver(val form, int ver)
{
  if (!opt_compat || opt_compat >= ver)
    return expand(form, nil);
  return form;
}

%}

%pure-parser
%parse-param{scanner_t *scnr}
%parse-param{parser_t *parser}
%lex-param{yyscan_t scnr}

%union {
  wchar_t *lexeme;
  union obj *val;
  wchar_t chr;
  cnum lineno;
}

%token <lexeme> SPACE TEXT SYMTOK
%token <lineno> ALL SOME NONE MAYBE CASES BLOCK CHOOSE GATHER
%token <lineno> AND OR END COLLECT
%token <lineno> UNTIL COLL OUTPUT REPEAT REP SINGLE FIRST LAST EMPTY
%token <lineno> MOD MODLAST DEFINE TRY CATCH FINALLY IF
%token <lineno> ERRTOK /* deliberately not used in grammar */
%token <lineno> HASH_BACKSLASH HASH_SLASH DOTDOT HASH_H HASH_S HASH_R HASH_SEMI
%token <lineno> HASH_B_QUOTE HASH_N HASH_T
%token <lineno> WORDS WSPLICE QWORDS QWSPLICE
%token <lineno> SECRET_ESCAPE_R SECRET_ESCAPE_E SECRET_ESCAPE_I
%token <lineno> OLD_DOTDOT

%token <val> NUMBER METANUM
%token <val> HASH_N_EQUALS HASH_N_HASH

%token <chr> REGCHAR REGTOKEN LITCHAR SPLICE
%token <chr> CONSDOT LAMBDOT UREFDOT OREFDOT UOREFDOT

%type <val> spec hash_semi_or_n_expr hash_semi_or_i_expr
%type <val> clauses_rev clauses_opt clause
%type <val> all_clause some_clause none_clause maybe_clause block_clause
%type <val> cases_clause choose_clause gather_clause collect_clause until_last
%type <val> collect_repeat
%type <val> clause_parts additional_parts gather_parts additional_gather_parts
%type <val> output_clause define_clause try_clause catch_clauses_opt
%type <val> if_clause elif_clauses_opt else_clause_opt
%type <val> line elems_opt elems clause_parts_h additional_parts_h
%type <val> text texts elem var var_op modifiers
%type <val> vector hash struct range tnode tree
%type <val> exprs exprs_opt n_exprs r_exprs i_expr i_dot_expr
%type <val> n_expr n_exprs_opt n_dot_expr
%type <val> list dwim meta compound
%type <val> out_clauses out_clauses_opt out_clause
%type <val> repeat_clause repeat_parts_opt o_line
%type <val> o_elems_opt o_elems o_elem o_var q_var rep_elem rep_parts_opt
%type <val> regex lisp_regex regexpr regbranch
%type <val> regterm regtoken regclass regclassterm regrange
%type <val> strlit chrlit quasilit quasi_items quasi_item litchars restlitchar
%type <val> wordslit wordsqlit buflit buflit_items buflit_item not_a_clause
%type <chr> regchar
%type <val> byacc_fool
%type <lineno> '(' '[' '@'

%nonassoc LOW /* used for precedence assertion */
%right SYMTOK '{' '}'
%right ALL SOME NONE MAYBE CASES CHOOSE AND OR END COLLECT UNTIL COLL
%right OUTPUT REPEAT REP FIRST LAST EMPTY DEFINE IF ELIF ELSE
%right SPACE TEXT NUMBER METANUM HASH_N_EQUALS HASH_N_HASH HASH_B_QUOTE
%nonassoc '[' ']' '(' ')'
%left '-' ',' '\'' '^' SPLICE '@'
%left '|' '/'
%left '&'
%right '~' '*' '?' '+' '%'
%right DOTDOT
%right '.' CONSDOT LAMBDOT UREFDOT OREFDOT UOREFDOT REGCHAR REGTOKEN LITCHAR
%right OLD_DOTDOT

%%

spec : clauses_opt              { parser->syntax_tree = $1; }
     | SECRET_ESCAPE_R regexpr  { parser->syntax_tree = $2; end_of_regex(scnr); }
     | SECRET_ESCAPE_E hash_semi_or_n_expr
                                { parser->syntax_tree = $2; YYACCEPT; }
       byacc_fool               { internal_error("notreached"); }
     | SECRET_ESCAPE_I hash_semi_or_i_expr
                                { parser->syntax_tree = $2; YYACCEPT; }
       byacc_fool               { internal_error("notreached"); }
     | SECRET_ESCAPE_E          { if (yychar == YYEOF) {
                                    parser->syntax_tree = nao;
                                    YYACCEPT;
                                  } else {
                                    yybadtok(yychar, nil);
                                    parser->syntax_tree = nil;
                                  } }
     | SECRET_ESCAPE_I          { if (yychar == YYEOF) {
                                    parser->syntax_tree = nao;
                                    YYACCEPT;
                                  } else {
                                    yybadtok(yychar, nil);
                                    parser->syntax_tree = nil;
                                  } }
     | error '\n'               { parser->syntax_tree = nil;
                                  if (parser->errors >= 8)
                                    YYABORT;
                                  yyerrok;
                                  yybadtok(yychar, nil); }

     ;


hash_semi_or_n_expr : HASH_SEMI                 { parser->ignore = 1; }
                      n_expr                    { parser->ignore = 0;
                                                  $$ = nao; }
                    | HASH_SEMI '.'             { parser->ignore = 1; }
                      n_expr                    { parser->ignore = 0;
                                                  $$ = nao; }
                    | HASH_SEMI OREFDOT         { parser->ignore = 1; }
                      n_expr                    { parser->ignore = 0;
                                                  $$ = nao; }
                    | n_expr                    { $$ = $1; }
                    | '.' n_expr                { $$ = uref_helper(parser, $2); }
                    | OREFDOT n_expr            { $$ = uoref_helper(parser, $2); }
                    ;

hash_semi_or_i_expr : HASH_SEMI                 { parser->ignore = 1; }
                      i_expr                    { parser->ignore = 0;
                                                  $$ = nao; }
                    | HASH_SEMI '.'             { parser->ignore = 1; }
                      i_expr                    { parser->ignore = 0;
                                                  $$ = nao; }
                    | HASH_SEMI OREFDOT         { parser->ignore = 1; }
                      i_expr                    { parser->ignore = 0;
                                                  $$ = nao; }
                    | i_expr                    { $$ = $1; }
                    | '.' i_expr                { $$ = uref_helper(parser, $2); }
                    | OREFDOT i_expr            { $$ = uoref_helper(parser, $2); }
                    ;


/* Hack needed for Berkeley Yacc */
byacc_fool : n_expr { internal_error("notreached"); }
           | HASH_SEMI { internal_error("notreached"); }
           | { internal_error("notreached"); }
           ;

clauses_rev : clause                    { $$ = check_parse_time_action(cons($1, nil)); }
            | clauses_rev clause        { $$ = check_parse_time_action(cons($2, $1));  }
            ;

clauses_opt : clauses_rev       { $$ = us_nreverse($1); }
            | /* empty */       { $$ = nil; }
            ;

clause : all_clause             { $$ = cons($1, nil); rlc($$, $1); }
       | some_clause            { $$ = cons($1, nil); rlc($$, $1); }
       | none_clause            { $$ = cons($1, nil); rlc($$, $1); }
       | maybe_clause           { $$ = cons($1, nil); rlc($$, $1); }
       | cases_clause           { $$ = cons($1, nil); rlc($$, $1); }
       | block_clause           { $$ = cons($1, nil); rlc($$, $1); }
       | choose_clause          { $$ = cons($1, nil); rlc($$, $1); }
       | collect_clause         { $$ = cons($1, nil); rlc($$, $1); }
       | gather_clause          { $$ = cons($1, nil); rlc($$, $1); }
       | define_clause          { $$ = list(define_transform(parser, $1), nao);
                                  rlc(car($$), $1);
                                  rlc($$, $1); }
       | try_clause             { $$ = cons($1, nil); rlc($$, $1); }
       | if_clause              { $$ = cons($1, nil); rlc($$, $1); }
       | output_clause          { $$ = cons($1, nil); rlc($$, $1); }
       | line                   { $$ = $1; }
       ;

all_clause : ALL newl clause_parts      { $$ = list(all_s, $3, nao);
                                          rl($$, num($1)); }
           | ALL newl error             { $$ = nil;
                                          yybadtok(yychar, lit("all clause")); }
           ;

some_clause : SOME exprs_opt ')'
              newl clause_parts         { $$ = list(some_s, $5, $2, nao);
                                          rl($$, num($1)); }
            | SOME exprs_opt ')'
              newl error
                                        { $$ = nil;
                                          yybadtok(yychar, lit("some clause")); }
            ;

none_clause : NONE newl clause_parts    { $$ = list(none_s, $3, nao);
                                          rl($$, num($1)); }
            | NONE newl error           { $$ = nil;
                                          yybadtok(yychar, lit("none clause")); }
            ;

maybe_clause : MAYBE newl clause_parts  { $$ = list(maybe_s, $3, nao);
                                          rl($$, num($1)); }
             | MAYBE newl error         { $$ = nil;
                                          yybadtok(yychar, lit("maybe clause")); }
             ;

cases_clause : CASES newl clause_parts  { $$ = list(cases_s, $3, nao);
                                          rl($$, num($1)); }
             | CASES newl error         { $$ = nil;
                                          yybadtok(yychar, lit("cases clause")); }
             ;

block_clause  : BLOCK exprs_opt ')'
                newl clauses_opt
                END newl                { val name = first($2);
                                          if (gt(length($2), one))
                                            yyerr("block: takes zero or no arguments");
                                          if (name && !bindable(name))
                                            yyerrorf(scnr,
                                                     lit("block: ~s is not a bindable symbol"),
                                                     name, nao);
                                          $$ = list(block_s, name, $5, nao);
                                          rl($$, num($1)); }
              | BLOCK exprs_opt ')'
                newl error              { $$ = nil;
                                          yybadtok(yychar, lit("block clause")); }
              ;

choose_clause : CHOOSE exprs_opt ')'
                newl clause_parts       { $$ = list(choose_s, $5, $2, nao);
                                          rl($$, num($1)); }
              | CHOOSE exprs_opt ')'
                newl error              { $$ = nil;
                                          yybadtok(yychar, lit("choose clause")); }
              ;

gather_clause : GATHER exprs_opt ')'
                newl gather_parts
                END newl                { val args = match_expand_keyword_args($2);
                                          $$ = list(gather_s,
                                                    append2(mapcar(curry_12_1(func_n2(cons), nil),
                                                                   first($5)), rest($5)),
                                                    args, nao);
                                          rl($$, num($1)); }

              | GATHER exprs_opt ')'
                newl gather_parts
                until_last exprs_opt ')' newl
                clauses_opt
                END newl                { val args = match_expand_keyword_args($2);
                                          $$ = list(gather_s,
                                                    append2(mapcar(curry_12_1(func_n2(cons), nil),
                                                                   first($5)), rest($5)),
                                                    args, cons(cdr($6),
                                                             cons($7, $10)), nao);
                                          rl($$, num($1)); }

              | GATHER exprs_opt ')'
                newl error              { $$ = nil;
                                          yybadtok(yychar, lit("gather clause")); }
              ;

gather_parts : clauses_opt additional_gather_parts
                                        { $$ = if2($1, cons($1, $2)); }
             ;

additional_gather_parts : AND newl gather_parts { $$ = $3; }
                        | OR newl gather_parts  { $$ = $3; }
                        | /* empty */           { $$ = nil; }
                        ;

collect_clause : collect_repeat exprs_opt ')' newl
                 clauses_opt END newl     { val args = match_expand_keyword_args($2);
                                            $$ = list(car($1),
                                                      $5, nil, args,
                                                      nao);
                                            rl($$, cdr($1)); }
               | collect_repeat exprs_opt ')'
                 newl clauses_opt until_last exprs_opt ')'
                 newl clauses_opt END newl
                                          { val args = match_expand_keyword_args($2);
                                            if (nilp($10))
                                              yyerr("empty until/last in collect");
                                            $$ = list(car($1), $5,
                                                      cons(cdr($6),
                                                           cons($7, $10)),
                                                      args, nao);
                                            rl($$, cdr($1));
                                            rl($10, car($6)); }
               | collect_repeat exprs_opt ')'
                 newl error             { $$ = nil;
                                          yybadtok(yychar, lit("collect clause")); }
               ;

collect_repeat : COLLECT { $$ = cons(collect_s, num($1)); }
               | REPEAT { $$ = cons(repeat_s, num($1)); }
               ;

until_last : UNTIL { $$ = cons(num($1), until_s); }
           | LAST  { $$ = cons(num($1), last_s); }
           ;

clause_parts : clauses_opt additional_parts     { $$ = if2($1, cons($1, $2)); }
             ;

additional_parts : END newl                     { $$ = nil; }
                 | AND newl clause_parts        { $$ = $3; }
                 | OR newl clause_parts         { $$ = $3; }
                 ;

if_clause : IF n_exprs_opt ')'
            newl clauses_opt
            elif_clauses_opt
            else_clause_opt
            END newl            { if (opt_compat && opt_compat <= 136)
                                  { val xexp = expand_meta($2, nil);
                                    val req = rlc(cons(require_s, xexp), $2);
                                    val iff = rlc(cons(cons(cons(req, nil), $5), nil), $2);
                                    val elifs = $6;
                                    val els = cons($7, nil);
                                    val cases = nappend2(nappend2(iff, elifs), els);
                                    $$ = list(cases_s, cases, nao); }
                                  else
                                  { val expr = expand(car($2), nil);
                                    val ifs = $5;
                                    val branch = cons(cons(expr, ifs), nil);
                                    val elifs = $6;
                                    val els = $7;
                                    if (cdr($2))
                                      yyerr("extra expression in if");
                                    $$ = cons(if_s,
                                              nappend2(branch, nappend2(elifs, els)));
                                    rl($$, num($1)); } }
          | IF n_exprs_opt ')'
            newl error          { $$ = nil; yybadtok(yychar, lit("if clause")); }
          ;

elif_clauses_opt : ELIF n_exprs_opt ')' newl
                   clauses_opt
                   elif_clauses_opt  { if (opt_compat && opt_compat <= 136)
                                       { val xexp = expand_meta($2, nil);
                                         val req = rlc(cons(require_s, xexp), $2);
                                         $$ = cons(cons(cons(req, nil), $5), $6); }
                                       else
                                       { val expr = expand(car($2), nil);
                                         val elifs = $5;
                                         val branch = cons(cons(expr, elifs), nil);
                                         if (cdr($2))
                                           yyerr("extra expression in elif");
                                         $$ = nappend2(branch, $6); } }
                 |                   { $$ = nil; }
                 ;

else_clause_opt : ELSE newl
                  clauses_opt        { if (opt_compat && opt_compat <= 136)
                                       { $$ = $3; }
                                       else
                                       { $$ = cons(cons(t, $3), nil); } }
                  |                  { $$ = nil; }
                  ;

line : elems_opt '\n'           { $$ = $1; }
     ;

elems_opt : elems               { $$ = $1; }
          |                     { $$ = nil; }
          ;

elems : elem                    { $$ = cons($1, nil);
                                  rlc($$, $1); }
      | elem elems              { $$ = cons($1, $2);
                                  rlc($$, $1); }
      ;


text : TEXT                     { $$ = rl(string_own($1), num(parser->lineno)); }
     | SPACE                    { if ($1[0] == ' ' && $1[1] == 0)
                                  { val spaces = list(oneplus_s,
                                                      chr(' '), nao);
                                    free($1);
                                    $$ = regex_compile(spaces, nil);
                                    rl($$, num(parser->lineno)); }
                                  else
                                  { $$ = rl(string_own($1), num(parser->lineno)); }}
     | regex                    { $$ = $1;
                                  rl($$, num(parser->lineno)); }
     | EMPTY                    { $$ = null_string; }
     ;

texts : text %prec LOW          { $$ = rlc(cons($1, nil), $1); }
      | text texts              { $$ = rlc(cons($1, $2), $2); }
      ;

elem : texts                    { $$ = rlc(cons(text_s, $1), $1);
                                  $$ = rlc(optimize_text($$), $$); }
     | var                      { $$ = rl($1, num(parser->lineno));
                                  match_reg_elem($$); }
     | list                     { val sym = first($1);
                                  if (sym ==  do_s || sym == require_s)
                                    $$ = rlc(cons(sym,
                                                   expand_forms(rest($1), nil)),
                                              $1);
                                  else
                                  { $$ = match_expand_elem($1);
                                    match_reg_elem($$); } }
     | COLL exprs_opt ')' elems_opt END { val args = match_expand_keyword_args($2);
                                          $$ = list(coll_s, $4, nil, args, nao);
                                          rl($$, num($1)); }
     | COLL exprs_opt ')' elems_opt
       until_last exprs_opt ')'
       elems_opt END            { val args = match_expand_keyword_args($2);
                                  $$ = list(coll_s, $4, cons(cdr($5),
                                                             cons($6, $8)),
                                            args, nao);
                                  rl($$, num($1));
                                  rl($6, car($5)); }
     | REP exprs_opt ')' elems END     { val args = match_expand_keyword_args($2);
                                         $$ = list(rep_s, $4, nil, args, nao);
                                         rl($$, num($1)); }
     | REP exprs_opt ')' elems
       until_last exprs_opt ')'
       elems END
                                { val args = match_expand_keyword_args($2);
                                  $$ = list(rep_s, $4, cons(cdr($5),
                                                            cons($6, $8)),
                                            args, nao);
                                  rl($$, num($1));
                                  rl($6, car($5)); }
     | BLOCK exprs_opt ')' elems END { $$ = list(block_s, car($2),
                                                 cons($4, nil), nao);
                                       rl($$, num($1)); }
     | COLL error               { $$ = nil;
                                  yybadtok(yychar, lit("coll clause")); }
     | REP error               { $$ = nil;
                                 yybadtok(yychar, lit("rep clause")); }
     | BLOCK error             { $$ = nil;
                                 yybadtok(yychar, lit("rep clause")); }
     | ALL clause_parts_h       { $$ = rl(list(all_s, t, $2, nao), num($1)); }
     | SOME exprs_opt ')'
       clause_parts_h           { $$ = rl(list(some_s, t, $4, $2, nao), num($1)); }
     | NONE clause_parts_h      { $$ = rl(list(none_s, t, $2, nao), num($1)); }
     | MAYBE clause_parts_h     { $$ = rl(list(maybe_s, t, $2, nao), num($1)); }
     | CASES clause_parts_h     { $$ = rl(list(cases_s, t, $2, nao), num($1)); }
     | CHOOSE exprs_opt ')'
       clause_parts_h           { $$ = list(choose_s, t, $4, $2, nao);
                                  rl($$, num($1)); }
     | DEFINE exprs ')' elems END
                                { $$ = list(define_s, t, $4, $2, nao);
                                  rl($$, num($1));
                                  match_reg_params(second($2)); }
     ;

clause_parts_h : elems_opt additional_parts_h   { $$ = if2($1, cons($1, $2)); }
               ;

additional_parts_h : END                        { $$ = nil; }
                   | AND clause_parts_h         { $$ = $2; }
                   | OR clause_parts_h          { $$ = $2; }
                   ;

define_clause : DEFINE exprs ')' newl
                clauses_opt
                END newl        { $$ = list(define_s, $2, $5, nao);
                                  rl($$, num($1));
                                  match_reg_params(second($2)); }
              | DEFINE ')' newl
                clauses_opt
                END newl        { $$ = list(define_s, nil, $4, nao);
                                  rl($$, num($1)); }
              | DEFINE error    { $$ = nil;
                                  yybadtok(yychar, lit("define directive")); }
              | DEFINE exprs ')' newl
                error           { $$ = nil; yybadtok(yychar, lit("define")); }
              | DEFINE ')' newl
                error           { $$ = nil;
                                  yybadtok(yychar, lit("define")); }
              ;

try_clause : TRY newl
             clauses_opt
             catch_clauses_opt
             END newl           { $$ = list(try_s,
                                            flatten(mapcar(func_n1(second),
                                                           $4)),
                                            $3, $4, nao);
                                  rl($$, num($1)); }
           | TRY newl
             error              { $$ = nil;
                                  yybadtok(yychar, lit("try clause")); }
           ;

catch_clauses_opt : CATCH ')' newl
                    clauses_opt
                    catch_clauses_opt   { $$ = cons(list(catch_s, cons(t, nil),
                                                         $4, nao), $5);
                                          rl($$, num($1)); }
                  | CATCH exprs ')' newl
                    clauses_opt
                    catch_clauses_opt   { $$ = cons(list(catch_s, $2, $5, nao),
                                                    $6);
                                          process_catch_exprs($2);
                                          rl($$, num($1)); }
                  | FINALLY newl
                    clauses_opt         { $$ = cons(list(finally_s, nil,
                                                         $3, nao),
                                                    nil);
                                          rl($$, num($1)); }
                  |                     { $$ = nil; }
                  | CATCH ')' newl
                    error               { $$ = nil;
                                          yybadtok(yychar, lit("catch clause")); }
                  | CATCH exprs ')' newl
                    error               { $$ = nil;
                                          yybadtok(yychar, lit("catch clause")); }
                  | FINALLY newl
                    error               { $$ = nil;
                                          yybadtok(yychar, lit("finally clause")); }
                  ;


output_clause : OUTPUT ')' o_elems '\n'
                out_clauses
                END newl        { $$ = nil;
                                  yyerr("obsolete output syntax: trailing material"); }
              | OUTPUT ')' newl
                END newl        { $$ = rl(list(output_s, nao), num($1)); }
              | OUTPUT ')' newl
                out_clauses
                END newl        { $$ = rl(list(output_s, $4, nao), num($1)); }
              | OUTPUT exprs ')' newl
                out_clauses
                END newl        { cons_bind (dest, rest, $2);
                                  val dest_ex = expand_form_ver(dest, 166);
                                  val args = if3(dest_ex == dest,
                                                 $2, cons(dest_ex, rest));
                                  $$ = list(output_s, $5, args, nao);
                                  rl($$, num($1));
                                  { val into_var = second(memql(into_k, args));
                                    val named_var = second(memql(named_k, args));
                                    match_reg_var(into_var);
                                    match_reg_var(named_var); } }
              | OUTPUT exprs ')' o_elems '\n'
                out_clauses
                END newl        { $$ = nil;
                                  yyerr("invalid combination of old and "
                                               "new syntax in output directive"); }
              | OUTPUT error    { $$ = nil;
                                  yybadtok(yychar, lit("output directive")); }
              | OUTPUT ')' o_elems '\n'
                error           { $$ = nil;
                                  yybadtok(yychar, lit("output clause")); }
              | OUTPUT ')' newl
                error           { $$ = nil;
                                  yybadtok(yychar, lit("output clause")); }
              | OUTPUT exprs ')' o_elems '\n'
                error           { $$ = nil;
                                  yybadtok(yychar, lit("output clause")); }
              | OUTPUT exprs ')' newl
                error           { $$ = nil;
                                  yybadtok(yychar, lit("output clause")); }
              ;

out_clauses : out_clause                { $$ = cons($1, nil); }
            | out_clause out_clauses    { $$ = cons($1, $2);  }
            ;

out_clause : repeat_clause              { $$ = cons($1, nil); }
           | o_line                     { $$ = $1; }
           ;

repeat_clause : REPEAT n_exprs_opt ')' newl
                out_clauses_opt
                repeat_parts_opt
                END newl                { $$ = repeat_rep_helper(repeat_s,
                                                                 $2, $5, $6);
                                          rl($$, num($1)); }
              | REPEAT newl
                error           { $$ = nil;
                                  yybadtok(yychar, lit("repeat clause")); }
              ;

repeat_parts_opt : SINGLE newl
                   out_clauses_opt
                   repeat_parts_opt     { $$ = cons(cons(single_s, $3), $4);
                                          rl($$, num($1)); }
                 | FIRST newl
                   out_clauses_opt
                   repeat_parts_opt     { $$ = cons(cons(first_s, $3), $4);
                                          rl($$, num($1)); }
                 | LAST exprs_opt ')' newl
                   out_clauses_opt
                   repeat_parts_opt     { if ($2)
                                            yyerrorf(scnr,
                                                     lit("last: in output, "
                                                         "takes no arguments"),
                                                     nao);
                                          $$ = cons(cons(last_s, $5), $6);
                                          rl($$, num($1)); }
                 | EMPTY newl
                   out_clauses_opt
                   repeat_parts_opt     { $$ = cons(cons(empty_s, $3), $4);
                                          rl($$, num($1)); }
                 | MOD exprs_opt ')'
                   newl
                   out_clauses_opt
                   repeat_parts_opt     { $$ = cons(cons(mod_s,
                                                         cons(expand_forms_ver($2, 166),
                                                              $5)), $6);
                                          rl($$, num($1)); }
                 | MODLAST exprs_opt ')'
                   newl
                   out_clauses_opt
                   repeat_parts_opt     { $$ = cons(cons(modlast_s,
                                                         cons(expand_forms_ver($2, 166),
                                                              $5)), $6);
                                          rl($$, num($1)); }
                 | /* empty */          { $$ = nil; }
                 ;


out_clauses_opt : out_clauses   { $$ = $1; }
                | /* empty */   { $$ = nil; }

o_line : o_elems_opt '\n'       { $$ = $1; }
       ;

o_elems_opt : o_elems           { $$ = $1;
                                  rl($$, num(parser->lineno)); }
            |                   { $$ = nil; }
            ;

o_elems : o_elem                { $$ = cons($1, nil); }
        | o_elem o_elems        { $$ = cons($1, $2); }
        | not_a_clause          { $$ = cons($1, nil); }
        | not_a_clause o_elems  { $$ = cons($1, $2); }
        ;

o_elem : TEXT                   { $$ = string_own($1);
                                  rl($$, num(parser->lineno)); }
       | SPACE                  { $$ = string_own($1);
                                  rl($$, num(parser->lineno)); }
       | o_var                  { $$ = $1; }
       | compound               { $$ = rlc(list(expr_s,
                                                 expand($1, nil), nao), $1); }
       | rep_elem               { $$ = $1; }
       ;

rep_elem : REP n_exprs_opt ')' o_elems_opt
           rep_parts_opt END    { $$ = repeat_rep_helper(rep_s, $2, $4, $5);
                                  rl($$, num($1)); }
         | REP error            { $$ = nil;
                                  yybadtok(yychar, lit("rep clause")); }
         ;

rep_parts_opt : SINGLE o_elems_opt
                rep_parts_opt           { $$ = cons(cons(single_s, $2), $3);
                                          rl($$, num($1)); }
              | FIRST o_elems_opt
                rep_parts_opt           { $$ = cons(cons(first_s, $2), $3);
                                          rl($$, num($1)); }
              | LAST exprs_opt ')'
                o_elems_opt rep_parts_opt { if ($2)
                                            yyerrorf(scnr,
                                                     lit("last: in output, "
                                                         "takes no arguments"),
                                                     nao);
                                          $$ = cons(cons(last_s, $4), $5);
                                          rl($$, num($1)); }
              | EMPTY o_elems_opt
                rep_parts_opt           { $$ = cons(cons(empty_s, $2), $3);
                                          rl($$, num($1)); }
              | MOD exprs_opt ')'
                o_elems_opt
                rep_parts_opt           { $$ = cons(cons(mod_s,
                                                         cons(expand_forms_ver($2, 166),
                                                              $4)), $5);
                                          rl($$, num($1)); }
              | MODLAST exprs_opt ')'
                o_elems_opt
                rep_parts_opt           { $$ = cons(cons(modlast_s,
                                                         cons(expand_forms_ver($2, 166),
                                                              $4)), $5);
                                          rl($$, num($1)); }
              | /* empty */             { $$ = nil; }
              ;


/* This sucks, but factoring '*' into a nonterminal
 * that generates an empty phrase causes reduce/reduce conflicts.
 */
var : SYMTOK                    { $$ = list(var_s, symhlpr($1, nil), nao); }
    | '{' SYMTOK '}'            { $$ = list(var_s, symhlpr($2, nil), nao); }
    | '{' SYMTOK modifiers '}'  { $$ = list(var_s, symhlpr($2, nil), $3, nao); }
    | var_op SYMTOK             { $$ = list(var_s, symhlpr($2, nil), $1, nao); }
    | var_op '{' SYMTOK '}'     { $$ = list(var_s, symhlpr($3, nil), $1, nao); }
    | var_op '{' SYMTOK regex '}'       { $$ = nil;
                                          yyerr("longest match "
                                                "not useable with regex"); }
    | var_op '{' SYMTOK NUMBER '}'      { $$ = nil;
                                          yyerr("longest match "
                                                "not useable with "
                                                "fixed width match"); }
    | SYMTOK error              { $$ = nil;
                                  yybadtok(yychar, lit("variable spec")); }
    | var_op error              { $$ = nil;
                                  yybadtok(yychar, lit("variable spec")); }
    ;

var_op : '*'                    { $$ = list(t, nao); }
       ;

modifiers : NUMBER              { $$ = cons($1, nil); }
          | regex               { $$ = cons($1, nil);
                                  rlc($$, $1); }
          | compound            { $$ = rlc(cons(expand_meta($1, nil),
                                                 nil), $1); }
          | SYMTOK              { $$ = cons(symhlpr($1, nil), nil); }
          ;

o_var : SYMTOK                  { val expr = symhlpr($1, nil);
                                  if (!opt_compat || opt_compat > 128)
                                    expr = expand(expr, nil);
                                  $$ = list(var_s, expr, nao);
                                  rl($$, num(parser->lineno)); }
      | '{' n_expr n_exprs_opt '}'
                                { if (opt_compat && opt_compat <= 128)
                                  { $$ = list(var_s,
                                              expand_meta($2, nil),
                                              expand_meta($3, nil), nao); }
                                  else
                                  { val quasi_var = list(var_s, $2, $3, nao);
                                    val quasi_items = cons(quasi_var, nil);
                                    $$ = car(expand_quasi(quasi_items, nil)); } }
      | SYMTOK error            { $$ = nil;
                                  yybadtok(yychar, lit("variable spec")); }
      ;

q_var : '@' '{' n_expr n_exprs_opt '}'
                                { $$ = list(var_s, $3, $4, nao);
                                  rl($$, num(parser->lineno)); }
      | '@' '{' error           { $$ = nil;
                                  yybadtok(yychar, lit("variable spec")); }
      ;


vector : '#' list               { if (parser->quasi_level > 0 && unquotes_occur($2, 0))
                                    $$ = rlc(cons(vector_lit_s,
                                                   cons($2, nil)), $2);
                                  else
                                    $$ = rlc(vec_list($2), $2); }
       | '#' error              { $$ = nil;
                                  yybadtok(yychar, lit("unassigned/reserved # notation")); }
       ;

hash : HASH_H list              { if (parser->quasi_level > 0 && unquotes_occur($2, 0))
                                    $$ = rl(cons(hash_lit_s, $2), num($1));
                                  else
                                    $$ = rl(hash_construct(first($2),
                                                             rest($2)),
                                            num($1)); }
     | HASH_H error              { $$ = nil;
                                    yybadtok(yychar, lit("hash literal")); }
     ;

struct : HASH_S list            { if (parser->quasi_level > 0 && unquotes_occur($2, 0))
                                    $$ = rl(cons(struct_lit_s, $2),
                                              num($1));
                                  else
                                  { val strct = make_struct_lit(first($2),
                                                                rest($2));
                                    $$ = rl(strct, num($1)); } }
       | HASH_S error           { $$ = nil;
                                    yybadtok(yychar, lit("struct literal")); }
       ;

range : HASH_R list             { if (length($2) != two)
                                    yyerr("range literal needs two elements");
                                  { val range = rcons(first($2), second($2));
                                    $$ = rl(range, num($1)); } }
      | HASH_R error            { $$ = nil;
                                  yybadtok(yychar, lit("range literal")); }
      ;

tnode : HASH_N list             { if (gt(length($2), three))
                                    yyerr("excess elements in tree node");
                                  { val tn = tnode(first($2), second($2),
                                                   third($2));
                                    $$ = rl(tn, num($1)); } }
      | HASH_N error            { $$ = nil;
                                  yybadtok(yychar, lit("tree node literal")); }
      ;

tree : HASH_T list             { if (parser->quasi_level > 0 && unquotes_occur($2, 0))
                                   $$ = rl(cons(tree_lit_s, $2), num($1));
                                 else
                                 { val opts = first($2);
                                   val key_fn_name = pop(&opts);
                                   val less_fn_name = pop(&opts);
                                   val equal_fn_name = pop(&opts);
                                   val key_fn = fname_helper(parser, key_fn_name);
                                   val less_fn = fname_helper(parser, less_fn_name);
                                   val equal_fn = fname_helper(parser, equal_fn_name);
                                   val tr = tree(rest($2), key_fn,
                                                 less_fn, equal_fn);
                                     $$ = rl(tr, num($1)); } }
     | HASH_T error            { $$ = nil;
                                 yybadtok(yychar, lit("tree node literal")); }
     ;

list : '(' n_exprs ')'          { $$ = rl($2, num($1)); }
     | '(' '.' n_exprs ')'      { val a = car($3);
                                  val ur = uref_helper(parser, a);
                                  if (ur == a)
                                    $$ = $3;
                                  else
                                    $$ = rlc(cons(ur, cdr($3)), ur); }
     | '(' ')'                  { $$ = nil; }
     | '(' LAMBDOT n_expr ')'   { $$ = $3; }
     | '(' CONSDOT n_expr ')'   { $$ = $3; }
     | '(' error                { $$ = nil;
                                  yybadtok(yychar, lit("expression")); }
     ;

meta : '@' n_expr               { if (consp($2))
                                    $$ = rl(cons(expr_s, cons($2, nil)), num($1));
                                  else
                                    $$ = rl(cons(var_s, cons($2, nil)),
                                            num($1)); }
     | '@' error                { $$ = nil;
                                  yybadtok(yychar, lit("meta expression")); }
     ;

dwim : '[' '.' n_exprs ']'      { val a = car($3);
                                  val ur = uref_helper(parser, a);
                                  $$ = rlcp_tree(cons(dwim_s,
                                                      cons(ur, cdr($3))), ur); }
     | '[' n_exprs ']'          { $$ = rl(cons(dwim_s, $2), num($1)); }
     | '[' ']'                  { $$ = rl(cons(dwim_s, nil), num($1)); }
     | '[' error                { $$ = nil;
                                  yybadtok(yychar, lit("DWIM expression")); }
     ;

compound : list
         | dwim
         | meta
         ;

exprs : n_exprs                 { $$ = rlc(expand_meta($1, nil), $1); }
      ;

exprs_opt : exprs               { $$ = $1; }
          | /* empty */         { $$ = nil; }
          ;

n_exprs : r_exprs               { val term_atom = pop(&$1);
                                  val tail_cons = $1;
                                  $$ = us_nreverse($1);
                                  if (term_atom != unique_s)
                                    rplacd(tail_cons, term_atom); }
        ;

r_exprs : n_expr                { val exprs = cons($1, nil);
                                  rlc(exprs, $1);
                                  $$ = rlc(cons(unique_s, exprs), exprs); }
        | HASH_SEMI             { parser->ignore = 1; }
          n_expr                { parser->ignore = 0;
                                  $$ = cons(unique_s, nil); }
        | r_exprs HASH_SEMI     { parser->ignore = 1; }
          n_expr                { parser->ignore = 0;
                                  $$ = $1; }
        | r_exprs n_expr        { uses_or2;
                                  val term_atom_cons = $1;
                                  val exprs = cdr($1);
                                  misplaced_consing_dot_check(scnr, term_atom_cons);
                                  rplacd(term_atom_cons,
                                         rlc(cons($2, exprs), or2($2, exprs)));
                                  $$ = term_atom_cons; }
        | r_exprs CONSDOT n_expr
                                { val term_atom_cons = $1;
                                  misplaced_consing_dot_check(scnr, term_atom_cons);
                                  rplaca(term_atom_cons, $3);
                                  $$ = $1; }
        | WSPLICE wordslit      { $$ = cons(unique_s, us_nreverse(rl($2, num($1))));
                                  rlc($$, cdr($$)); }
        | r_exprs WSPLICE
          wordslit              { val term_atom_cons = $1;
                                  val exprs = cdr($1);
                                  misplaced_consing_dot_check(scnr, term_atom_cons);
                                  rplacd(term_atom_cons,
                                         nappend2(rl(us_nreverse($3), num($2)),
                                                  exprs));
                                  $$ = term_atom_cons; }
        | QWSPLICE wordsqlit    { $$ = cons(unique_s, rl($2, num($1)));
                                  rlc($$, cdr($$)); }
        | r_exprs QWSPLICE
          wordsqlit             { val term_atom_cons = $1;
                                  val exprs = cdr($1);
                                  misplaced_consing_dot_check(scnr, term_atom_cons);
                                  rplacd(term_atom_cons,
                                         nappend2(rl(us_nreverse($3), num($2)),
                                                  exprs));
                                  $$ = term_atom_cons; }
        ;

i_expr : SYMTOK                 { $$ = ifnign(symhlpr($1, t)); }
       | METANUM                { $$ = cons(var_s, cons($1, nil));
                                  rl($$, num(parser->lineno)); }
       | NUMBER                 { $$ = $1; }
       | compound               { $$ = $1; }
       | vector                 { $$ = $1; }
       | hash                   { $$ = $1; }
       | struct                 { $$ = $1; }
       | range                  { $$ = $1; }
       | tnode                  { $$ = $1; }
       | tree                   { $$ = $1; }
       | lisp_regex             { $$ = $1; }
       | chrlit                 { $$ = $1; }
       | strlit                 { $$ = $1; }
       | quasilit               { $$ = $1; }
       | WORDS wordslit         { $$ = rl($2, num($1)); }
       | QWORDS wordsqlit       { $$ = rl(cons(quasilist_s, $2), num($1)); }
       | buflit                 { $$ = $1; }
       | '\'' i_dot_expr        { $$ = rl(rlc(list(quote_s, $2, nao), $2),
                                          num(parser->lineno)); }
       | '^'                    { parser->quasi_level++; }
         i_dot_expr             { parser->quasi_level--;
                                  $$ = rl(rlc(list(sys_qquote_s, $3, nao), $3),
                                          num(parser->lineno)); }
       | ','                    { parser->quasi_level--; }
         i_dot_expr             { parser->quasi_level++;
                                  $$ = rl(rlc(list(sys_unquote_s, $3, nao), $3),
                                          num(parser->lineno)); }
       | SPLICE                 { parser->quasi_level--; }
         i_dot_expr             { parser->quasi_level++;
                                  $$ = rl(rlc(list(sys_splice_s, $3, nao), $3),
                                          num(parser->lineno)); }
       | HASH_N_EQUALS          { parser_circ_def(parser, $1, unique_s); }
         i_dot_expr             { parser_circ_def(parser, $1, $3);
                                  $$ = $3; }
       | HASH_N_HASH            { $$ = parser_circ_ref(parser, $1); }
       ;

i_dot_expr : '.' i_expr         { $$ = uref_helper(parser, $2); }
           | i_expr %prec LOW   { $$ = $1; }
           ;
n_expr : SYMTOK                 { $$ = ifnign(symhlpr($1, t)); }
       | METANUM                { $$ = cons(var_s, cons($1, nil));
                                  rl($$, num(parser->lineno)); }
       | NUMBER                 { $$ = $1; }
       | compound               { $$ = $1; }
       | vector                 { $$ = $1; }
       | hash                   { $$ = $1; }
       | struct                 { $$ = $1; }
       | range                  { $$ = $1; }
       | tnode                  { $$ = $1; }
       | tree                   { $$ = $1; }
       | lisp_regex             { $$ = $1; }
       | chrlit                 { $$ = $1; }
       | strlit                 { $$ = $1; }
       | quasilit               { $$ = $1; }
       | WORDS wordslit         { $$ = rl($2, num($1)); }
       | QWORDS wordsqlit       { $$ = rl(cons(quasilist_s, $2), num($1)); }
       | buflit                 { $$ = $1; }
       | '\'' n_dot_expr        { $$ = rl(rlc(list(quote_s, $2, nao), $2),
                                          num(parser->lineno)); }
       | '^'                    { parser->quasi_level++; }
         n_dot_expr             { parser->quasi_level--;
                                  $$ = rl(rlc(list(sys_qquote_s, $3, nao), $3),
                                          num(parser->lineno)); }
       | ','                    { parser->quasi_level--; }
         n_dot_expr             { parser->quasi_level++;
                                  $$ = rl(rlc(list(sys_unquote_s, $3, nao), $3),
                                          num(parser->lineno)); }
       | SPLICE                 { parser->quasi_level--; }
         n_dot_expr             { parser->quasi_level++;
                                  $$ = rl(rlc(list(sys_splice_s, $3, nao), $3),
                                          num(parser->lineno)); }
       | n_expr DOTDOT n_expr   { uses_or2;
                                  $$ = rlc(list(rcons_s, $1, $3, nao),
                                            or2($1, $3)); }
       | n_expr DOTDOT '.' n_expr
                                { uses_or2;
                                  $$ = rlc(list(rcons_s, $1,
                                                          uref_helper(parser, $4),
                                                          nao),
                                            or2($1, $4)); }
       | n_expr OLD_DOTDOT n_expr
                                { uses_or2;
                                  $$ = rlc(list(rcons_s, $1, $3, nao),
                                            or2($1, $3)); }
       | n_expr OLD_DOTDOT '.' n_expr
                                { uses_or2;
                                  $$ = rlc(list(rcons_s, $1,
                                                          uref_helper(parser, $4),
                                                          nao),
                                            or2($1, $4)); }
       | n_expr '.' n_expr      { $$ = qref_helper(parser, $1, $3); }
       | n_expr OREFDOT n_expr  { $$ = qref_helper(parser,
                                                   cons(t, cons($1, nil)),
                                                        $3); }
       | UREFDOT n_expr         { $$ = uref_helper(parser, $2); }
       | UOREFDOT n_expr        { $$ = uoref_helper(parser, $2); }
       | HASH_N_EQUALS          { parser_circ_def(parser, $1, unique_s); }
         n_dot_expr             { parser_circ_def(parser, $1, $3);
                                  $$ = $3; }
       | HASH_N_HASH            { $$ = parser_circ_ref(parser, $1); }
       ;

n_exprs_opt : n_exprs           { $$ = $1; }
            | /* empty */       { $$ = nil; }
          ;

n_dot_expr : '.' n_expr         { $$ = uref_helper(parser, $2); }
           | OREFDOT n_expr     { $$ = uoref_helper(parser, $2); }
           | n_expr %prec LOW   { $$ = $1; }
           ;

regex : '/' regexpr '/'         { $$ = regex_compile($2, nil);
                                  end_of_regex(scnr);
                                  rl($$, num(parser->lineno)); }
      | '/' error               { $$ = nil;
                                  yybadtok(yychar, lit("regex"));
                                  end_of_regex(scnr); }
      ;

lisp_regex : HASH_SLASH regexpr '/'
                                { $$ = regex_compile($2, nil);
                                  end_of_regex(scnr);
                                  rl($$, num(parser->lineno)); }
           | HASH_SLASH error
                                { $$ = nil;
                                  yybadtok(yychar, lit("regex"));
                                  end_of_regex(scnr); }
           ;

regexpr : regbranch                     { $$ = if3(cdr($1),
                                                   cons(compound_s, $1),
                                                   car($1)); }
        | regexpr '|' regexpr           { $$ = list(or_s, $1, $3, nao); }
        | regexpr '&' regexpr           { $$ = list(and_s, $1, $3, nao); }
        | '~' regexpr                   { $$ = list(compl_s, $2, nao); }
        | /* empty */ %prec LOW         { $$ = nil; }
        ;

regbranch : regterm %prec LOW   { $$ = cons($1, nil); }
          | regterm regbranch   { $$ = cons($1, $2); }
          | regterm '~' regexpr { $$ = list($1, list(compl_s, $3, nao), nao); }
          ;

regterm : regterm '*'           { $$ = list(zeroplus_s, $1, nao); }
        | regterm '+'           { $$ = list(oneplus_s, $1, nao); }
        | regterm '?'           { $$ = list(optional_s, $1, nao); }
        | regterm '%' regexpr   { $$ = list(nongreedy_s, $1, $3, nao); }
        | '[' regclass ']'      { if (first($2) == chr('^'))
                                  { if (rest($2))
                                      $$ = cons(cset_s, rest($2));
                                    else
                                      $$ = wild_s; }
                                  else
                                    $$ = cons(set_s, $2); }
        | '[' ']'               { $$ = cons(set_s, nil); }
        | '[' error             { $$ = nil;
                                  yybadtok(yychar, lit("regex character class")); }
        | '.'                   { $$ = wild_s; }
        | ']'                   { $$ = chr(']'); }
        | '-'                   { $$ = chr('-'); }
        | REGCHAR               { $$ = chr($1); }
        | regtoken              { $$ = $1; }
        | '(' regexpr ')'       { $$ = $2; }
        | '(' error             { $$ = nil;
                                  yybadtok(yychar, lit("regex subexpression")); }
        ;

regclass : regclassterm                 { $$ = cons($1, nil); }
         | regclassterm regclass        { $$ = cons($1, $2); }
         ;

regclassterm : regrange         { $$ = $1; }
             | regchar          { $$ = chr($1); }
             | regtoken         { $$ = $1; }
             ;

regrange : regchar '-' regchar  { $$ = cons(chr($1), chr($3)); }

regchar : '?'                   { $$ = '?'; }
        | '.'                   { $$ = '.'; }
        | '*'                   { $$ = '*'; }
        | '+'                   { $$ = '+'; }
        | '('                   { $$ = '('; }
        | ')'                   { $$ = ')'; }
        | '|'                   { $$ = '|'; }
        | '~'                   { $$ = '~'; }
        | '&'                   { $$ = '&'; }
        | '%'                   { $$ = '%'; }
        | '/'                   { $$ = '/'; }
        | REGCHAR               { $$ = $1; }
        ;

regtoken : REGTOKEN             { switch ($1)
                                  { case 's':
                                      $$ = space_k; break;
                                    case 'S':
                                      $$ = cspace_k; break;
                                    case 'd':
                                      $$ = digit_k; break;
                                    case 'D':
                                      $$ = cdigit_k; break;
                                    case 'w':
                                      $$ = word_char_k; break;
                                    case 'W':
                                      $$ = cword_char_k; break; }}

newl : '\n'
     | error '\n'       { yyerr("newline expected after directive");
                          yyerrok; }
     ;

strlit : '"' '"'                { $$ = null_string; }
       | '"' litchars '"'       { $$ = $2;
                                  rl($$, num(parser->lineno)); }
       | '"' error              { $$ = nil;
                                  yybadtok(yychar, lit("string literal")); }
       ;

chrlit : HASH_BACKSLASH SYMTOK  { wchar_t ch;
                                  val str = string_own($2);
                                  const wchar_t *cstr = c_str(str);

                                  if (cstr[1] == 0)
                                  { ch = cstr[0]; }
                                  else
                                  { ch = char_from_name(cstr);
                                    if (ch == L'!')
                                    { yyerrorf(scnr, lit("unknown character name: ~a"),
                                               str, nao); }}
                                  end_of_char(scnr);
                                  $$ = chr(ch); }
       | HASH_BACKSLASH LITCHAR { $$ = chr($2);
                                  end_of_char(scnr); }
       | HASH_BACKSLASH error   { $$ = nil;
                                  yybadtok(yychar,
                                             lit("character literal")); }
       ;

quasilit : '`' '`'              { $$ = null_string; }
         | '`' quasi_items '`'  { $$ = cons(quasi_s, $2);
                                  rlc($$, $2);
                                  rl($$, num(parser->lineno)); }
         | '`' error            { $$ = nil;
                                  yybadtok(yychar, lit("quasistring")); }
         ;

quasi_items : quasi_item                { $$ = cons($1, nil);
                                          rl($$, num(parser->lineno)); }
            | quasi_item quasi_items    { $$ = cons($1, $2);
                                          rl($$, num(parser->lineno)); }
            ;

quasi_item : litchars           { $$ = $1; }
           | TEXT               { $$ = string_own($1); }
           | q_var              { $$ = $1; }
           | METANUM            { $$ = cons(var_s, cons($1, nil));
                                  rl($$, num(parser->lineno)); }
           | '@' n_expr         { if (integerp($2) || symbolp($2))
                                    $$ = rlcp_tree(cons(var_s, cons($2, nil)),
                                                   $2);
                                  else
                                    $$ = $2; }
           ;

litchars : LITCHAR              { $$ = mkstring(one, chr($1)); }
         | LITCHAR restlitchar  { val ch = mkstring(one, chr($1));
                                  $$ = string_extend(ch, $2); }
         ;

restlitchar : LITCHAR                   { $$ = mkstring(one, chr($1)); }
            | restlitchar LITCHAR       { $$ = string_extend($1, chr($2)); }
            ;

wordslit : '"'                  { $$ = nil; }
         | ' ' wordslit         { $$ = $2; }
         | litchars wordslit    { val word = $1;
                                  $$ = rlc(cons(word, $2), $1); }
         | error                { $$ = nil;
                                  yybadtok(yychar, lit("word list")); }
         ;

wordsqlit : '`'                  { $$ = nil; }
          | ' ' wordsqlit        { $$ = $2; }
          | quasi_items '`'      { val qword = cons(quasi_s, $1);
                                   $$ = rlc(cons(qword, nil), $1); }
          | quasi_items ' '
            wordsqlit
                                 { val qword = cons(quasi_s, $1);
                                   $$ = rlc(cons(qword, $3), $1); }
          ;

buflit : HASH_B_QUOTE '\''               { $$ = make_buf(zero, nil, nil);
                                           end_of_buflit(scnr); }
       | HASH_B_QUOTE buflit_items '\''  { end_of_buflit(scnr);
                                           buf_trim($2);
                                           $$ = $2; }
       | HASH_B_QUOTE error              { yyerr("unterminated buffer literal");
                                           end_of_buflit(scnr);
                                           yyerrok; }
       ;

buflit_items : buflit_items buflit_item { buf_put_u8($1, length_buf($$), $2);
                                          $$ = $1; }
             | buflit_item              { $$ = make_buf(zero, nil, num_fast(512));
                                          buf_put_u8($$, zero, $1); }
             ;

buflit_item  : LITCHAR LITCHAR          { $$ = num($1 << 4 | $2); }
             | LITCHAR error            { $$ = zero;
                                          yyerr("unpaired digit in buffer literal");
                                          yyerrok; }
             ;



not_a_clause : ALL              { $$ = mkexp(all_s, nil, num(parser->lineno)); }
             | SOME             { $$ = mkexp(some_s, nil, num(parser->lineno)); }
             | NONE             { $$ = mkexp(none_s, nil, num(parser->lineno)); }
             | MAYBE            { $$ = mkexp(maybe_s, nil, num(parser->lineno)); }
             | CASES            { $$ = mkexp(cases_s, nil, num(parser->lineno)); }
             | AND              { $$ = mkexp(and_s, nil, num(parser->lineno)); }
             | OR               { $$ = mkexp(or_s, nil, num(parser->lineno)); }
             | TRY              { $$ = mkexp(try_s, nil, num(parser->lineno)); }
             | FINALLY          { $$ = mkexp(finally_s, nil, num(parser->lineno)); }
             | ELSE             { $$ = mkexp(else_s, nil, num(parser->lineno)); }
             | ELIF             { $$ = mkexp(elif_s, nil, num(parser->lineno)); }
             | BLOCK
               exprs_opt ')'    { $$ = mkexp(block_s, $2, nil); }
             | CHOOSE
               exprs_opt ')'    { $$ = mkexp(choose_s, $2, nil); }
             | COLLECT
               exprs_opt ')'    { $$ = mkexp(collect_s, $2, nil); }
             | COLL
               exprs_opt ')'    { $$ = mkexp(coll_s, $2, nil); }
             | GATHER
               exprs_opt ')'    { $$ = mkexp(gather_s, $2, nil); }
             | DEFINE
               exprs_opt ')'    { $$ = mkexp(define_s, $2, nil); }
             | CATCH
               exprs_opt ')'    { $$ = mkexp(catch_s, $2, nil); }
             | IF
               exprs_opt ')'    { $$ = mkexp(if_s, $2, nil); }
             | OUTPUT
                exprs_opt ')'   { yyerr("@(output) doesn't nest"); }

             ;

%%

const int have_yydebug = YYDEBUG;

int yylex(YYSTYPE *, yyscan_t scanner);

void yydebug_onoff(int val)
{
#if YYDEBUG
  yydebug = val;
#endif
}

static val sym_helper(parser_t *parser, wchar_t *lexeme, val meta_allowed)
{
  scanner_t *scnr = parser->scanner;
  int leading_at = *lexeme == L'@';
  wchar_t *tokfree = lexeme;
  wchar_t *colon = wcschr(lexeme, L':');
  val sym;

  if (leading_at) {
    if (!meta_allowed) {
      val tok = string_own(lexeme);
      yyerrorf(scnr, lit("~a: meta variable not allowed in this context"), tok, nao);
      return nil;
    }
    lexeme++;
  }

  if (colon != 0)
    *colon = 0;

  if (colon == lexeme) {
    val sym_name = string(colon + 1);
    scrub_scanner(parser->scanner, SYMTOK, tokfree);
    free(tokfree);
    sym = intern(sym_name, keyword_package);
  } else if (colon != 0) {
    val pkg_name = string(lexeme);
    val sym_name = string(colon + 1);
    scrub_scanner(parser->scanner, SYMTOK, tokfree);
    free(tokfree);
    if (equal(pkg_name, lit("#"))) {
      sym = make_sym(sym_name);
    } else {
      val package = find_package(pkg_name);
      if (!package) {
        yyerrorf(scnr, lit("~a:~a: package ~a not found"),
                 pkg_name, sym_name, pkg_name, nao);
        return nil;
      }
      sym = intern(sym_name, package);
    }
  } else {
    val sym_name = string(lexeme);
    scrub_scanner(parser->scanner, SYMTOK, tokfree);
    free(tokfree);
    sym = intern_fallback(sym_name, cur_package);
  }

  return leading_at ? rl(list(var_s, sym, nao), num(parser->lineno)) : sym;
}

static val expand_repeat_rep_args(val args)
{
  list_collect_decl (out, ptail);
  val exp_pair = nil, exp_pairs = nil;

  for (; args; args = cdr(args)) {
    val arg = car(args);

    if (consp(arg)) {
      if (exp_pairs) {
        list_collect_decl (iout, iptail);
        for (; arg; arg = cdr(arg)) {
          val iarg = car(arg);
          if (consp(iarg)) {
            val sym = first(iarg);
            iptail = list_collect(iptail, list(sym,
                                               expand(second(iarg), nil),
                                               nao));
            match_reg_var(sym);
          } else {
            iptail = list_collect(iptail, iarg);
            match_reg_var(iarg);
          }
        }
        ptail = list_collect(ptail, iout);
      } else if (exp_pair) {
        val sym = first(arg);
        ptail = list_collect(ptail, list(sym,
                                         expand(second(arg), nil),
                                         nao));
        match_reg_var(sym);
      } else {
        ptail = list_collect(ptail, arg);
      }
    } else if (!exp_pair && !exp_pairs) {
      if (arg == counter_k) {
        exp_pair = t;
        ptail = list_collect(ptail, arg);
        continue;
      } else if (arg == vars_k) {
        exp_pairs = t;
        ptail = list_collect(ptail, arg);
        continue;
      }
    } else if (exp_pair) {
      match_reg_var(arg);
    }

    exp_pair = exp_pairs = nil;
    ptail = list_collect(ptail, arg);
  }

  return out;
}

static val repeat_rep_helper(val sym, val args, val main, val parts)
{
  uses_or2;
  val exp_args = expand_repeat_rep_args(args);
  val single_parts = nil, single_parts_p = nil;
  val first_parts = nil, first_parts_p = nil;
  val last_parts = nil, last_parts_p = nil;
  val empty_parts = nil, empty_parts_p = nil;
  val mod_parts = nil, mod_parts_p = nil;
  val modlast_parts = nil, modlast_parts_p = nil;
  val iter;

  for (iter = parts; iter != nil; iter = cdr(iter)) {
    val part = car(iter);
    val sym = car(part);
    val clauses = copy_list(cdr(part));

    if (sym == single_s) {
      single_parts = nappend2(single_parts, clauses);
      single_parts_p = t;
    } else if (sym == first_s) {
      first_parts = nappend2(first_parts, clauses);
      first_parts_p = t;
    } else if (sym == last_s) {
      last_parts = nappend2(last_parts, clauses);
      last_parts_p = t;
    } else if (sym == empty_s) {
      empty_parts = nappend2(empty_parts, clauses);
      empty_parts_p = t;
    } else if (sym == mod_s) {
      mod_parts = cons(clauses, mod_parts);
      mod_parts_p = t;
    } else if (sym == modlast_s) {
      modlast_parts = cons(clauses, modlast_parts);
      modlast_parts_p = t;
    } else {
      abort();
    }
  }

  single_parts = or2(single_parts, single_parts_p);
  first_parts = or2(first_parts, first_parts_p);
  last_parts = or2(last_parts, last_parts_p);
  empty_parts = or2(empty_parts, empty_parts_p);
  mod_parts = or2(nreverse(mod_parts), mod_parts_p);
  modlast_parts = or2(nreverse(modlast_parts), modlast_parts_p);

  return list(sym, exp_args, main, single_parts, first_parts,
              last_parts, empty_parts, nreverse(mod_parts),
              nreverse(modlast_parts), nao);
}

static void process_catch_exprs(val exprs)
{
  val params = second(exprs);
  for (; params; params = cdr(params)) {
    val param = first(params);
    if (consp(param))
      match_reg_var(car(param));
    else
      match_reg_var(param);
  }
}

static val define_transform(parser_t *parser, val define_form)
{
  scanner_t *scnr = parser->scanner;
  val sym = first(define_form);
  val args = second(define_form);

  if (define_form == nil)
    return nil;

  assert (sym == define_s);

  if (args == nil) {
    yyerr("define requires arguments");
    return define_form;
  }

  if (!consp(args) || !listp(cdr(args))) {
    yyerr("bad define argument syntax");
    return define_form;
  } else {
    val name = first(args);
    val params = second(args);

    if (!symbolp(name)) {
      yyerr("function name must be a symbol");
      return define_form;
    }

    if (!proper_list_p(params)) {
      yyerr("invalid function parameter list");
      return define_form;
    }

    if (!all_satisfy(params, func_n1(symbolp), nil))
      yyerr("function parameters must be symbols");
  }

  return define_form;
}

static val optimize_text(val text_form)
{
  if (all_satisfy(rest(text_form), func_n1(stringp), nil))
    return cat_str(rest(text_form), lit(""));
  return text_form;
}

static val unquotes_occur(val quoted_form, int level)
{
  uses_or2;

  if (atom(quoted_form)) {
    return nil;
  } else {
    val sym = car(quoted_form);
    if (sym == sys_unquote_s || sym == sys_splice_s)
      return (level == 0) ? t : unquotes_occur(cdr(quoted_form), level - 1);
    if (sym == sys_qquote_s)
      return unquotes_occur(cdr(quoted_form), level + 1);
    return or2(unquotes_occur(sym, level),
               unquotes_occur(cdr(quoted_form), level));
  }
}

val expand_meta(val form, val menv)
{
  val sym;

  if (atom(form))
    return form;

  menv = default_arg(menv, make_env(nil, nil, nil));

  if ((sym = car(form)) == quasi_s) {
    if (opt_compat && opt_compat <= 128) {
      list_collect_decl (out, ptail);

      for (; consp(form); form = cdr(form)) {
        val subform = car(form);
        if (consp(subform) && car(subform) == expr_s)
          ptail = list_collect(ptail, expand_meta(subform, menv));
        else
          ptail = list_collect(ptail, subform);
      }

      ptail = list_collect_nconc(ptail, form);

      return rlcp(out, form);
    }

    return expand(form, nil);
  }

  if ((sym = car(form)) == expr_s) {
    val exp_x = expand(second(form), menv);
    if (!bindable(exp_x))
      return rlcp(cons(sym, cons(exp_x, nil)), form);
    return rlcp(cons(var_s, cons(exp_x, nil)), form);
  }

  if (sym == var_s) {
    val var_x = expand(second(form), menv);
    if (!bindable(var_x))
      return rlcp(cons(expr_s, cons(var_x, nil)), form);
    return rlcp(cons(var_s, cons(var_x, nil)), form);
  }

  {
    list_collect_decl (out, ptail);

    for (; consp(form); form = cdr(form)) {
      loc nptail = list_collect(ptail, expand_meta(car(form), menv));
      rlcp(deref(ptail), form);
      ptail = nptail;
    }

    ptail = list_collect_nconc(ptail, form);

    return out;
  }
}

static val rlviable(val form)
{
  switch (type(form)) {
  case NIL:
  case LIT:
  case CHR:
  case NUM:
  case SYM:
  case BGNUM:
  case FLNUM:
    return nil;
  default:
    return t;
  }
}

val rlset(val form, val info)
{
  if (rlviable(form)) {
    loc place = gethash_l(lit("rlcp"), form_to_ln_hash, form, nulloc);
    if (nilp(deref(place)))
      set(place, info);
  }
  return form;
}

val rlrec(parser_t *parser, val form, val line)
{
  if (parser->rec_source_loc)
    rlset(form, cons(line, parser->name));
  return form;
}

val rlcp_parser(parser_t *parser, val to, val from)
{
  if (parser->rec_source_loc)
    rlset(to, source_loc(from));
  return to;
}

static val rlcp_tree_rec(val to, val from, struct circ_stack *up)
{
  val ret = to;

  while (consp(to)) {
    val a = car(to);
    struct circ_stack rlcs = { up, a };
    rlcp(to, from);
    if (!parser_callgraph_circ_check(up, a))
      break;
    rlcp_tree_rec(a, from, &rlcs);
    to = cdr(to);
    if (!parser_callgraph_circ_check(up, to))
      break;
  }
  return ret;
}


val rlcp_tree(val to, val from)
{
  return rlcp_tree_rec(to, from, 0);
}

static wchar_t char_from_name(const wchar_t *name)
{
  static struct {
    const wchar_t *name;
    const wchar_t ch;
  } map[] = {
    { L"nul", 0 },
    { L"alarm", L'\a' },
    { L"backspace", L'\b' },
    { L"tab", L'\t' },
    { L"linefeed", L'\n' },
    { L"newline", L'\n' },
    { L"vtab", L'\v' },
    { L"page", L'\f' },
    { L"return", L'\r' },
    { L"esc", 27 },
    { L"space", L' ' },
    { L"pnul", 0xDC00 },
    { 0, 0 },
  };
  int i;

  for (i = 0; map[i].name; i++) {
    if (wcscmp(map[i].name, name) == 0)
      return map[i].ch;
  }

  return L'!'; /* code meaning not found */
}

static val make_expr(parser_t *parser, val sym, val rest, val lineno)
{
  val expr = cons(sym, rest);
  val ret = cons(expr_s, cons(expand(expr, nil), nil));

  if (rest) {
    rlc(expr, rest);
    rlc(ret, rest);
  } else {
    rl(expr, lineno);
    rl(ret, lineno);
  }

  return ret;
}

static val check_parse_time_action(val spec_rev)
{
  val line = first(spec_rev);

  if (consp(line)) {
    val elem = first(line);
    if (consp(elem)) {
      val sym = car(elem);
      if (sym == include_s) {
        return nappend2(nreverse(include(line)), rest(spec_rev));
      }
      if (sym == mdo_s) {
        eval_intrinsic(cons(progn_s, cdr(elem)), nil);
        return nil;
      }
      if (sym == in_package_s) {
        eval_intrinsic(elem, nil);
        return nil;
      }
    }
  }
  return spec_rev;
}

static void misplaced_consing_dot_check(scanner_t *scanner, val term_atom_cons)
{
  if (car(term_atom_cons) != unique_s) {
    yyerrorf(scanner, lit("misplaced consing dot"), nao);
    rplaca(term_atom_cons, unique_s);
  }
}

static val uref_helper(parser_t *parser, val expr)
{
  if (consp(expr) && car(expr) == qref_s) {
    return rplaca(expr, uref_s);
  } else {
    return rl(rlc(list(uref_s, expr, nao), expr), num(parser->lineno));
  }
}

static val uoref_helper(parser_t *parser, val expr)
{
  val uref = uref_helper(parser, expr);
  rplacd(uref, cons(t, cdr(uref)));
  return uref;
}

static val qref_helper(parser_t *parser, val lexpr, val rexpr)
{
  uses_or2;

  if (consp(rexpr) && car(rexpr) == qref_s) {
    rplacd(rexpr, rlc(cons(lexpr, cdr(rexpr)), lexpr));
    return rl(rexpr, num(parser->lineno));
  } else {
    return rl(rlc(list(qref_s, lexpr, rexpr, nao),
                  or2(lexpr, rexpr)),
                  num(parser->lineno));
  }
}

static val fname_helper(parser_t *parser, val name)
{
  if (!name) {
    return nil;
  } else if (!bindable(name)) {
    yyerrorf(parser->scanner, lit("#T: ~s isn't a function name"),
             name, nao);
  } else if (!memq(name, tree_fun_whitelist)) {
    yyerrorf(parser->scanner, lit("#T: ~s not in *tree-fun-whitelist*"),
             name, nao);
  } else {
    val fbinding = lookup_fun(nil, name);
    if (fbinding)
      return cdr(fbinding);
    yyerrorf(parser->scanner, lit("#T: function named ~s doesn't exist"),
             name, nao);
  }

  return nil;
}

#ifndef YYEOF
#define YYEOF 0
#endif

void yybadtoken(parser_t *parser, int tok, val context)
{
  val problem = nil;
  scanner_t *scnr = parser->scanner;

  switch (tok) {
  case ERRTOK:
    return;
  case SPACE:   problem = lit("space"); break;
  case TEXT:    problem = lit("text"); break;
  case SYMTOK:  problem = lit("symbol-token"); break;
  case METANUM: problem = lit("metanum"); break;
  case ALL:     problem = lit("\"all\""); break;
  case SOME:    problem = lit("\"some\""); break;
  case NONE:    problem = lit("\"none\""); break;
  case MAYBE:   problem = lit("\"maybe\""); break;
  case CASES:   problem = lit("\"cases\""); break;
  case CHOOSE:  problem = lit("\"choose\""); break;
  case AND:     problem = lit("\"and\""); break;
  case OR:      problem = lit("\"or\""); break;
  case END:     problem = lit("\"end\""); break;
  case COLLECT: problem = lit("\"collect\""); break;
  case UNTIL:   problem = lit("\"until\""); break;
  case COLL:    problem = lit("\"coll\""); break;
  case OUTPUT:  problem = lit("\"output\""); break;
  case REPEAT:  problem = lit("\"repeat\""); break;
  case REP:     problem = lit("\"rep\""); break;
  case SINGLE:  problem = lit("\"single\""); break;
  case FIRST:   problem = lit("\"first\""); break;
  case LAST:    problem = lit("\"last\""); break;
  case EMPTY:   problem = lit("\"empty\""); break;
  case DEFINE:  problem = lit("\"define\""); break;
  case TRY:     problem = lit("\"try\""); break;
  case CATCH:   problem = lit("\"catch\""); break;
  case FINALLY: problem = lit("\"finally\""); break;
  case IF:      problem = lit("\"if\""); break;
  case ELIF:    problem = lit("\"elif\""); break;
  case ELSE:    problem = lit("\"else\""); break;
  case NUMBER:  problem = lit("number"); break;
  case REGCHAR: problem = lit("regular expression character"); break;
  case REGTOKEN: problem = lit("regular expression token"); break;
  case LITCHAR: problem = lit("string literal character"); break;
  case CONSDOT:
  case LAMBDOT: problem = lit("consing dot"); break;
  case DOTDOT: problem = lit(".."); break;
  case OLD_DOTDOT:     problem = lit(".."); break;
  case UREFDOT:        problem = lit("referencing dot"); break;
  case OREFDOT:
  case UOREFDOT:       problem = lit("referencing .?"); break;
  case HASH_BACKSLASH: problem = lit("#\\"); break;
  case HASH_SLASH:     problem = lit("#/"); break;
  case HASH_H:         problem = lit("#H"); break;
  case HASH_S:         problem = lit("#S"); break;
  case HASH_R:         problem = lit("#R"); break;
  case HASH_N:         problem = lit("#N"); break;
  case HASH_T:         problem = lit("#T"); break;
  case HASH_SEMI:      problem = lit("#;"); break;
  case HASH_N_EQUALS:  problem = lit("#<n>="); break;
  case HASH_N_HASH:    problem = lit("#<n>#"); break;
  case HASH_B_QUOTE:   problem = lit("#b'"); break;
  case WORDS:   problem = lit("#\""); break;
  case WSPLICE: problem = lit("#*\""); break;
  case QWORDS:         problem = lit("#`"); break;
  case QWSPLICE:       problem = lit("#*`"); break;
  }

  if (problem != 0)
    if (context)
      yyerrorf(scnr, lit("misplaced ~a in ~a"), problem, context, nao);
    else
      yyerrorf(scnr, lit("unexpected ~a"), problem, nao);
  else
    if (context) /* Byacc sets yychar to 0 */
      if (tok == YYEOF || tok == YYEMPTY)
        yyerrorf(scnr, lit("unterminated ~a"), context, nao);
      else if (tok == '\n')
        yyerrorf(scnr, lit("newline in ~a"), context, nao);
      else
        yyerrorf(scnr, lit("misplaced character ~a in ~a"), chr(tok), context, nao);
    else
      if (tok == YYEOF)
        yyerrorf(scnr, lit("unexpected end of input"), nao);
      else if (tok == YYEMPTY)
        return;
      else
        yyerrorf(scnr, lit("unexpected character ~a"), chr(tok), nao);
}

int parse_once(val self, val stream, val name)
{
  int res = 0;
#if CONFIG_DEBUG_SUPPORT
  unsigned dbg_state = debug_clear(opt_dbg_expansion ? 0 : DBG_ENABLE);
#endif
  val parser_obj = ensure_parser(stream, name);
  parser_t *parser = parser_get_impl(self, parser_obj);
  parser->rec_source_loc = 1;

  uw_catch_begin(cons(error_s, nil), esym, eobj);


  res = yyparse(parser->scanner, parser);

  parser_resolve_circ(parser);

  uw_catch(esym, eobj) {
    yyerrorf(parser->scanner, lit("error exception during parse"), nao);
    uw_throw(esym, eobj);
  }

  uw_unwind {
#if CONFIG_DEBUG_SUPPORT
    debug_set(dbg_state);
#endif
  }

  uw_catch_end;

  return res;
}

int parse(parser_t *parser, val name, enum prime_parser prim)
{
  int res = 0;
  cnum start_line = parser->lineno;

  parser->errors = 0;
  parser->eof = 0;
  parser->ignore = 0;
  parser->prepared_msg = nil;
  parser->circ_ref_hash = nil;
  parser->circ_count = 0;
  parser->syntax_tree = nil;
  parser->quasi_level = 0;

  prime_parser(parser, name, prim);

  uw_catch_begin(cons(error_s, nil), esym, eobj);

  res = yyparse(parser->scanner, parser);

  prime_parser_post(parser, prim);

  parser_resolve_circ(parser);

  uw_catch(esym, eobj) {
    yyerrorf(parser->scanner, lit("error exception during parse"), nao);
    uw_throw(esym, eobj);
  }

  uw_unwind;

  uw_catch_end;

  if (parser->errors && parser->syntax_tree == nil &&
      parser->lineno != start_line)
  {
    yyerrorf(parser->scanner, lit("while parsing form starting at line ~a"),
             num(start_line), nao);
  }

  return res;
}
