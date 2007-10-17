/*
  +----------------------------------------------------------------------+
  | Authors: Alexey Rybak <alexey.rybak@gmail.com>,                      |
  |          downloads and online documentation:                         |
  |              - http://alexeyrybak.com/blitz/                         |
  |              - http://sourceforge.net/projects/blitz-templates/      |
  |                                                                      |
  |          Template analyzing is based on php_templates code by        |
  |          Maxim Poltarak (http://php-templates.sf.net)                |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

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

// php interface
PHP_MINIT_FUNCTION(blitz);
PHP_MSHUTDOWN_FUNCTION(blitz);
PHP_MINFO_FUNCTION(blitz);

ZEND_BEGIN_MODULE_GLOBALS(blitz)
	long  var_prefix;
	char *node_open;
	char *node_close;
    char *phpt_ctx_left;
    char *phpt_ctx_right;
    char *path;
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

#define BLITZ_TYPE_VAR      	1	
#define BLITZ_TYPE_METHOD       2	
#define BLITZ_IS_VAR(type)      (type & BLITZ_TYPE_VAR)
#define BLITZ_IS_METHOD(type)   (type & BLITZ_TYPE_METHOD)

#define BLITZ_ARG_TYPE_VAR		    1
#define BLITZ_ARG_TYPE_VAR_PATH	    2
#define BLITZ_ARG_TYPE_STR          4
#define BLITZ_ARG_TYPE_NUM          8
#define BLITZ_ARG_TYPE_BOOL         16

#define BLITZ_TAG_VAR_PREFIX    		'$'
#define BLITZ_TAG_VAR_PREFIX_S  		"$"
#define BLITZ_TAG_OPEN_DEFAULT 			"{{"
#define BLITZ_TAG_CLOSE_DEFAULT	 		"}}"
#define BLITZ_TAG_PHPT_CTX_LEFT         "<!-- "
#define BLITZ_TAG_PHPT_CTX_RIGHT        " -->"

#define BLITZ_MAX_LEXEM_LEN    			1024
#define BLITZ_INPUT_BUF_SIZE 	   		4096
#define BLITZ_ALLOC_TAGS_STEP   		16
#define BLITZ_CALL_ALLOC_ARG_INIT		4
#define BLITZ_NODE_ALLOC_CHILDREN_INIT	4

#define BLITZ_NODE_TYPE_PREDEF_MASK		28
#define BLITZ_IS_PREDEF_METHOD(type)	(type & BLITZ_NODE_TYPE_PREDEF_MASK)
#define BLITZ_NODE_TYPE_IF_S			"if"
#define BLITZ_NODE_TYPE_INCLUDE_S		"include"
#define BLITZ_NODE_TYPE_BEGIN_S			"begin"
#define BLITZ_NODE_TYPE_BEGIN_SU		"BEGIN"
#define BLITZ_NODE_TYPE_END_S  			"end"
#define BLITZ_NODE_TYPE_END_SU 			"END"

#define BLITZ_TAG_IS_BEGIN(s)                               \
    (((0 == strcmp(s,BLITZ_NODE_TYPE_BEGIN_SU))        \
          || (0 == strcmp(s,BLITZ_NODE_TYPE_BEGIN_S))))

#define BLITZ_TAG_IS_END(s)                                 \
    (((0 == strcmp(s,BLITZ_NODE_TYPE_END_SU))          \
          || (0 == strcmp(s,BLITZ_NODE_TYPE_END_S))))

#define BLITZ_TAG_IS_BEGIN_OR_END(s)                        \
    ((BLITZ_TAG_IS_BEGIN(s) || BLITZ_TAG_IS_END(s)))

#define BLITZ_WRAPPER_MAX_ARGS          3
#define BLITZ_NODE_TYPE_WRAPPER_ESCAPE_S  "escape"
#define BLITZ_NODE_TYPE_WRAPPER_DATE_S    "date"

#define BLITZ_NODE_TYPE_IF              (1<<2 | BLITZ_TYPE_METHOD)
#define BLITZ_NODE_TYPE_INCLUDE         (2<<2 | BLITZ_TYPE_METHOD)
#define BLITZ_NODE_TYPE_BEGIN           (3<<2 | BLITZ_TYPE_METHOD) /* non-finalized node - will become BLITZ_NODE_TYPE_CONTEXT */
#define BLITZ_NODE_TYPE_END             (4<<2 | BLITZ_TYPE_METHOD) /* non-finalized node - will be removed after parsing */
#define BLITZ_NODE_TYPE_CONTEXT         (5<<2 | BLITZ_TYPE_METHOD)
#define BLITZ_NODE_TYPE_WRAPPER         (6<<2 | BLITZ_TYPE_METHOD)

#define BLITZ_NODE_TYPE_VAR             (1<<2 | BLITZ_TYPE_VAR)
#define BLITZ_NODE_TYPE_VAR_PATH        (2<<2 | BLITZ_TYPE_VAR)

#define BLITZ_NODE_TYPE_WRAPPER_ESCAPE  1
#define BLITZ_NODE_TYPE_WRAPPER_DATE    2
#define BLITZ_NODE_TYPE_WRAPPER_UPPER   3
#define BLITZ_NODE_TYPE_WRAPPER_LOWER   4
#define BLITZ_NODE_TYPE_WRAPPER_TRIM    5

#define BLITZ_ITPL_ALLOC_INIT			4	

#define BLITZ_TMP_BUF_MAX_LEN           1024
#define BLITZ_CONTEXT_PATH_MAX_LEN      1024
#define BLITZ_FILE_PATH_MAX_LEN         1024

#define BLITZ_FLAG_FETCH_INDEX_BUILT    1
#define BLITZ_FLAG_GLOBALS_IS_OTHER     2
#define BLITZ_FLAG_ITERATION_IS_OTHER   4

// blitz tags
#define BLITZ_TAG_POS_OPEN                      1
#define BLITZ_TAG_POS_CLOSE                     2

// these are php_templates compability tricks.
// when BLITZ_SUPPORT_PHPT_TAGS_PARTIALLY = 1, you can use <!-- [BEGIN|END] name !--> format.
// set this parameter to 0 if you don't use this format - this will speed up a little
#define BLITZ_SUPPORT_PHPT_TAGS_PARTIALLY       1 
// when BLITZ_NOBRAKET_FUNCTIONS_ARE_VARS = 1, both {{ some }} and <!-- some --> (calls whithout brackets) are variables.
// when BLITZ_NOBRAKET_FUNCTIONS_ARE_VARS = 0, {{ some }} is function, and <!-- some --> is variable.
#define BLITZ_SUPPORT_PHPT_NOBRAKET_FUNCTIONS_ARE_VARS       1

// php-templates tags
// 1) partial php-templates support: 
//    - LEFT & RIGHT CTX tags are EQUAL FOR BOTH open & close
//    - variable LEFT & RIGHT tags are EQUAL TO BLITZ_TAG_POS_OPEN & BLITZ_TAG_POS_CLOSE
// EXAMPLE: <!-- BEGIN php_templates_ctx !--> {{ $php_templates_variable }} <!-- END php_templates_ctx -->
#define BLITZ_TAG_POS_PHPT_CTX_LEFT             4 
#define BLITZ_TAG_POS_PHPT_CTX_RIGHT            8 
// 2) full support: all php-templates tags (not implemented yet)
#define BLITZ_TAG_POS_PHPT_VAR_LEFT             4     
#define BLITZ_TAG_POS_PHPT_VAR_RIGHT            8
#define BLITZ_TAG_POS_PHPT_CTX_OPEN_LEFT        16
#define BLITZ_TAG_POS_PHPT_CTX_OPEN_RIGHT       32
#define BLITZ_TAG_POS_PHPT_CTX_CLOSE_LEFT       64
#define BLITZ_TAG_POS_PHPT_CTX_CLOSE_RIGHT      128

#define BLITZ_LOOP_STACK_MAX    32

// tag position
typedef struct {
    unsigned long pos;
    unsigned char type;
} tag_pos;

// call argument
typedef struct {
    char *name;
    unsigned long len;
    char type;
} call_arg;

// node
typedef struct _tpl_node_struct {
    unsigned long pos_begin;
    unsigned long pos_end;
    unsigned long pos_begin_shift;
    unsigned long pos_end_shift;
    char type;
    unsigned long flags;
    char has_error;
    char *lexem; 
    unsigned long lexem_len;
    call_arg *args;
    unsigned char n_args;
    struct _tpl_node_struct **children; 
    unsigned int n_children;
    unsigned int n_children_alloc;
    unsigned int pos_in_list;
} tpl_node_struct;

// config
typedef struct {
    char *open_node;
    char *close_node;
    unsigned int l_open_node;
    unsigned int l_close_node;
    char var_prefix;
    char *phpt_ctx_left;
    char *phpt_ctx_right;
    unsigned int l_phpt_ctx_left;
    unsigned int l_phpt_ctx_right;
} tpl_config_struct;

// loop stack item
typedef struct _blitz_loop_stack_item {
    unsigned int current;
    unsigned int total;
} blitz_loop_stack_item;

// template: static data
typedef struct _blitz_static_data {
    char *name;
    tpl_config_struct *config;
    tpl_node_struct *nodes;
    unsigned int n_nodes;
    char *body;
    unsigned long body_len;
    HashTable *fetch_index;
} blitz_static_data;

// template
typedef struct _blitz_tpl {
    struct _blitz_static_data static_data;
    char flags;
    HashTable *hash_globals;
    zval *iterations;
    zval **current_iteration;  /* current iteraion values */
    zval **last_iteration;     /* latest iteration - used in combined iterate+set methods  */
    zval **current_iteration_parent; /* list of current context iterations (current_iteration is last element in this list) */
    char *current_path;
    char *tmp_buf;
    HashTable *ht_tpl_name; /* index template_name -> itpl_list number */
    struct _blitz_tpl **itpl_list; /* list of included templates */
    unsigned int itpl_list_alloc;
    unsigned int itpl_list_len;
    unsigned int loop_stack_level;
    blitz_loop_stack_item loop_stack[BLITZ_LOOP_STACK_MAX]; 
} blitz_tpl;

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

#define BLITZ_SCAN_SINGLE_QUOTED(c,p,pos,len,ok)                               \
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

#define BLITZ_SCAN_DOUBLE_QUOTED(c,p,pos,len,ok)                               \
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

#define BLITZ_SCAN_NUMBER(c,p,pos,symb)                                        \
    while(((symb) = *(c)) && BLITZ_IS_NUMBER(symb)) {                          \
        *(p) = symb;                                                           \
        ++(p);                                                                 \
        ++(c);                                                                 \
        ++(pos);                                                               \
    }                                                                          \
    *(p) = '\x0';

#define BLITZ_SCAN_ALNUM(c,p,pos,symb)                                                   \
    while(((symb) = *(c)) && (BLITZ_IS_NUMBER(symb) || BLITZ_IS_ALPHA(symb))) {          \
        *(p) = symb;                                                                     \
        ++(p);                                                                           \
        ++(c);                                                                           \
        ++(pos);                                                                         \
    }                                                                                    \
    *(p) = '\x0';


#define BLITZ_SCAN_VAR(c,p,pos,symb,is_path)                                                 \
    while(((symb) = *(c)) &&                                                                 \
        (BLITZ_IS_NUMBER(symb) || BLITZ_IS_ALPHA(symb) || (symb == '.' && (is_path=1)))) {   \
        *(p) = symb;                                                                         \
        ++(p);                                                                               \
        ++(c);                                                                               \
        ++(pos);                                                                             \
    }                                                                                        \
    *(p) = '\x0';

#define BLITZ_CALL_STATE_NEXT_ARG    1
#define BLITZ_CALL_STATE_FINISHED    2
#define BLITZ_CALL_STATE_HAS_NEXT    3 
#define BLITZ_CALL_STATE_BEGIN       4
#define BLITZ_CALL_STATE_END         5
#define BLITZ_CALL_STATE_ERROR       0

#define BLITZ_CALL_ERROR             1
#define BLITZ_CALL_ERROR_IF          2
#define BLITZ_CALL_ERROR_INCLUDE     3


#define BLITZ_ZVAL_NOT_EMPTY(z, res)                                                              \
    switch (Z_TYPE_PP(z)) {                                                                       \
        case IS_BOOL: res = (0 == Z_LVAL_PP(z)) ? 0 : 1; break;                                   \
        case IS_STRING: res = (0 == Z_STRLEN_PP(z)) ? 0 : 1; break;                               \
        case IS_LONG: res = (0 == Z_LVAL_PP(z)) ? 0 : 1; break;                                   \
        case IS_DOUBLE: res = (.0 == Z_LVAL_PP(z)) ? 0 : 1; break;                                \
        default: res = 0; break;                                                                  \
    } 

#define BLITZ_ARG_NOT_EMPTY(a,ht,res)                                                             \
    if((a).type & BLITZ_ARG_TYPE_STR) {                                                           \
        (res) = ((a).len>0) ? 1 : 0;                                                              \
    } else if ((a).type == BLITZ_ARG_TYPE_NUM) {                                                  \
        (res) = (0 == strncmp((a).name,"0",1)) ? 0 : 1;                                           \
    } else if (((a).type == BLITZ_ARG_TYPE_VAR) && ht) {                                          \
        zval **z;                                                                                 \
        if((a).name && (a).len>0) {                                                               \
            if (SUCCESS == zend_hash_find(ht,(a).name,1+(a).len,(void**)&z))                      \
            {                                                                                     \
                BLITZ_ZVAL_NOT_EMPTY(z, res)                                                      \
            } else {                                                                              \
                res = -1;                                                                         \
            }                                                                                     \
        } else {                                                                                  \
            res = 0;                                                                              \
        }                                                                                         \
    } else if((a).type & BLITZ_ARG_TYPE_BOOL) {                                                   \
        res = ('t' == *((a).name)) ? 1 : 0;                                                       \
    } else {                                                                                      \
        res = 0;                                                                                  \
    }

// switch (Z_TYPE_PP(z)) : see 10 lines upper
// well, we cannot set non-scalar template value, but if ever...
// case IS_ARRAY: res = (0 == zend_hash_num_elements(Z_ARRVAL_PP(z))) ? 0 : 1;

#define BLITZ_REALLOC_RESULT(blen,nlen,rlen,alen,res,pres)                                        \
    (nlen) = (rlen) + (blen);                                                                     \
    if ((rlen) < (nlen)) {                                                                        \
        while ((alen) < (nlen)) {                                                                 \
            (alen) <<= 1;                                                                         \
        }                                                                                         \
        (res) = (unsigned char*)erealloc((res),(alen + 1)*sizeof(unsigned char));                     \
        (pres) = (res) + (rlen);                                                                  \
    }                                                                                             \

#define BLITZ_FETCH_TPL_RESOURCE(id,tpl,desc)                                                     \
    if (((id) = getThis()) == 0) {                                                                \
        WRONG_PARAM_COUNT;                                                                        \
        RETURN_FALSE;                                                                             \
    }                                                                                             \
                                                                                                  \
    if (zend_hash_find(Z_OBJPROP_P((id)), "tpl", sizeof("tpl"), (void **)&(desc)) == FAILURE) {   \
        php_error_docref(NULL TSRMLS_CC, E_WARNING,                                               \
            "INTERNAL:cannot find template descriptor (object->tpl resource)"                     \
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

#define BLITZ_GET_PREDEFINED_VAR(tpl, n, len, value)                                                                   \
    if (n[0] != '_') {                                                                                                 \
        value = -1;                                                                                                    \
    } else {                                                                                                           \
        unsigned int current = tpl->loop_stack[tpl->loop_stack_level].current;                                         \
        if (len == 5 && n[1] == 'e' && n[2] == 'v' && n[3] == 'e' && n[4] == 'n') {                                    \
            value = !(current%2);                                                                                      \
        } else if (len == 4 && n[1] == 'o' && n[2] == 'd' && n[3] == 'd') {                                            \
            value = current%2;                                                                                         \
        } else if (len == 6 && n[1] == 'f' && n[2] == 'i' && n[3] == 'r' && n[4] == 's' && n[5] == 't') {              \
            value = (0 == current);                                                                                    \
        } else if (len == 5 && n[1] == 'l' && n[2] == 'a' && n[3] == 's' && n[4] == 't') {                             \
            value = (current+1 == tpl->loop_stack[tpl->loop_stack_level].total);                                       \
        } else if (len == 4 && n[1] == 'n' && n[2] == 'u' && n[3] == 'm') {                                            \
            value = current + 1;                                                                                       \
        } else if (len == 6 && n[1] == 't' && n[2] == 'o' && n[3] == 't' && n[4] == 'a' && n[5] == 'l') {              \
            value = tpl->loop_stack[tpl->loop_stack_level].total;                                                      \
        }                                                                                                              \
    }

#define TIMER(ts,tz,tdata,tnum)                         \
    gettimeofday(&(ts),&(tz));                          \
    tdata[tnum] = 1000000*ts.tv_sec + ts.tv_usec;       \
    tnum++;

#define TIMER_ADD(ts,tz,tdata,tnum)                     \
    gettimeofday(&(ts),&(tz));                          \
    tdata[tnum] += 1000000*ts.tv_sec + ts.tv_usec;      \

#endif	/* PHP_BLITZ_H */
