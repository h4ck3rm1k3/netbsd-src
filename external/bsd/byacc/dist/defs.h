/* Id: defs.h,v 1.20 2009/10/27 10:47:43 tom Exp */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#define YYMAJOR 1
#define YYMINOR 9

#define CONCAT(first,second)    first #second
#define CONCAT1(string,number)  CONCAT(string, number)
#define CONCAT2(first,second)   #first "." #second

#ifdef YYPATCH
#define VSTRING(a,b) CONCAT2(a,b) CONCAT1(" ",YYPATCH)
#else
#define VSTRING(a,b) CONCAT2(a,b)
#endif

#define VERSION VSTRING(YYMAJOR, YYMINOR)

/*  machine-dependent definitions			*/
/*  the following definitions are for the Tahoe		*/
/*  they might have to be changed for other machines	*/

/*  MAXCHAR is the largest unsigned character value	*/
/*  MAXSHORT is the largest value of a C short		*/
/*  MINSHORT is the most negative value of a C short	*/
/*  MAXTABLE is the maximum table size			*/
/*  BITS_PER_WORD is the number of bits in a C unsigned	*/
/*  WORDSIZE computes the number of words needed to	*/
/*	store n bits					*/
/*  BIT returns the value of the n-th bit starting	*/
/*	from r (0-indexed)				*/
/*  SETBIT sets the n-th bit starting from r		*/

#define	MAXCHAR		255
#define	MAXSHORT	32767
#define MINSHORT	-32768
#define MAXTABLE	32500
#define BITS_PER_WORD	32
#define	WORDSIZE(n)	(((n)+(BITS_PER_WORD-1))/BITS_PER_WORD)
#define	BIT(r, n)	((((r)[(n)>>5])>>((n)&31))&1)
#define	SETBIT(r, n)	((r)[(n)>>5]|=((unsigned)1<<((n)&31)))

/*  character names  */

#define	NUL		'\0'	/*  the null character  */
#define	NEWLINE		'\n'	/*  line feed  */
#define	SP		' '	/*  space  */
#define	BS		'\b'	/*  backspace  */
#define	HT		'\t'	/*  horizontal tab  */
#define	VT		'\013'	/*  vertical tab  */
#define	CR		'\r'	/*  carriage return  */
#define	FF		'\f'	/*  form feed  */
#define	QUOTE		'\''	/*  single quote  */
#define	DOUBLE_QUOTE	'\"'	/*  double quote  */
#define	BACKSLASH	'\\'	/*  backslash  */

/* defines for constructing filenames */

#if defined(VMS)
#define CODE_SUFFIX	"_code.c"
#define	DEFINES_SUFFIX	"_tab.h"
#define	OUTPUT_SUFFIX	"_tab.c"
#else
#define CODE_SUFFIX	".code.c"
#define	DEFINES_SUFFIX	".tab.h"
#define	OUTPUT_SUFFIX	".tab.c"
#endif
#define	VERBOSE_SUFFIX	".output"
#define GRAPH_SUFFIX    ".dot"

/* keyword codes */

#define TOKEN 0
#define LEFT 1
#define RIGHT 2
#define NONASSOC 3
#define MARK 4
#define TEXT 5
#define TYPE 6
#define START 7
#define UNION 8
#define IDENT 9
#define EXPECT 10
#define EXPECT_RR 11
#define PURE_PARSER 12
#define PARSE_PARAM 13
#define LEX_PARAM 14

/*  symbol classes  */

#define UNKNOWN 0
#define TERM 1
#define NONTERM 2

/*  the undefined value  */

#define UNDEFINED (-1)

/*  action codes  */

#define SHIFT 1
#define REDUCE 2

/*  character macros  */

#define IS_IDENT(c)	(isalnum(c) || (c) == '_' || (c) == '.' || (c) == '$')
#define	IS_OCTAL(c)	((c) >= '0' && (c) <= '7')
#define	NUMERIC_VALUE(c)	((c) - '0')

/*  symbol macros  */

#define ISTOKEN(s)	((s) < start_symbol)
#define ISVAR(s)	((s) >= start_symbol)

/*  storage allocation macros  */

#define CALLOC(k,n)	(calloc((unsigned)(k),(unsigned)(n)))
#define	FREE(x)		(free((char*)(x)))
#define MALLOC(n)	(malloc((unsigned)(n)))
#define	NEW(t)		((t*)allocate(sizeof(t)))
#define	NEW2(n,t)	((t*)allocate(((unsigned)(n)*sizeof(t))))
#define REALLOC(p,n)	(realloc((char*)(p),(unsigned)(n)))

#define DO_FREE(x)	if (x) { FREE(x); x = 0; }

/* messages */
#define PLURAL(n) ((n) > 1 ? "s" : "")

typedef char Assoc_t;
typedef char Class_t;
typedef short Index_t;
typedef short Value_t;

/*  the structure of a symbol table entry  */

typedef struct bucket bucket;
struct bucket
{
    struct bucket *link;
    struct bucket *next;
    char *name;
    char *tag;
    Value_t value;
    Index_t index;
    Value_t prec;
    Class_t class;
    Assoc_t assoc;
};

/*  the structure of the LR(0) state machine  */

typedef struct core core;
struct core
{
    struct core *next;
    struct core *link;
    Value_t number;
    Value_t accessing_symbol;
    Value_t nitems;
    Value_t items[1];
};

/*  the structure used to record shifts  */

typedef struct shifts shifts;
struct shifts
{
    struct shifts *next;
    Value_t number;
    Value_t nshifts;
    Value_t shift[1];
};

typedef struct param param;
struct param {
    struct param *next;
    char *name;
    char *type;
};

/*  the structure used to store reductions  */

typedef struct reductions reductions;
struct reductions
{
    struct reductions *next;
    Value_t number;
    Value_t nreds;
    Value_t rules[1];
};

/*  the structure used to represent parser actions  */

typedef struct action action;
struct action
{
    struct action *next;
    Value_t symbol;
    Value_t number;
    Value_t prec;
    char action_code;
    Assoc_t assoc;
    char suppressed;
};

/* global variables */

extern char dflag;
extern char gflag;
extern char lflag;
extern char rflag;
extern char tflag;
extern char vflag;
extern const char *symbol_prefix;

extern const char *myname;
extern char *cptr;
extern char *line;
extern int lineno;
extern int outline;
extern int exit_code;
extern int pure_parser;

extern const char * const banner[];
extern const char * const tables[];
extern const char * const header[];
extern const char * const body[];
extern const char * const trailer[];

extern char *code_file_name;
extern char *defines_file_name;
extern const char *input_file_name;
extern char *output_file_name;
extern char *verbose_file_name;
extern char *graph_file_name;

extern FILE *action_file;
extern FILE *code_file;
extern FILE *defines_file;
extern FILE *input_file;
extern FILE *output_file;
extern FILE *text_file;
extern FILE *union_file;
extern FILE *verbose_file;
extern FILE *graph_file;

extern int nitems;
extern int nrules;
extern int nsyms;
extern int ntokens;
extern int nvars;
extern int ntags;

extern char unionized;
extern char line_format[];

extern Value_t start_symbol;
extern char **symbol_name;
extern char **symbol_pname;
extern Value_t *symbol_value;
extern Value_t *symbol_prec;
extern char *symbol_assoc;

extern Value_t *ritem;
extern Value_t *rlhs;
extern Value_t *rrhs;
extern Value_t *rprec;
extern Assoc_t *rassoc;

extern Value_t **derives;
extern char *nullable;

extern bucket *first_symbol;
extern bucket *last_symbol;

extern int nstates;
extern core *first_state;
extern shifts *first_shift;
extern reductions *first_reduction;
extern Value_t *accessing_symbol;
extern core **state_table;
extern shifts **shift_table;
extern reductions **reduction_table;
extern unsigned *LA;
extern Value_t *LAruleno;
extern Value_t *lookaheads;
extern Value_t *goto_map;
extern Value_t *from_state;
extern Value_t *to_state;

extern action **parser;
extern int SRexpect;
extern int RRexpect;
extern int SRtotal;
extern int RRtotal;
extern Value_t *SRconflicts;
extern Value_t *RRconflicts;
extern Value_t *defred;
extern Value_t *rules_used;
extern Value_t nunused;
extern Value_t final_state;

extern Value_t *itemset;
extern Value_t *itemsetend;
extern unsigned *ruleset;

extern param *lex_param;
extern param *parse_param;

/* global functions */

extern bucket *lookup(const char *);
extern bucket *make_bucket(const char *);

#ifndef GCC_NORETURN
#define GCC_NORETURN		/* nothing */
#endif

#ifndef GCC_UNUSED
#define GCC_UNUSED		/* nothing */
#endif

/* closure.c */
extern void closure(Value_t *nucleus, int n);
extern void finalize_closure(void);
extern void set_first_derives(void);

/* error.c */
extern void default_action_warning(void);
extern void dollar_error(int a_lineno, char *a_line, char *a_cptr);
extern void dollar_warning(int a_lineno, int i);
extern void fatal(const char *msg);
extern void illegal_character(char *c_cptr);
extern void illegal_tag(int t_lineno, char *t_line, char *t_cptr);
extern void no_grammar(void);
extern void no_space(void);
extern void open_error(const char *filename);
extern void over_unionized(char *u_cptr);
extern void prec_redeclared(void);
extern void print_pos(char *st_line, char *st_cptr);
extern void reprec_warning(char *s);
extern void restarted_warning(void);
extern void retyped_warning(char *s);
extern void revalued_warning(char *s);
extern __dead void syntax_error(int st_lineno, char *st_line, char *st_cptr) GCC_NORETURN;
extern void terminal_lhs(int s_lineno);
extern void terminal_start(char *s);
extern void tokenized_start(char *s);
extern void undefined_goal(char *s);
extern void undefined_symbol_warning(char *s);
extern void unexpected_EOF(void);
extern void unknown_rhs(int i);
extern void unterminated_action(int a_lineno, char *a_line, char *a_cptr);
extern void unterminated_comment(int c_lineno, char *c_line, char *c_cptr);
extern void unterminated_string(int s_lineno, char *s_line, char *s_cptr);
extern void unterminated_text(int t_lineno, char *t_line, char *t_cptr);
extern void unterminated_union(int u_lineno, char *u_line, char *u_cptr);
extern void untyped_lhs(void);
extern void untyped_rhs(int i, char *s);
extern void used_reserved(char *s);

/* graph.c */
extern void graph(void);

/* lalr.c */
extern int hash(const char *name);
extern void create_symbol_table(void);
extern void free_symbol_table(void);
extern void free_symbols(void);

/* lalr.c */
extern void lalr(void);

/* lr0.c */
extern void lr0(void);
extern void show_cores(void);
extern void show_ritems(void);
extern void show_rrhs(void);
extern void show_shifts(void);

/* main.c */
extern char *allocate(unsigned n);
extern void done(int k) GCC_NORETURN;

/* mkpar.c */
extern void free_parser(void);
extern void make_parser(void);

/* output.c */
extern void output(void);

/* reader.c */
extern void reader(void);

/* skeleton.c */
extern void write_section(const char * const section[], int);

/* verbose.c */
extern void verbose(void);

/* warshall.c */
extern void reflexive_transitive_closure(unsigned *R, int n);

#ifdef NO_LEAKS
extern void lr0_leaks(void);
extern void lalr_leaks(void);
extern void mkpar_leaks(void);
extern void output_leaks(void);
extern void reader_leaks(void);
#endif