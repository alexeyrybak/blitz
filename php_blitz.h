/*
 +----------------------------------------------------------------------+
  | Blitz Templates                                                      |
  +----------------------------------------------------------------------+
  | Downloads and online documentation:                                  |
  |     http://alexeyrybak.com/blitz/                                    |
  |     http://sourceforge.net/projects/blitz-templates/                 |
  |                                                                      |
  | Author:                                                              |
  |     Alexey Rybak [fisher] <alexey.rybak at gmail dot com>            |
  | Bugfixes and patches:                                                |
  |     Antony Dovgal [tony2001] <tony at daylessday dot org>            |
  |     Konstantin Baryshnikov [fixxxer] <konstantin at symbi dot info>  |
  | Mailing list:                                                        |
  |     blitz-php at googlegroups dot com                                |
  +----------------------------------------------------------------------+
*/

/* $Id: php_blitz.h,v 1.25 2012/06/18 15:02:58 fisher Exp $ */

#ifndef PHP_BLITZ_H
#define PHP_BLITZ_H

extern zend_module_entry blitz_module_entry;
#define phpext_blitz_ptr &blitz_module_entry

#ifdef PHP_WIN32
#define PHP_BLITZ_API __declspec(dllexport)
#define BLITZ_USE_STREAMS
#else
#define PHP_BLITZ_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/*****************************************************************************/
/* php interface                                                             */
/*****************************************************************************/

PHP_MINIT_FUNCTION(blitz);
PHP_MSHUTDOWN_FUNCTION(blitz);
PHP_MINFO_FUNCTION(blitz);

ZEND_BEGIN_MODULE_GLOBALS(blitz)
	long  var_prefix;
	char *tag_open;
	char *tag_close;
    char *tag_open_alt;
    char *tag_close_alt;
    char *tag_comment_open;
    char *tag_comment_close;
    char enable_alternative_tags;
    char enable_comments;
    char *path;
    char enable_include;
    char enable_callbacks;
    char enable_php_callbacks;
    char php_callbacks_first;
    char remove_spaces_around_context_tags;
    char warn_context_duplicates;
    char check_recursion;
    unsigned long scope_lookup_limit;
    char lower_case_method_names;
    char auto_escape;
    char throw_exceptions;
ZEND_END_MODULE_GLOBALS(blitz)

#ifdef ZTS
#define BLITZ_G(v) TSRMG(blitz_globals_id, zend_blitz_globals *, v)
#else
#define BLITZ_G(v) (blitz_globals.v)
#endif

/*****************************************************************************/
/* additional headings                                                       */
/*****************************************************************************/
#include <stdio.h>

#define BLITZ_MAX_LIST_SIZE     65535

#define BLITZ_TYPE_VAR      	1	
#define BLITZ_TYPE_METHOD       2	
#define BLITZ_IS_VAR(type)      (type & BLITZ_TYPE_VAR)
#define BLITZ_IS_METHOD(type)   (type & BLITZ_TYPE_METHOD)
#define BLITZ_IS_ARG_EXPR(type) (type & BLITZ_ARG_TYPE_EXPR_SHIFT)

#define BLITZ_ARG_TYPE_VAR		    1
#define BLITZ_ARG_TYPE_VAR_PATH	    2
#define BLITZ_ARG_TYPE_STR          4 
#define BLITZ_ARG_TYPE_NUM          8
#define BLITZ_ARG_TYPE_FALSE        16
#define BLITZ_ARG_TYPE_TRUE         32
#define BLITZ_ARG_TYPE_FLOAT        64
#define BLITZ_ARG_TYPE_EXPR_SHIFT   128
#define BLITZ_EXPR_OPERATOR_GE      (1 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_G       (2 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_LE      (3 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_L       (4 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_NE      (5 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_E       (6 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_LA      (7 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_LO      (8 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_N       (9 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_LP      (10 | BLITZ_ARG_TYPE_EXPR_SHIFT)
#define BLITZ_EXPR_OPERATOR_RP      (11 | BLITZ_ARG_TYPE_EXPR_SHIFT)

#define BLITZ_TAG_VAR_PREFIX    		'$'
#define BLITZ_TAG_VAR_PREFIX_S  		"$"

/* tags to separate HTML from blitz code: all these constants must be changed synchronously! */

#define BLITZ_ENABLE_ALTERNATIVE_TAGS   1
#define BLITZ_ENABLE_COMMENTS           1 

#define BLITZ_TAG_OPEN                  "{{"
#define BLITZ_TAG_CLOSE                 "}}"
#define BLITZ_TAG_OPEN_ALT              "<!--"
#define BLITZ_TAG_CLOSE_ALT             "-->"
#define BLITZ_TAG_COMMENT_OPEN          "/*"
#define BLITZ_TAG_COMMENT_CLOSE          "*/"
#define BLITZ_TAG_LIST_LEN              6

#define BLITZ_TAG_ID_OPEN               0
#define BLITZ_TAG_ID_OPEN_ALT           1
#define BLITZ_TAG_ID_CLOSE              2 
#define BLITZ_TAG_ID_CLOSE_ALT          3
#define BLITZ_TAG_ID_COMMENT_OPEN       4
#define BLITZ_TAG_ID_COMMENT_CLOSE      5

#define BLITZ_ALTERNATIVE_TAGS_ENABLED      1
#define BLITZ_NOBRAKET_FUNCTIONS_ARE_VARS   1

/* tag analizer states */
#define BLITZ_ANALISER_STATE_LIST_LEN       7
#define BLITZ_ANALISER_STATE_NONE           0 
#define BLITZ_ANALISER_STATE_OPEN           1 + BLITZ_TAG_ID_OPEN
#define BLITZ_ANALISER_STATE_CLOSE          1 + BLITZ_TAG_ID_CLOSE
#define BLITZ_ANALISER_STATE_OPEN_ALT       1 + BLITZ_TAG_ID_OPEN_ALT
#define BLITZ_ANALISER_STATE_CLOSE_ALT      1 + BLITZ_TAG_ID_CLOSE_ALT
#define BLITZ_ANALISER_STATE_COMMENT_OPEN   1 + BLITZ_TAG_ID_COMMENT_OPEN
#define BLITZ_ANALISER_STATE_COMMENT_CLOSE  1 + BLITZ_TAG_ID_COMMENT_CLOSE

/* tag analizer actions */
#define BLITZ_ANALISER_ACTION_NONE          0
#define BLITZ_ANALISER_ACTION_ADD           1
#define BLITZ_ANALISER_ACTION_IGNORE_PREV   2 
#define BLITZ_ANALISER_ACTION_ERROR_PREV    3
#define BLITZ_ANALISER_ACTION_IGNORE_CURR   4 
#define BLITZ_ANALISER_ACTION_ERROR_CURR    5
#define BLITZ_ANALISER_ACTION_ERROR_BOTH    6

#define BLITZ_INPUT_BUF_SIZE 	   		4096
#define BLITZ_MAX_FETCH_INDEX_KEY_SIZE  1024
#define BLITZ_ALLOC_TAGS_STEP   		16
#define BLITZ_CALL_ALLOC_ARG_INIT		4
#define BLITZ_NODE_ALLOC_CHILDREN_INIT	4

#define BLITZ_NODE_TYPE_PREDEF_MASK		28
#define BLITZ_IS_PREDEF_METHOD(type)	(type & BLITZ_NODE_TYPE_PREDEF_MASK)

#define BLITZ_STRING_IS_BEGIN(s, len)                                                       \
    ((5 == len) &&                                                                          \
        (('B' == s[0] && 'E' == s[1] && 'G' == s[2] && 'I' == s[3] && 'N' == s[4])          \
        ||                                                                                  \
        ('b' == s[0] && 'e' == s[1] && 'g' == s[2] && 'i' == s[3] && 'n' == s[4]))          \
    )
    
#define BLITZ_STRING_IS_END(s, len)                                                         \
    ((3 == len) &&                                                                          \
        (('E' == s[0] && 'N' == s[1] && 'D' == s[2])                                        \
        ||                                                                                  \
        ('e' == s[0] && 'n' == s[1] && 'd' == s[2]))                                        \
    )

#define BLITZ_STRING_IS_IF(s, len)                                                          \
    ((2 == len) &&                                                                          \
        (('I' == s[0] && 'F' == s[1])                                                       \
        ||                                                                                  \
        ('i' == s[0] && 'f' == s[1]))                                                       \
    )

#define BLITZ_STRING_IS_UNLESS(s, len)                                                                   \
    ((6 == len) &&                                                                                       \
        (('U' == s[0] && 'N' == s[1] && 'L' == s[2] && 'E' == s[3] && 'S' == s[4] && 'S' == s[5])        \
        ||                                                                                               \
        ('u' == s[0] && 'n' == s[1] && 'l' == s[2] && 'e' == s[3] && 's' == s[4] && 's' == s[5]))        \
    )

#define BLITZ_STRING_IS_ELSEIF(s, len)                                                                   \
    ((6 == len) &&                                                                                       \
        (('E' == s[0] && 'L' == s[1] && 'S' == s[2] && 'E' == s[3] && 'I' == s[4] && 'F' == s[5])        \
        ||                                                                                               \
        ('e' == s[0] && 'l' == s[1] && 's' == s[2] && 'e' == s[3] && 'i' == s[4] && 'f' == s[5]))        \
    )

#define BLITZ_STRING_IS_ELSE(s, len)                                                                     \
    ((4 == len) &&                                                                                       \
        (('E' == s[0] && 'L' == s[1] && 'S' == s[2] && 'E' == s[3])                                      \
        ||                                                                                               \
        ('e' == s[0] && 'l' == s[1] && 's' == s[2] && 'e' == s[3]))                                      \
    )

#define BLITZ_STRING_IS_INCLUDE(s, len)                                                                                 \
    ((7 == len) &&                                                                                                      \
        (('I' == s[0] && 'N' == s[1] && 'C' == s[2] && 'L' == s[3] && 'U' == s[4] && 'D' == s[5] && 'E' == s[6])        \
        ||                                                                                                              \
        ('i' == s[0] && 'n' == s[1] && 'c' == s[2] && 'l' == s[3] && 'u' == s[4] && 'd' == s[5] && 'e' == s[6]))        \
    )

#define BLITZ_STRING_IS_ESCAPE(s, len)                                                              \
    (((1 == len) &&                                                                                 \
        (('q' == s[0]) || ('Q' == s[0]))) ||                                                        \
    ((6 == len) &&                                                                                  \
        (('E' == s[0] && 'S' == s[1] && 'C' == s[2] && 'A' == s[3] && 'P' == s[4] && 'E' == s[5])   \
        ||                                                                                          \
        ('e' == s[0] && 's' == s[1] && 'c' == s[2] && 'a' == s[3] && 'p' == s[4] && 'e' == s[5]))   \
    ))

#define BLITZ_STRING_IS_DATE(s, len)                                                        \
    ((4 == len) &&                                                                          \
        (('D' == s[0] && 'A' == s[1] && 'T' == s[2] && 'E' == s[3])                         \
        ||                                                                                  \
        ('d' == s[0] && 'a' == s[1] && 't' == s[2] && 'e' == s[3]))                         \
    )
#define BLITZ_PHP_NAMESPACE_SHIFT   5
#define BLITZ_STRING_PHP_NAMESPACE(s, len)                                                  \
    ((len >= 5) &&                                                                          \
        (('P' == s[0] && 'H' == s[1] && 'P' == s[2] && ':' == s[3] && ':' == s[4])          \
        ||                                                                                  \
        ('p' == s[0] && 'h' == s[1] && 'p' == s[2] && ':' == s[3] && ':' == s[4]))          \
    )

#define BLITZ_THIS_NAMESPACE_SHIFT  6 
#define BLITZ_STRING_THIS_NAMESPACE(s, len)                                                            \
    ((len >= 6) &&                                                                                     \
        (('T' == s[0] && 'H' == s[1] && 'I' == s[2] && 'S' == s[3] && ':' == s[4] && ':' == s[5])      \
        ||                                                                                             \
        ('t' == s[0] && 'h' == s[1] && 'i' == s[2] && 's' == s[3] && ':' == s[4] && ':' == s[5]))      \
    )

#define BLITZ_WRAPPER_MAX_ARGS                  3

// base node types
#define BLITZ_NODE_TYPE_COMMENT                 0
#define BLITZ_NODE_TYPE_IF                      (1 << 2 | BLITZ_TYPE_METHOD) /* {{ if($a, 'b', "c"); }} */
#define BLITZ_NODE_TYPE_UNLESS                  (2 << 2 | BLITZ_TYPE_METHOD) /* {{ unless($a, 'b', "c"); }} */
#define BLITZ_NODE_TYPE_INCLUDE                 (3 << 2 | BLITZ_TYPE_METHOD) /* {{ include('file.tpl'); }} */
#define BLITZ_NODE_TYPE_BEGIN                   (4 << 2 | BLITZ_TYPE_METHOD) /* non-finalized node - will become BLITZ_NODE_TYPE_CONTEXT */
#define BLITZ_NODE_TYPE_END                     (5 << 2 | BLITZ_TYPE_METHOD) /* non-finalized node - will be removed after parsing */
#define BLITZ_NODE_TYPE_CONTEXT                 (6 << 2 | BLITZ_TYPE_METHOD) /* {{ BEGIN a }} bla-bla {{ END }} */
#define BLITZ_NODE_TYPE_CONDITION               (7 << 2 | BLITZ_TYPE_METHOD) /* {{ BEGIN a }} bla-bla {{ END }} */
// reserved +3 base types

#define BLITZ_NODE_TYPE_WRAPPER_ESCAPE          (11 << 2 | BLITZ_TYPE_METHOD) 
#define BLITZ_NODE_TYPE_WRAPPER_DATE            (12 << 2 | BLITZ_TYPE_METHOD) 
#define BLITZ_NODE_TYPE_WRAPPER_UPPER           (13 << 2 | BLITZ_TYPE_METHOD) 
#define BLITZ_NODE_TYPE_WRAPPER_LOWER           (14 << 2 | BLITZ_TYPE_METHOD) 
#define BLITZ_NODE_TYPE_WRAPPER_TRIM            (15 << 2 | BLITZ_TYPE_METHOD) 
// reserved +5 wrappers
// put all other wrappers here with codes less than BLITZ_NODE_TYPE_IF_NF, because 
// wrapper check in blitz.c is type >= BLITZ_NODE_TYPE_WRAPPER_ESCAPE &&  type < BLITZ_NODE_TYPE_IF_NF

#define BLITZ_NODE_TYPE_IF_NF                   (21<< 2 | BLITZ_TYPE_METHOD) /* non-finalized -> IF_CONTEXT */
#define BLITZ_NODE_TYPE_UNLESS_NF               (22<< 2 | BLITZ_TYPE_METHOD) /* non-finalized -> UNLESS_CONTEXT */
#define BLITZ_NODE_TYPE_IF_CONTEXT              (23<< 2 | BLITZ_TYPE_METHOD) /* {{ IF $a }} bla-bla ... */
#define BLITZ_NODE_TYPE_UNLESS_CONTEXT          (24<< 2 | BLITZ_TYPE_METHOD) /* {{ UNLESS $a }} bla-bla ... */
#define BLITZ_NODE_TYPE_ELSEIF_NF               (25<< 2 | BLITZ_TYPE_METHOD) /* non-finalized -> ELSEIF_CONTEXT */
#define BLITZ_NODE_TYPE_ELSEIF_CONTEXT          (26<< 2 | BLITZ_TYPE_METHOD) /* ... {{ ELSEIF $a }} bla-bla {{ END }} */
#define BLITZ_NODE_TYPE_ELSE_NF                 (27<< 2 | BLITZ_TYPE_METHOD) /* non-finalized -> ELSE_CONTEXT */
#define BLITZ_NODE_TYPE_ELSE_CONTEXT            (28<< 2 | BLITZ_TYPE_METHOD) /* ... {{ ELSE }} bla-bla {{ END }} */

#define BLITZ_NODE_TYPE_VAR                     (1<<2 | BLITZ_TYPE_VAR)  /* $a */
#define BLITZ_NODE_TYPE_VAR_PATH                (2<<2 | BLITZ_TYPE_VAR)  /* $hash.key or $obj.property or $obj.property_hash.key et cetera */

#define BLITZ_NODE_PATH_NON_UNIQUE              0
#define BLITZ_NODE_PATH_UNIQUE                  1

#define BLITZ_ITPL_ALLOC_INIT			        4

#define BLITZ_TMP_BUF_MAX_LEN                   1024
#define BLITZ_CONTEXT_PATH_MAX_LEN              1024

#define BLITZ_NODE_NO_NAMESPACE                 0
#define BLITZ_NODE_THIS_NAMESPACE               1
#define BLITZ_NODE_PHP_NAMESPACE                2
#define BLITZ_NODE_CUSTOM_NAMESPACE             3

#define BLITZ_FLAG_FETCH_INDEX_BUILT            1
#define BLITZ_FLAG_GLOBALS_IS_OTHER             2
#define BLITZ_FLAG_ITERATION_IS_OTHER           4
#define BLITZ_FLAG_CALLED_USER_METHOD           8

#define BLITZ_CALLED_USER_METHOD(v) (((v)->flags & BLITZ_FLAG_CALLED_USER_METHOD) == BLITZ_FLAG_CALLED_USER_METHOD)

#define BLITZ_LOOP_STACK_MAX    32
#define BLITZ_SCOPE_STACK_MAX   128
#define BLITZ_IF_STACK_MAX      32

#define BLITZ_ESCAPE_DEFAULT    0
#define BLITZ_ESCAPE_NO         1
#define BLITZ_ESCAPE_YES        2
#define BLITZ_ESCAPE_NL2BR      3

#define BLITZ_CONSTANT_FLAGS (CONST_CS | CONST_PERSISTENT)

/* simple string with length */
typedef struct {
    char *s;
    unsigned long len;
} blitz_string;

/* simple list */
typedef struct {
    char *first;
    char *end;
    unsigned long size;
    unsigned long item_size;
    unsigned long allocated;
} blitz_list;

/* tag: position and id */
typedef struct {
    unsigned long pos;
    unsigned char tag_id;
} tag_pos;

/* call argument */
typedef struct {
    char *name;
    unsigned long len;
    unsigned char type;
} call_arg;

/* template node */
typedef struct _blitz_node {
    unsigned long pos_begin;
    unsigned long pos_end;
    unsigned long pos_begin_shift;
    unsigned long pos_end_shift;
    unsigned char type;
    unsigned char escape_mode;
    char hidden;
    char namespace_code;
    char lexem[BLITZ_MAX_LEXEM_LEN]; 
    unsigned long lexem_len;
    call_arg *args;
    unsigned char n_args;
    struct _blitz_node *first_child;
    struct _blitz_node *next;
    unsigned int pos_in_list;
} blitz_node;

/* loop stack item */
typedef struct _blitz_loop_stack_item {
    unsigned int current;
    unsigned int total;
} blitz_loop_stack_item;

/* template: "static" data - related to body, tags etc */
typedef struct _blitz_static_data {
    char name[MAXPATHLEN];
    struct _blitz_node *nodes;
    unsigned int n_nodes;
    char *body;
    unsigned long body_len;
    HashTable *fetch_index;
    unsigned int tag_open_len;
    unsigned int tag_close_len;
    unsigned int tag_open_alt_len;
    unsigned int tag_close_alt_len;
    unsigned int tag_comment_open_len;
    unsigned int tag_comment_close_len;
} blitz_static_data;

#define BLITZ_ERROR_MAX_LEN     1024
/* template: "static" and "dynamic" parts */
typedef struct _blitz_tpl {
    struct _blitz_static_data static_data;
    char flags;
    HashTable *hash_globals;
    zval *iterations;
    zval **current_iteration;  /* current iteraion values */
    zval **last_iteration;     /* latest iteration - used in combined iterate+set methods  */
    zval **current_iteration_parent; /* list of current context iterations (current_iteration is last element in this list) */
    zval **caller_iteration;  /* caller iteraion */
    char *current_path;
    char *tmp_buf;
    HashTable *ht_tpl_name; /* index template_name -> itpl_list number */
    struct _blitz_tpl **itpl_list; /* list of included templates */
    unsigned int itpl_list_alloc;
    unsigned int itpl_list_len;
    unsigned int loop_stack_level;
    struct _blitz_tpl *tpl_parent; /* parent template for included */
    struct _blitz_loop_stack_item loop_stack[BLITZ_LOOP_STACK_MAX]; 
    zval *scope_stack[BLITZ_SCOPE_STACK_MAX];
    unsigned int scope_stack_pos;
    char *error;
} blitz_tpl;

#define BLITZ_ANALIZER_NODE_STACK_LEN    64
/* template analizer: node stack */
typedef struct _blitz_analizer_stack_elem {
    struct _blitz_node *parent;
    struct _blitz_node *last;
} analizer_stack_elem;

/* template analizer: context */
typedef struct _blitz_analizer_ctx {
    unsigned int n_nodes;
    unsigned int n_open;
    unsigned int n_close;
    unsigned char action;
    unsigned char state;
    unsigned long pos_open;
    tag_pos *tag;
    tag_pos *prev_tag;
    struct _blitz_tpl *tpl;
    struct _blitz_analizer_stack_elem node_stack[BLITZ_ANALIZER_NODE_STACK_LEN];
    unsigned int node_stack_len;
    struct _blitz_node *node;
    unsigned long current_open; 
    unsigned long current_close; 
    unsigned long close_len; 
    unsigned int true_lexem_len;
    unsigned int n_needs_end;
    unsigned int n_actual_end;
} analizer_ctx;

/* call scanner */
#define BLITZ_ISNUM_MIN                48  /* '0' */
#define BLITZ_ISNUM_MAX                57  /* '9' */
#define BLITZ_ISALPHA_SMALL_MIN        97  /* 'a' */
#define BLITZ_ISALPHA_SMALL_MAX        122 /* 'z' */
#define BLITZ_ISALPHA_BIG_MIN          65  /* 'A' */
#define BLITZ_ISALPHA_BIG_MAX          90  /* 'Z' */

#define BLITZ_SKIP_BLANK(c,len,pos)                                            \
    len = 0;                                                                   \
    while(isspace(c[len])) len++;                                              \
    c+=len; pos+=len;                                                          \


#define BLITZ_SKIP_BLANK_END(c,len,pos)                                        \
    len = 0;                                                                   \
    while(isspace(c[len]) len++;                                               \
    c+=len; pos+=len;                                                          \


#define BLITZ_IS_NUMBER(c)                                                     \
    (                                                                          \
        ((c)>=BLITZ_ISNUM_MIN && (c)<=BLITZ_ISNUM_MAX)                         \
        || (c) == '-'                                                          \
    )

#define BLITZ_IS_ALPHA(c)                                                      \
  ( ((c)>=BLITZ_ISALPHA_SMALL_MIN && (c)<=BLITZ_ISALPHA_SMALL_MAX)             \
    || ((c)>=BLITZ_ISALPHA_BIG_MIN && (c)<=BLITZ_ISALPHA_BIG_MAX)              \
    || (c) == '_'                                                              \
  )

#define BLITZ_IS_ALPHA_STRICT(c)                                               \
  ( ((c)>=BLITZ_ISALPHA_SMALL_MIN && (c)<=BLITZ_ISALPHA_SMALL_MAX)             \
    || ((c)>=BLITZ_ISALPHA_BIG_MIN && (c)<=BLITZ_ISALPHA_BIG_MAX)              \
  )

#define BLITZ_IS_OPERATOR(c)                                                   \
  ( (c) == '=' || (c) == '>' || (c) == '<' || (c) == '&' || (c) == '|'         \
    || (c) == '!' || (c) == '(' || (c) == ')'                                  \
  )

#define BLITZ_OPERATOR_HAS_PRECEDENCE(a, b)                                    \
  ( BLITZ_OPERATOR_GET_PRECEDENCE(a) <= BLITZ_OPERATOR_GET_PRECEDENCE(b)       \
  )

#define BLITZ_OPERATOR_GET_PRECEDENCE(c)                                       \
  (                                                                            \
    ((c) == BLITZ_EXPR_OPERATOR_LP || (c) == BLITZ_EXPR_OPERATOR_RP) ? 0 :     \
    ((c) == BLITZ_EXPR_OPERATOR_N) ? 1 :                                       \
    ((c) == BLITZ_EXPR_OPERATOR_LE || (c) == BLITZ_EXPR_OPERATOR_L             \
     || (c) == BLITZ_EXPR_OPERATOR_GE || (c) == BLITZ_EXPR_OPERATOR_G) ? 2 :   \
    ((c) == BLITZ_EXPR_OPERATOR_E || (c) == BLITZ_EXPR_OPERATOR_NE) ? 3 :      \
    ((c) == BLITZ_EXPR_OPERATOR_LA) ? 4 :                                      \
    ((c) == BLITZ_EXPR_OPERATOR_LO) ? 5 :                                      \
    6                                                                          \
  )

#define BLITZ_OPERATOR_GET_NUM_OPERANDS(c)                                     \
  ( (c) == BLITZ_EXPR_OPERATOR_N ? 1 : 2                                       \
  )

#define BLITZ_OPERATOR_TO_STRING(c)                                            \
  ( (c) == BLITZ_EXPR_OPERATOR_GE ? ">=" :                                     \
    (c) == BLITZ_EXPR_OPERATOR_G ? ">" :                                       \
    (c) == BLITZ_EXPR_OPERATOR_LE ? "<=" :                                     \
    (c) == BLITZ_EXPR_OPERATOR_L ? "<" :                                       \
    (c) == BLITZ_EXPR_OPERATOR_NE ? "!=" :                                     \
    (c) == BLITZ_EXPR_OPERATOR_E ? "==" :                                      \
    (c) == BLITZ_EXPR_OPERATOR_LA ? "&&" :                                     \
    (c) == BLITZ_EXPR_OPERATOR_LO ? "||" :                                     \
    (c) == BLITZ_EXPR_OPERATOR_N ? "!" :                                       \
    (c) == BLITZ_EXPR_OPERATOR_LP ? "(" :                                      \
    (c) == BLITZ_EXPR_OPERATOR_RP ? ")" : "<UNKNOWN>"                          \
  )

#define BLITZ_SCAN_SINGLE_QUOTED(c, p, pos, len, ok)                           \
    was_escaped = 0;                                                           \
    ok = 0;                                                                    \
    while(*(c)) {                                                              \
        if (*(c) == '\'' && !was_escaped) {                                    \
            ok = 1;                                                            \
            break;                                                             \
        } else if (*(c) == '\\' && !was_escaped) {                             \
            was_escaped = 1;                                                   \
        } else {                                                               \
            *(p) = *(c);                                                       \
            ++(p);                                                             \
            ++(len);                                                           \
            if (was_escaped) was_escaped = 0;                                  \
        }                                                                      \
        (c)++;                                                                 \
        (pos)++;                                                               \
    }                                                                          \
    *(p) = '\x0';

#define BLITZ_SCAN_DOUBLE_QUOTED(c, p, pos, len, ok)                           \
    was_escaped = 0;                                                           \
    ok = 0;                                                                    \
    while(*(c)) {                                                              \
        if (*(c) == '\"' && !was_escaped) {                                    \
            ok = 1;                                                            \
            break;                                                             \
        } else if (*(c) == '\\' && !was_escaped) {                             \
            was_escaped = 1;                                                   \
        } else {                                                               \
            *(p) = *(c);                                                       \
            ++(p);                                                             \
            ++(len);                                                           \
            if (was_escaped) was_escaped = 0;                                  \
        }                                                                      \
        ++(c);                                                                 \
        ++(pos);                                                               \
    }                                                                          \
    *(p) = '\x0';

#define BLITZ_SCAN_NUMBER(c, p, pos, symb, has_dot)                                     \
    while(((symb) = *(c)) && (BLITZ_IS_NUMBER(symb) || (symb == '.' && !has_dot))) {    \
        if (symb == '.') has_dot = 1;                                                   \
        *(p) = symb;                                                                    \
        ++(p);                                                                          \
        ++(c);                                                                          \
        ++(pos);                                                                        \
    }                                                                                   \
    *(p) = '\x0';

#define BLITZ_SCAN_ALNUM(c, p, pos, symb)                                                \
    while(((symb) = *(c)) && (BLITZ_IS_NUMBER(symb) || BLITZ_IS_ALPHA(symb))) {          \
        *(p) = symb;                                                                     \
        ++(p);                                                                           \
        ++(c);                                                                           \
        ++(pos);                                                                         \
    }                                                                                    \
    *(p) = '\x0';

#define BLITZ_SCAN_VAR(c, p, pos, symb, is_path)                                                   \
    while(((symb) = *(c)) &&                                                                       \
        (BLITZ_IS_NUMBER(symb) || BLITZ_IS_ALPHA(symb) || (symb == '.' && (is_path = 1)))) {       \
        *(p) = symb;                                                                               \
        ++(p);                                                                                     \
        ++(c);                                                                                     \
        ++(pos);                                                                                   \
    }                                                                                              \
    *(p) = '\x0';

#define BLITZ_SCAN_VAR_NOCOPY(c, len, symb, is_path)                                               \
    while(((symb) = *(c)) &&                                                                       \
        (BLITZ_IS_NUMBER(symb) || BLITZ_IS_ALPHA(symb) || (symb == '.' && (is_path = 1)))) {       \
        ++(c);                                                                                     \
        ++(len);                                                                                   \
    }                                                                                              \

#define BLITZ_SCAN_NAMESPACE_NOCOPY(c, len, symb, is_path)                                         \
    while(((symb) = *(c)) &&                                                                       \
        (BLITZ_IS_NUMBER(symb) || BLITZ_IS_ALPHA(symb) || (symb == '\\'))) {                       \
        ++(c);                                                                                     \
        ++(len);                                                                                   \
    }                                                                                              \


#define BLITZ_SCAN_EXPR_OPERATOR(c, i_len, i_type)                                                  \
    if (*(c) == '>') {                                                                              \
        if (((c) + 1) && (*((c) + 1) == '=')) {                                                     \
            i_type = BLITZ_EXPR_OPERATOR_GE;                                                        \
            i_len = 2;                                                                              \
        } else {                                                                                    \
            i_type = BLITZ_EXPR_OPERATOR_G;                                                         \
            i_len = 1;                                                                              \
        }                                                                                           \
    } else if (*(c) == '<') {                                                                       \
        if (((c) + 1) && (*((c) + 1) == '=')) {                                                     \
            i_type = BLITZ_EXPR_OPERATOR_LE;                                                        \
            i_len = 2;                                                                              \
        } else {                                                                                    \
            i_type = BLITZ_EXPR_OPERATOR_L;                                                         \
            i_len = 1;                                                                              \
        }                                                                                           \
    } else if (*(c) == '!') {                                                                       \
        if (((c) + 1) && (*((c) + 1) == '=')) {                                                     \
            i_type = BLITZ_EXPR_OPERATOR_NE;                                                        \
            i_len = 2;                                                                              \
        } else {                                                                                    \
            i_type = BLITZ_EXPR_OPERATOR_N;                                                         \
            i_len = 1;                                                                              \
        }                                                                                           \
    } else if ((*(c) == '=') && ((c) + 1) && (*((c) + 1) == '=')) {                                 \
        i_type = BLITZ_EXPR_OPERATOR_E;                                                             \
        i_len = 2;                                                                                  \
    } else if ((*(c) == '&') && ((c) + 1) && (*((c) + 1) == '&')) {                                 \
        i_type = BLITZ_EXPR_OPERATOR_LA;                                                            \
        i_len = 2;                                                                                  \
    } else if ((*(c) == '|') && ((c) + 1) && (*((c) + 1) == '|')) {                                 \
        i_type = BLITZ_EXPR_OPERATOR_LO;                                                            \
        i_len = 2;                                                                                  \
    } else if (*(c) == '(') {                                                                       \
        i_type = BLITZ_EXPR_OPERATOR_LP;                                                            \
        i_len = 1;                                                                                  \
    } else if (*(c) == ')') {                                                                       \
        i_type = BLITZ_EXPR_OPERATOR_RP;                                                            \
        i_len = 1;                                                                                  \
    }                                                                                               \

#define BLITZ_CALL_STATE_NEXT_ARG    1
#define BLITZ_CALL_STATE_FINISHED    2
#define BLITZ_CALL_STATE_HAS_NEXT    3 
#define BLITZ_CALL_STATE_BEGIN       4
#define BLITZ_CALL_STATE_END         5
#define BLITZ_CALL_STATE_IF          6
#define BLITZ_CALL_STATE_ELSE        7
#define BLITZ_CALL_STATE_ERROR       0

#define BLITZ_CALL_ERROR             1
#define BLITZ_CALL_ERROR_IF          2
#define BLITZ_CALL_ERROR_INCLUDE     3
#define BLITZ_CALL_ERROR_IF_CONTEXT  4

#define BLITZ_ZVAL_NOT_EMPTY(z, res)                                                              \
    switch (Z_TYPE_PP(z)) {                                                                       \
        case IS_BOOL: res = (0 == Z_LVAL_PP(z)) ? 0 : 1; break;                                   \
        case IS_STRING:                                                                           \
            if (0 == Z_STRLEN_PP(z)) {                                                            \
                res = 0;                                                                          \
            } else if ((1 == Z_STRLEN_PP(z)) && (Z_STRVAL_PP(z)[0] == '0')) {                     \
                res = 0;                                                                          \
            } else {                                                                              \
                res = 1;                                                                          \
            }                                                                                     \
            break;                                                                                \
        case IS_LONG: res = (0 == Z_LVAL_PP(z)) ? 0 : 1; break;                                   \
        case IS_DOUBLE: res = (.0 == Z_DVAL_PP(z)) ? 0 : 1; break;                                \
        case IS_ARRAY: res = (0 == zend_hash_num_elements(Z_ARRVAL_PP(z))) ? 0 : 1; break;        \
        case IS_OBJECT: res = (0 == zend_hash_num_elements(Z_OBJPROP_PP(z))) ? 0 : 1; break;      \
                                                                                                  \
        default: res = 0; break;                                                                  \
    }

#define BLITZ_ARG_NOT_EMPTY(a, ht, res)                                                           \
    if ((a).type == BLITZ_ARG_TYPE_STR) {                                                         \
        if (((a).len == 1) && ((a).name[0] == '0')) {                                             \
            (res) = 0;                                                                            \
        } else {                                                                                  \
            (res) = (a).len > 0;                                                                  \
        }                                                                                         \
    } else if ((a).type == BLITZ_ARG_TYPE_NUM) {                                                  \
        (res) = (0 == strncmp((a).name, "0", 1)) ? 0 : 1;                                         \
    } else if (((a).type == BLITZ_ARG_TYPE_VAR) && ht) {                                          \
        zval **z;                                                                                 \
        if((a).name && (a).len > 0) {                                                             \
            if (SUCCESS == zend_hash_find(ht, (a).name, 1 + (a).len, (void**)&z))                 \
            {                                                                                     \
                BLITZ_ZVAL_NOT_EMPTY(z, res)                                                      \
            } else {                                                                              \
                res = -1;                                                                         \
            }                                                                                     \
        } else {                                                                                  \
            res = 0;                                                                              \
        }                                                                                         \
    } else if((a).type == BLITZ_ARG_TYPE_FALSE) {                                                 \
        res = 0;                                                                                  \
    } else if((a).type == BLITZ_ARG_TYPE_TRUE) {                                                  \
        res = 1;                                                                                  \
    } else {                                                                                      \
        res = 0;                                                                                  \
    }

#define BLITZ_GET_ARG_ZVAL(a, ht, z)                                                              \
    if (((a).type == BLITZ_ARG_TYPE_VAR) && ht) {                                                 \
        if ((a).name && (a).len>0) {                                                              \
            zend_hash_find(ht, (a).name, 1 + (a).len, (void**)&z);                                \
        }                                                                                         \
    }                                                                                             

#define BLITZ_HASH_FIND_P(z, k, k_len, out)                                                       \
    ( (Z_TYPE_P(z) == IS_ARRAY) ? zend_hash_find(Z_ARRVAL_P(z), k, k_len, (void **)out) :         \
       ( (Z_TYPE_P(z) == IS_OBJECT) ? zend_hash_find(Z_OBJPROP_P(z), k, k_len, (void **)out) :    \
          FAILURE                                                                                 \
       )                                                                                          \
    )

#define BLITZ_REALLOC_RESULT(blen,nlen,rlen,alen,res,pres)                                        \
    (nlen) = (rlen) + (blen);                                                                     \
    if ((rlen) < (nlen)) {                                                                        \
        while ((alen) < (nlen)) {                                                                 \
            (alen) <<= 1;                                                                         \
        }                                                                                         \
        (res) = erealloc((res),(alen + 1)*sizeof(char));                                          \
        (pres) = (res) + (rlen);                                                                  \
    }                                                                                             \

#define BLITZ_FETCH_TPL_RESOURCE(id,tpl,desc)                                                     \
    if (((id) = getThis()) == 0) {                                                                \
        RETURN_FALSE;                                                                             \
    }                                                                                             \
                                                                                                  \
    if (zend_hash_find(Z_OBJPROP_P((id)), "tpl", sizeof("tpl"), (void **)&(desc)) == FAILURE) {   \
        php_error_docref(NULL TSRMLS_CC, E_WARNING,                                               \
            "INTERNAL: template was not loaded/initialized (cannot find template descriptor)"     \
        );                                                                                        \
        RETURN_FALSE;                                                                             \
    }                                                                                             \
                                                                                                  \
    ZEND_FETCH_RESOURCE(tpl, blitz_tpl *, desc, -1, "blitz template", le_blitz);                  \
    if(!tpl) {                                                                                    \
        RETURN_FALSE;                                                                             \
    }


#define BLITZ_PREDEFINED_BUF_LEN    16
/* loop stack tricks */
#define BLITZ_LOOP_MOVE_FORWARD(tpl)                                                              \
    if (tpl->loop_stack_level>=BLITZ_LOOP_STACK_MAX) {                                            \
        php_error_docref(NULL TSRMLS_CC, E_WARNING,                                               \
            "INTERNAL ERROR: loop stack limit (%u) was exceeded, recompile blitz "                \
            "with BLITZ_LOOP_STACK_MAX increased", BLITZ_LOOP_STACK_MAX);                         \
    } else {                                                                                      \
        tpl->loop_stack_level++;                                                                  \
    }                                                                                             \
    tpl->loop_stack[tpl->loop_stack_level].current = 0;                                           \
    tpl->loop_stack[tpl->loop_stack_level].total = 0;                                               

#define BLITZ_LOOP_MOVE_BACK(tpl)                                                                 \
    if (tpl->loop_stack_level>0) {                                                                \
        tpl->loop_stack_level--;                                                                  \
    }

#define BLITZ_LOOP_INIT(tpl, num)           tpl->loop_stack[tpl->loop_stack_level].total = (num);
#define BLITZ_LOOP_INCREMENT(tpl)           tpl->loop_stack[tpl->loop_stack_level].current++

#define BLITZ_SCOPE_STACK_PUSH(tpl, data)                                                         \
    if (tpl->scope_stack_pos < BLITZ_SCOPE_STACK_MAX) {                                           \
        tpl->scope_stack[tpl->scope_stack_pos] = data;                                            \
        tpl->scope_stack_pos++;                                                                   \
    } else {                                                                                      \
        php_error_docref(NULL TSRMLS_CC, E_WARNING,                                               \
            "Too deep iteration set, lookup scope depth is too high, lookup stack is broken "     \
            "and variables can be resolved improperly. To fix this rebuild blitz extension with " \
            "increased BLITZ_SCOPE_STACK_MAX constant in php_blitz.h"                             \
        );                                                                                        \
    }                                                                                             \

#define BLITZ_SCOPE_STACK_UPDATE(tpl, data)                                                       \
    tpl->scope_stack[tpl->scope_stack_pos - 1] = data;                                            \

#define BLITZ_SCOPE_STACK_SHIFT(tpl)                                                              \
    if (tpl->scope_stack_pos > 0) {                                                               \
        tpl->scope_stack_pos--;                                                                   \
    } else {                                                                                      \
        php_error_docref(NULL TSRMLS_CC, E_WARNING,                                               \
            "Too deep iteration set, lookup scope depth is too high, lookup stack is broken "     \
            "and variables can be resolved improperly. To fix this rebuild blitz extension with " \
            "increased BLITZ_SCOPE_STACK_MAX constant in php_blitz.h"                             \
        );                                                                                        \
    }                                                                                             \

#define BLITZ_IF_STACK_PUSH(stack, stack_level, argument)                                         \
    if ((stack_level + 1) < BLITZ_IF_STACK_MAX) {                                                 \
        ++stack_level;                                                                            \
        stack[stack_level] = argument;                                                            \
    } else {                                                                                      \
        php_error_docref(NULL TSRMLS_CC, E_WARNING,                                               \
            "Too complex conditional, operator stack depth is too high and broken, operators "    \
            "will  be resolved improperly. To fix this rebuild blitz extension with increased "   \
            "BLITZ_IF_STACK_MAX constant in php_blitz.h"                                          \
        );                                                                                        \
    }                                                                                             \

#define BLITZ_EXPR_STACK_PUSH(stack, stack_level, aname, alen, atype)                             \
    if ((stack_level + 1) < BLITZ_IF_STACK_MAX) {                                                 \
        ++stack_level;                                                                            \
        stack[stack_level].name = (aname);                                                        \
        stack[stack_level].len = (alen);                                                          \
        stack[stack_level].type = (atype);                                                        \
    } else {                                                                                      \
        php_error_docref(NULL TSRMLS_CC, E_WARNING,                                               \
            "Too complex conditional, operator stack depth is too high and broken, operators "    \
            "will  be resolved improperly. To fix this rebuild blitz extension with increased "   \
            "BLITZ_IF_STACK_MAX constant in php_blitz.h"                                          \
        );                                                                                        \
    }                                                                                             \

#define BLITZ_GET_PREDEFINED_VAR_EX(tpl, n, len, value, stack_level)                                                   \
    if (len == 0 || n[0] != '_') {                                                                                     \
        value = -1;                                                                                                    \
    } else {                                                                                                           \
        unsigned int current = tpl->loop_stack[stack_level].current;                                                   \
        if (len == 5 && n[1] == 'e' && n[2] == 'v' && n[3] == 'e' && n[4] == 'n') {                                    \
            value = !(current % 2 );                                                                                   \
        } else if (len == 4 && n[1] == 'o' && n[2] == 'd' && n[3] == 'd') {                                            \
            value = current % 2;                                                                                       \
        } else if (len == 6 && n[1] == 'f' && n[2] == 'i' && n[3] == 'r' && n[4] == 's' && n[5] == 't') {              \
            value = (0 == current);                                                                                    \
        } else if (len == 5 && n[1] == 'l' && n[2] == 'a' && n[3] == 's' && n[4] == 't') {                             \
            value = (current + 1 == tpl->loop_stack[stack_level].total);                                               \
        } else if (len == 2 && n[1] == 'i') {                                                                          \
            value = current;                                                                                           \
        } else if (len == 4 && n[1] == 'n' && n[2] == 'u' && n[3] == 'm') {                                            \
            value = current + 1;                                                                                       \
        } else if (len == 6 && n[1] == 't' && n[2] == 'o' && n[3] == 't' && n[4] == 'a' && n[5] == 'l') {              \
            value = tpl->loop_stack[stack_level].total;                                                                \
        }                                                                                                              \
    }

#define BLITZ_GET_PREDEFINED_VAR(tpl, n, len, value)                                                                   \
	BLITZ_GET_PREDEFINED_VAR_EX(tpl, n, len, value, tpl->loop_stack_level)

#define BLITZ_IS_PREDEFINED_TOP(s, l)                              \
    (l == 4) && (                                                  \
        (s[0] == '_' && s[1] == 't' && s[2] == 'o' && s[3] == 'p') \
        ||                                                         \
        (s[0] == '_' && s[1] == 'T' && s[2] == 'O' && s[3] == 'P') \
    )

#define BLITZ_IS_PREDEFINED_PARENT(s, l)                                                                        \
    (l == 7) && (                                                                                               \
        (s[0] == '_' && s[1] == 'p' && s[2] == 'a' && s[3] == 'r' && s[4] == 'e' && s[5] == 'n' && s[6] == 't') \
        ||                                                                                                      \
        (s[0] == '_' && s[1] == 'P' && s[2] == 'A' && s[3] == 'R' && s[4] == 'E' && s[5] == 'N' && s[6] == 'T') \
    )

#define BLITZ_STRING_IS_TRUE(s, l) \
    (l == 4) && ( \
        (s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') \
        || \
        (s[0] == 'T' && s[1] == 'R' && s[2] == 'U' && s[3] == 'E') \
    )

#define BLITZ_STRING_IS_FALSE(s, l) \
    (l == 5) && ( \
        (s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') \
        || \
        (s[0] == 'F' && s[1] == 'A' && s[2] == 'L' && s[3] == 'S' && s[4] == 'E') \
    )

#define TIMER(ts,tz,tdata,tnum)                         \
    gettimeofday(&(ts),&(tz));                          \
    tdata[tnum] = 1000000*ts.tv_sec + ts.tv_usec;       \
    tnum++;

#define TIMER_ADD(ts,tz,tdata,tnum)                     \
    gettimeofday(&(ts),&(tz));                          \
    tdata[tnum] += 1000000*ts.tv_sec + ts.tv_usec;      \

#endif	/* PHP_BLITZ_H */
