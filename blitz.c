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

/* $Id$ */

#ifndef PHP_WIN32
#include <sys/mman.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "safe_mode.h"
#include "php_globals.h"
#include "php_ini.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/info.h"
#include "ext/standard/html.h"
#include <fcntl.h>

#ifdef PHP_WIN32
#include "win32/time.h"
#else
#include <sys/time.h>
#endif

#if PHP_API_VERSION >= 20041225 
#include "ext/date/php_date.h"
#endif

#if PHP_API_VERSION >= 20030307
#define OnUpdateLongLegacy OnUpdateLong
#else
#define OnUpdateLongLegacy OnUpdateInt
#endif

#include "php_blitz.h"

#define BLITZ_DEBUG 0 
#define BLITZ_VERSION_STRING "0.7.1.10-dev"

ZEND_DECLARE_MODULE_GLOBALS(blitz)

/* some declarations  */
static int blitz_exec_template(blitz_tpl *tpl, zval *id, char **result, unsigned long *result_len TSRMLS_DC);
static int blitz_exec_nodes(blitz_tpl *tpl, blitz_node *first, zval *id,
    char **result, unsigned long *result_len, unsigned long *result_alloc_len,
    unsigned long parent_begin, unsigned long parent_end, zval *parent_ctx_data TSRMLS_DC);
static inline int blitz_analize (blitz_tpl *tpl TSRMLS_DC);
static inline void blitz_remove_spaces_around_context_tags(blitz_tpl *tpl TSRMLS_DC);

static int le_blitz;

/* internal classes: Blitz */
static zend_class_entry blitz_class_entry;

#ifdef COMPILE_DL_BLITZ
ZEND_GET_MODULE(blitz)
#endif

/* {{{ macros */

#define REALLOC_POS_IF_EXCEEDS                                                  \
    if (p >= alloc_size) {                                                      \
        alloc_size = alloc_size << 1;                                           \
        *pos = erealloc(*pos, alloc_size * sizeof(tag_pos));                    \
    }                                                                           

#define INIT_CALL_ARGS                                                          \
    node->args = ecalloc(BLITZ_CALL_ALLOC_ARG_INIT, sizeof(call_arg));          \
    node->n_args = 0;                                                           \
    n_arg_alloc = BLITZ_CALL_ALLOC_ARG_INIT;

#define REALLOC_ARG_IF_EXCEEDS                                                  \
    if (arg_id >= n_arg_alloc) {                                                \
        n_arg_alloc = n_arg_alloc << 1;                                         \
        node->args = erealloc(node->args,n_arg_alloc*sizeof(call_arg));         \
    }  

#define ADD_CALL_ARGS(buf, i_len, i_type)                                       \
    REALLOC_ARG_IF_EXCEEDS;                                                     \
    i_arg = node->args + arg_id;                                                \
    if (buf && i_len) {                                                         \
        i_arg->name = estrndup((char *)(buf),(i_len));                          \
        i_arg->len = (i_len);                                                   \
    } else {                                                                    \
        i_arg->name = NULL;                                                     \
        i_arg->len = 0;                                                         \
    }                                                                           \
    i_arg->type = (i_type);                                                     \
    ++arg_id;                                                                   \
    node->n_args = arg_id;                                                     

/* }}} */

static ZEND_INI_MH(OnUpdateVarPrefixHandler) /* {{{ */
{
    char *p;
#ifndef ZTS
    char *base = (char *) mh_arg2;
#else
    char *base;
    base = (char *) ts_resource(*((int *) mh_arg2));
#endif

    p = (char *) (base+(size_t) mh_arg1);

    if (new_value && new_value_length == 0) {
        *p = '\x0';
    } else {
        if (!new_value || new_value_length != 1) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "failed to set blitz.var_prefix (only one character is allowed, like $ or %%)");
            return FAILURE;
        }
        *p = new_value[0];
    }

    return SUCCESS;
}
/* }}} */

/* {{{ ini options */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("blitz.var_prefix", BLITZ_TAG_VAR_PREFIX_S, PHP_INI_ALL,
        OnUpdateVarPrefixHandler, var_prefix, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.tag_open", BLITZ_TAG_OPEN, PHP_INI_ALL, 
        OnUpdateString, tag_open, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.tag_close", BLITZ_TAG_CLOSE, PHP_INI_ALL, 
        OnUpdateString, tag_close, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.tag_open_alt", BLITZ_TAG_OPEN_ALT, PHP_INI_ALL,
        OnUpdateString, tag_open_alt, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.tag_close_alt", BLITZ_TAG_CLOSE_ALT, PHP_INI_ALL,
        OnUpdateString, tag_close_alt, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.comment_open", BLITZ_TAG_COMMENT_OPEN, PHP_INI_ALL,
        OnUpdateString, tag_comment_open, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.comment_close", BLITZ_TAG_COMMENT_CLOSE, PHP_INI_ALL,
        OnUpdateString, tag_comment_close, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.enable_alternative_tags", "1", PHP_INI_ALL,
        OnUpdateBool, enable_alternative_tags, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.enable_comments", "0", PHP_INI_ALL,
        OnUpdateBool, enable_comments, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.path", "", PHP_INI_ALL,
        OnUpdateString, path, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.disable_include", "0", PHP_INI_ALL,
        OnUpdateBool, disable_include, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.remove_spaces_around_context_tags", "1", PHP_INI_ALL,
        OnUpdateBool, remove_spaces_around_context_tags, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.warn_context_duplicates", "0", PHP_INI_ALL,
        OnUpdateBool, warn_context_duplicates, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.check_recursion", "1", PHP_INI_ALL,
        OnUpdateBool, check_recursion, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.charset", "", PHP_INI_ALL,
        OnUpdateString, charset, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.scope_lookup_limit", "0", PHP_INI_ALL,
        OnUpdateLongLegacy, scope_lookup_limit, zend_blitz_globals, blitz_globals)

PHP_INI_END()
/* }}} */

static inline unsigned int blitz_realloc_list(blitz_list *list, unsigned long new_size) { /* {{{ */
    void *p = NULL;
    unsigned long allocated = 0;
    if (new_size < list->allocated)
        return 1;

    allocated = list->allocated << 1;

    p = (void*)erealloc(list->first, list->item_size * allocated);

    if (!p)
        return 0;

    list->first = p;
    list->allocated = allocated;
    list->end = list->first + list->item_size * list->size;

    return 1;
}
/* }}} */

static inline unsigned int blitz_realloc_list_to_add (blitz_list *list) { /* {{{ */
    if (list->size >= BLITZ_MAX_LIST_SIZE)
        return 0;

    return blitz_realloc_list(list, list->size + 1);
}
/* }}} */

static inline int blitz_read_with_stream(blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{
    char *filename = tpl->static_data.name;
    php_stream *stream;

    if (php_check_open_basedir(filename TSRMLS_CC)) {
        return 0;
    }

    stream = php_stream_open_wrapper(filename, "rb", IGNORE_PATH|IGNORE_URL|IGNORE_URL_WIN|ENFORCE_SAFE_MODE|REPORT_ERRORS, NULL);
    if (!stream) {
        return 0;
    }

    tpl->static_data.body_len = php_stream_copy_to_mem(stream, &tpl->static_data.body, PHP_STREAM_COPY_ALL, 0);
    php_stream_close(stream);

    return 1;
}
/* }}} */

static inline int blitz_read_with_fread(blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{
    FILE *stream;
    unsigned int get_len;
    char *filename = tpl->static_data.name;

    if (php_check_open_basedir(filename TSRMLS_CC)) {
        return 0;
    }

    /* VCWD_FOPEN() fixes problems with relative paths in multithreaded environments */
    if (!(stream = VCWD_FOPEN(filename, "rb"))) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to open file \"%s\"", filename);
        return 0;
    }

    tpl->static_data.body = (char*)emalloc(BLITZ_INPUT_BUF_SIZE);
    tpl->static_data.body_len = 0;
    while ((get_len = fread(tpl->static_data.body+tpl->static_data.body_len, 1, BLITZ_INPUT_BUF_SIZE, stream)) > 0) {
        tpl->static_data.body_len += get_len;
        tpl->static_data.body = (char*)erealloc(tpl->static_data.body, tpl->static_data.body_len + BLITZ_INPUT_BUF_SIZE);
    }
    fclose(stream);

    return 1;
}
/* }}} */

static blitz_tpl *blitz_init_tpl_base(HashTable *globals, zval *iterations, blitz_tpl *tpl_parent TSRMLS_DC) /* {{{ */
{
    blitz_static_data *static_data = NULL;
    blitz_tpl *tpl = ecalloc(1, sizeof(blitz_tpl));
    

    static_data = & tpl->static_data;
    static_data->body = NULL;

    tpl->flags = 0;
    static_data->n_nodes = 0;
    static_data->nodes = NULL;
    static_data->tag_open_len = strlen(BLITZ_G(tag_open));
    static_data->tag_close_len = strlen(BLITZ_G(tag_close));
    static_data->tag_open_alt_len = strlen(BLITZ_G(tag_open_alt));
    static_data->tag_close_alt_len = strlen(BLITZ_G(tag_close_alt));

    tpl->loop_stack_level = 0;

    if (iterations) {
        /* 
           "Inherit" iterations from opener, used for incudes. Just make a copy mark template 
           with special flag BLITZ_FLAG_ITERATION_IS_OTHER not to free this object.
        */
        tpl->iterations = iterations;
        tpl->flags |= BLITZ_FLAG_ITERATION_IS_OTHER;
    } else {
        MAKE_STD_ZVAL(tpl->iterations);
        array_init(tpl->iterations);
    }

    if (tpl_parent) {
        tpl->tpl_parent = tpl_parent;
    } else {
        tpl->tpl_parent = NULL;
    }

    tpl->current_iteration = NULL;
    tpl->caller_iteration = NULL;
    tpl->last_iteration = NULL;
    tpl->current_iteration_parent = & tpl->iterations;
    tpl->current_path = estrndup("/", sizeof("/") - 1);

    tpl->tmp_buf = emalloc(BLITZ_TMP_BUF_MAX_LEN);
    static_data->fetch_index = NULL;

    /* allocate "personal" hash-table */
    if (globals == NULL) { 
        ALLOC_HASHTABLE(tpl->hash_globals);
        zend_hash_init(tpl->hash_globals, 8, NULL, ZVAL_PTR_DTOR, 0);
    } else {
        /* 
           "Inherit" globals from opener, used for includes. Just make a copy mark template
           with special flag BLITZ_FLAG_GLOBALS_IS_OTHER not to free this object.
        */
        tpl->hash_globals = globals;
        tpl->flags |= BLITZ_FLAG_GLOBALS_IS_OTHER;
    }

    ALLOC_HASHTABLE(tpl->ht_tpl_name);
    zend_hash_init(tpl->ht_tpl_name, 8, NULL, ZVAL_PTR_DTOR, 0);

    tpl->itpl_list = ecalloc(BLITZ_ITPL_ALLOC_INIT, sizeof(blitz_tpl*));
    tpl->itpl_list_len = 0;
    tpl->itpl_list_alloc = BLITZ_ITPL_ALLOC_INIT;

    tpl->scope_stack_pos = 0;

    return tpl;
}
/* }}} */

static inline void blitz_free_node(blitz_node *node) /* {{{ */
{
    int i = 0;

    for (i = 0; i < node->n_args; i++) {
        if (node->args[i].name) {
            efree(node->args[i].name);
        }
    }

    node->n_args = 0;
    if (node->args) {
        efree(node->args);
        node->args = NULL;
    }

    node->namespace_code = 0;
}
/* }}} */

static void blitz_free_tpl(blitz_tpl *tpl) /* {{{ */
{
    int n_nodes = 0, i = 0;

    if (!tpl) {
        return;
    }

    n_nodes = tpl->static_data.n_nodes;
    for(i = 0 ; i < n_nodes; i++) {
        blitz_free_node(&tpl->static_data.nodes[i]);
	}

    if (tpl->static_data.nodes) {
        efree(tpl->static_data.nodes);
    }

    if (tpl->static_data.body) {
        efree(tpl->static_data.body);
    }

    if (tpl->hash_globals && !(tpl->flags & BLITZ_FLAG_GLOBALS_IS_OTHER)) {
        zend_hash_destroy(tpl->hash_globals);
        FREE_HASHTABLE(tpl->hash_globals);
    }

    if (tpl->ht_tpl_name) {
        zend_hash_destroy(tpl->ht_tpl_name);
        FREE_HASHTABLE(tpl->ht_tpl_name);
    }

    if (tpl->static_data.fetch_index) {
        zend_hash_destroy(tpl->static_data.fetch_index);
        FREE_HASHTABLE(tpl->static_data.fetch_index);
    }

    if (tpl->tmp_buf) {
        efree(tpl->tmp_buf);
    }

    if (tpl->iterations && !(tpl->flags & BLITZ_FLAG_ITERATION_IS_OTHER)) {
        zval_dtor(tpl->iterations);
        efree(tpl->iterations);
        tpl->iterations = NULL;
    }

    if (tpl->itpl_list) {
        for(i=0; i<tpl->itpl_list_len; i++) {
            if (tpl->itpl_list[i]) {
                blitz_free_tpl(tpl->itpl_list[i]);
            }
        }
        efree(tpl->itpl_list);
    }

    if (tpl->current_path) {
        efree(tpl->current_path);
        tpl->current_path = NULL;
    }

    efree(tpl);
}
/* }}} */

static blitz_tpl *blitz_init_tpl(const char *filename, int filename_len,
    HashTable *globals, zval *iterations, blitz_tpl *tpl_parent TSRMLS_DC) /* {{{ */
{
    int global_path_len = 0;
    php_stream *stream = NULL;
    unsigned int filename_normalized_len = 0;
    unsigned int add_buffer_len = 0;
    int result = 0;
    blitz_tpl *tpl = NULL;
    blitz_tpl *i_tpl = NULL;
    char *s = NULL;
    int i = 0, check_loop_max = BLITZ_LOOP_STACK_MAX;
    blitz_static_data *static_data;

    tpl = blitz_init_tpl_base(globals, iterations, tpl_parent TSRMLS_CC);

    if (!tpl) { 
        return NULL;
    }

    if (!filename || !filename_len) {
        return tpl; /* OK, will be loaded after */
    }

    filename_normalized_len = filename_len;

#ifndef PHP_WIN32
    /* "/dir" starts from root  - don't add global path */
    if ('/' != filename[0] && BLITZ_G(path)[0] != 0) { 
#else
    /* "C:\dir" starts from "root" - don't add global path */
    if (!(filename_len>2 && filename[1] == ':' && BLITZ_IS_ALPHA_STRICT(filename[0]) && BLITZ_G(path)[0] != 0)) {
#endif
        global_path_len = strlen(BLITZ_G(path));

        if ((global_path_len + filename_len) > MAXPATHLEN) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "INTERNAL ERROR: file path is too long (limited by MAXPATHLEN)");
            blitz_free_tpl(tpl);
            return NULL;
        }

        memcpy(tpl->static_data.name, BLITZ_G(path), global_path_len);
        memcpy(tpl->static_data.name + global_path_len, filename, filename_len);
        filename_normalized_len = filename_len + global_path_len;
        tpl->static_data.name[filename_normalized_len] = '\0';
    } else {
        // we need realpath to check looped includes
        VCWD_REALPATH(filename, tpl->static_data.name);
        filename_normalized_len = strlen(tpl->static_data.name);
        if (filename_normalized_len == 0) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to open file \"%s\" (realpath failed)", filename);
            return NULL;
        }

        filename_normalized_len = strlen(tpl->static_data.name);
    }

    /* check loops */
    if (BLITZ_G(check_recursion)) {
        i_tpl = tpl;
        while (i++ < check_loop_max) {
            i_tpl = i_tpl->tpl_parent;
            if (!i_tpl) break; 
            s = i_tpl->static_data.name;
            if (0 == strncmp(s, tpl->static_data.name, filename_normalized_len)) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, 
                    "ERROR: include recursion detected for \"%s\". You can disable this check setting blitz.check_recursion to 0 (please, don't do this if you don't know what you are doing)",
                    tpl->static_data.name
                );
                blitz_free_tpl(tpl);
                return NULL;
            }
        }
    }

#ifdef BLITZ_USE_STREAMS
    result = blitz_read_with_stream(tpl TSRMLS_CC);
#else
    result = blitz_read_with_fread(tpl TSRMLS_CC);
#endif

    if (!result) { 
        return tpl;
    }

    /* search algorithm requires lager buffer: body_len + add_buffer */
    static_data = & tpl->static_data;
    add_buffer_len = MAX(
        MAX(static_data->tag_open_len, static_data->tag_close_len), 
        MAX(static_data->tag_open_alt_len, static_data->tag_close_alt_len) 
    );

    tpl->static_data.body = erealloc(tpl->static_data.body, tpl->static_data.body_len + add_buffer_len);
    memset(tpl->static_data.body + tpl->static_data.body_len, '\0', add_buffer_len);

    return tpl;
}
/* }}} */

static int blitz_load_body(blitz_tpl *tpl, const char *body, int body_len TSRMLS_DC) /* {{{ */
{
    unsigned int add_buffer_len = 0;
    char *name = "noname_loaded_from_zval";
    int name_len = sizeof("noname_loaded_from_zval") - 1;

    if (!tpl || !body || !body_len) {
        return 0;
    }

    tpl->static_data.body_len = body_len;

    if (tpl->static_data.body_len) {
        add_buffer_len = MAX(
            MAX(tpl->static_data.tag_open_len, tpl->static_data.tag_close_len),
            MAX(tpl->static_data.tag_open_alt_len, tpl->static_data.tag_close_alt_len)
        );

        tpl->static_data.body = emalloc(tpl->static_data.body_len + add_buffer_len);
        memcpy(tpl->static_data.body, body, body_len);
        memset(tpl->static_data.body + tpl->static_data.body_len, '\0', add_buffer_len);
    }

    memcpy(tpl->static_data.name, name, name_len);

    return 1;
}
/* }}} */

static int blitz_include_tpl_cached(blitz_tpl *tpl, const char *filename, unsigned int filename_len,
    zval *iteration_params, blitz_tpl **itpl TSRMLS_DC) /* {{{ */
{
    zval **desc = NULL;
    unsigned long itpl_idx = 0;
    zval *temp = NULL;
    unsigned char has_protected_iteration = 0;

    /* try to find already parsed tpl index */
    if (SUCCESS == zend_hash_find(tpl->ht_tpl_name, (char *)filename, filename_len, (void **)&desc)) {
        *itpl = tpl->itpl_list[Z_LVAL_PP(desc)];
        if (iteration_params) {
            (*itpl)->iterations = iteration_params;
            (*itpl)->flags |= BLITZ_FLAG_ITERATION_IS_OTHER;
        } else {
            has_protected_iteration = (*itpl)->flags & BLITZ_FLAG_ITERATION_IS_OTHER;

            if ((*itpl)->iterations && !has_protected_iteration) {
                zend_hash_clean(Z_ARRVAL_P((*itpl)->iterations));
            } else if (!(*itpl)->iterations) {
                MAKE_STD_ZVAL((*itpl)->iterations);
                array_init((*itpl)->iterations);
                (*itpl)->flags ^= BLITZ_FLAG_ITERATION_IS_OTHER;
            }
        }

        return 1;
    }

    if (filename_len >= MAXPATHLEN) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Filename exceeds the maximum allowed length of %d characters", MAXPATHLEN);
        return 0;
    }

    /* initialize template */

    if (!(*itpl = blitz_init_tpl(filename, filename_len, tpl->hash_globals, iteration_params, tpl TSRMLS_CC))) {
        return 0;
    }

    /* analize template */
    if (!blitz_analize(*itpl TSRMLS_CC)) {
        blitz_free_tpl(*itpl);
        return 0;
    }

    /* realloc list if needed */
    if (tpl->itpl_list_len >= tpl->itpl_list_alloc-1) {
        tpl->itpl_list = erealloc(tpl->itpl_list,(tpl->itpl_list_alloc<<1) * sizeof(blitz_tpl*));
        tpl->itpl_list_alloc <<= 1;
    }

    /* save template index values */
    itpl_idx = tpl->itpl_list_len;
    tpl->itpl_list[itpl_idx] = *itpl;
    MAKE_STD_ZVAL(temp);
    ZVAL_LONG(temp, itpl_idx);
    zend_hash_update(tpl->ht_tpl_name, (char *)filename, filename_len, &temp, sizeof(zval *), NULL);
    tpl->itpl_list_len++;

    return 1;
}
/* }}} */

static void blitz_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
    blitz_tpl *tpl = NULL;
    tpl = (blitz_tpl*)rsrc->ptr;
    blitz_free_tpl(tpl);
}
/* }}} */

static void php_blitz_init_globals(zend_blitz_globals *blitz_globals) /* {{{ */
{
    memset(blitz_globals, 0, sizeof(blitz_globals));
    blitz_globals->var_prefix = BLITZ_TAG_VAR_PREFIX;
    blitz_globals->tag_open = BLITZ_TAG_OPEN;
    blitz_globals->tag_close = BLITZ_TAG_CLOSE;
    blitz_globals->tag_open_alt = BLITZ_TAG_OPEN_ALT;
    blitz_globals->tag_close_alt = BLITZ_TAG_CLOSE_ALT;
    blitz_globals->enable_alternative_tags = 1;
    blitz_globals->enable_comments = 0;
    blitz_globals->disable_include = 0;
    blitz_globals->path = "";
    blitz_globals->remove_spaces_around_context_tags = 1;
    blitz_globals->warn_context_duplicates = 0;
    blitz_globals->check_recursion = 1;
    blitz_globals->tag_comment_open = BLITZ_TAG_COMMENT_OPEN;
    blitz_globals->tag_comment_close = BLITZ_TAG_COMMENT_CLOSE;
    blitz_globals->scope_lookup_limit = 0;
}
/* }}} */

static void blitz_get_node_paths(zval *list, blitz_node *node, const char *parent_path, unsigned int skip_vars, unsigned int with_type) /* {{{ */
{

    unsigned long j = 0;
    char suffix[2] = "\x0";
    char path[BLITZ_CONTEXT_PATH_MAX_LEN] = "\x0";
    unsigned int may_have_children = 0, do_add = 0;
    blitz_node *i_node = NULL;

    if (!node) 
        return;

    /* hidden, error or non-finalized node (end was not found) */
    if (node->hidden || node->type == BLITZ_NODE_TYPE_BEGIN)
        return;

    if (node->type == BLITZ_NODE_TYPE_CONTEXT || 
        node->type == BLITZ_NODE_TYPE_IF_CONTEXT || 
        node->type == BLITZ_NODE_TYPE_UNLESS_CONTEXT || 
        node->type == BLITZ_NODE_TYPE_ELSEIF_CONTEXT || 
        node->type == BLITZ_NODE_TYPE_ELSE_CONTEXT) 
    {
        may_have_children = 1;
        suffix[0] = '/';
        
        if (node->type == BLITZ_NODE_TYPE_CONTEXT) { /* contexts: use context name from args instead of useless "BEGIN" */
            sprintf(path, "%s%s%s", parent_path, node->args[0].name, suffix); 
        } else {
            sprintf(path, "%s%s%s", parent_path, node->lexem, suffix);
        }
        add_next_index_string(list, path, 1);
    } else if (!skip_vars) {
        sprintf(path, "%s%s%s", parent_path, node->lexem, suffix);
        add_next_index_string(list, path, 1);
    }

    if (*path != '\0' && with_type) {
        if (node->type == BLITZ_NODE_TYPE_CONTEXT) {
            add_next_index_long(list, BLITZ_NODE_PATH_UNIQUE);
        } else {
            add_next_index_long(list, BLITZ_NODE_PATH_NON_UNIQUE);
        }
    }

    if (node->first_child) {
        i_node = node->first_child;
        while (i_node) {
            blitz_get_node_paths(list, i_node, path, skip_vars, with_type);
            i_node = i_node->next;
        }
    }
}
/* }}} */

static void blitz_get_path_list(blitz_tpl *tpl, zval *list, unsigned int skip_vars, unsigned int with_type) /* {{{ */
{
    unsigned long i = 0;
    unsigned int last_close = 0;
    char path[BLITZ_CONTEXT_PATH_MAX_LEN] = "/";

    for (i = 0; i < tpl->static_data.n_nodes; ++i) {
        if (tpl->static_data.nodes[i].pos_begin >= last_close) {
            blitz_get_node_paths(list, tpl->static_data.nodes + i, path, skip_vars, with_type);
            last_close = tpl->static_data.nodes[i].pos_end;
        }
    }

    return;
}
/* }}} */

static void php_blitz_dump_node(blitz_node *node, unsigned int *p_level) /* {{{ */
{
    unsigned long j = 0;
    unsigned int level = 0;
    char shift_str[] = "==========================================================";
    blitz_node *child = NULL;

    if (!node) {
        return;
    }

    level = p_level ? *p_level : 0;
    if (level >= 10) {
        level = 10;
    }

    memset(shift_str,' ',2 * level + 1);
    shift_str[2*level + 1] = '^';
    shift_str[2*level + 3] = '\x0';

    php_printf("\n%s%s[%u] (%lu(%lu), %lu(%lu)); ",
        shift_str,
        node->lexem,
        (unsigned int)node->type,
        node->pos_begin, node->pos_begin_shift,
        node->pos_end, node->pos_end_shift
    );

    if (BLITZ_IS_METHOD(node->type)) {
        php_printf("ARGS(%d): ",node->n_args);
        for (j = 0; j < node->n_args; ++j) {
            if (j) php_printf(",");
            php_printf("%s(%d)",node->args[j].name, node->args[j].type);
        }

        if (node->first_child) {
            php_printf("; CHILDREN:");
            child = node->first_child;
            while (child) {
                (*p_level)++;
                php_blitz_dump_node(child, p_level);
                (*p_level)--;
                child = child->next;
            }
        }
    }
}
/* }}} */

static void php_blitz_dump_struct(blitz_tpl *tpl) /* {{{ */
{
    unsigned long i = 0;
    unsigned int level = 0;
    unsigned int last_close = 0;
    blitz_node *node = NULL;

    php_printf("== TREE STRUCT (%ld nodes):",(unsigned long)tpl->static_data.n_nodes);
    node = tpl->static_data.nodes;

    while (node) {
        php_blitz_dump_node(node, &level);
        node = node->next;
    }

    php_printf("\n");

    return;
}
/* }}} */

void blitz_warn_context_duplicates(blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{
    zval *path_list = NULL;
    HashTable uk_path;
    zval **path = NULL, **path_type = NULL;
    int z = 1;
    char *key;
    unsigned int key_len;
    unsigned long key_index;

    MAKE_STD_ZVAL(path_list);
    array_init(path_list);

    zend_hash_init(&uk_path, 10, NULL, NULL, 0);

    blitz_get_path_list(tpl, path_list, 1, 1);

    zend_hash_internal_pointer_reset(Z_ARRVAL_P(path_list));
    while (zend_hash_get_current_data_ex(Z_ARRVAL_P(path_list), (void**) &path, NULL) == SUCCESS) {
        zend_hash_move_forward(Z_ARRVAL_P(path_list));
        zend_hash_get_current_data_ex(Z_ARRVAL_P(path_list), (void**) &path_type, NULL);

        if (Z_LVAL_PP(path_type) == BLITZ_NODE_PATH_NON_UNIQUE) {
            zend_hash_move_forward(Z_ARRVAL_P(path_list));
            continue;
        }

        if (zend_hash_exists(&uk_path, Z_STRVAL_PP(path), Z_STRLEN_PP(path))) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "WARNING: context name \"%s\" duplicate in %s",
                 Z_STRVAL_PP(path), tpl->static_data.name
            );
        } else {
            zend_hash_add(&uk_path, Z_STRVAL_PP(path), Z_STRLEN_PP(path), &z, sizeof(int), NULL);
        }

        zend_hash_move_forward(Z_ARRVAL_P(path_list));
    }

    zval_ptr_dtor(&path_list);
    zend_hash_destroy(&uk_path);

    return;
}
/* }}} */

static int blitz_find_tag_positions(blitz_string *body, blitz_list *list_pos TSRMLS_DC) { /* {{{ */
    unsigned char current_tag_pos[BLITZ_TAG_LIST_LEN];
    unsigned char idx_tag[BLITZ_TAG_LIST_LEN];
    blitz_string list_tag[BLITZ_TAG_LIST_LEN];
    unsigned char tag_list_len = BLITZ_TAG_LIST_LEN;
    unsigned long pos = 0, pos_check_max = 0;
    unsigned char c = 0, p = 0, tag_id = 0, idx_tag_id = 0, found = 0, skip_steps = 0;
    unsigned char *pc = NULL;
    unsigned long i = 0;
    unsigned int tag_min_len = 0;
    unsigned char check_map[256];
    blitz_string *t = NULL;
    tag_pos *tp = NULL;

    list_tag[BLITZ_TAG_ID_OPEN].s = BLITZ_G(tag_open);
    list_tag[BLITZ_TAG_ID_CLOSE].s = BLITZ_G(tag_close);
    idx_tag[0] = BLITZ_TAG_ID_OPEN;
    idx_tag[1] = BLITZ_TAG_ID_CLOSE;
    tag_id = 1;

#if BLITZ_ENABLE_ALTERNATIVE_TAGS
    if (BLITZ_G(enable_alternative_tags)) {
        list_tag[BLITZ_TAG_ID_OPEN_ALT].s = BLITZ_G(tag_open_alt);
        list_tag[BLITZ_TAG_ID_CLOSE_ALT].s = BLITZ_G(tag_close_alt); 
        idx_tag[2] = BLITZ_TAG_ID_OPEN_ALT;
        idx_tag[3] = BLITZ_TAG_ID_CLOSE_ALT;
        tag_id += 2;
    } else {
        tag_list_len -= 2;
    }
#else
    tag_list_len -= 2;
#endif

#if BLITZ_ENABLE_COMMENTS
    if (BLITZ_G(enable_comments)) {
        list_tag[BLITZ_TAG_ID_COMMENT_OPEN].s = BLITZ_G(tag_comment_open);
        list_tag[BLITZ_TAG_ID_COMMENT_CLOSE].s = BLITZ_G(tag_comment_close);
        idx_tag[tag_id + 1] = BLITZ_TAG_ID_COMMENT_OPEN;
        idx_tag[tag_id + 2] = BLITZ_TAG_ID_COMMENT_CLOSE;
    } else {
        tag_list_len -= 2;
    }
#else
    tag_list_len -= 2;
#endif

    memset(check_map, 0, 256*sizeof(unsigned char));
    tag_min_len = strlen(list_tag[0].s);
    for (idx_tag_id = 0; idx_tag_id < tag_list_len; idx_tag_id++) {
        tag_id = idx_tag[idx_tag_id]; // loop through index, some tags can be disabled
        list_tag[tag_id].len = strlen(list_tag[tag_id].s);
        current_tag_pos[tag_id] = 0;
        if (list_tag[tag_id].len < tag_min_len) {
            tag_min_len = list_tag[tag_id].len;
        }

        pc = list_tag[tag_id].s;
        while (*pc) {
            check_map[*pc] = 1;
            pc++;
        }
    }

    pc = body->s;
    c = *pc;
    pos = 0;
    skip_steps = 0;
    pos_check_max = body->len - tag_min_len;
    while (c) {
        c = *pc;
        found = 0;
        for (idx_tag_id = 0; idx_tag_id < tag_list_len; idx_tag_id++) {
            tag_id = idx_tag[idx_tag_id]; // loop through index, some tags can be disabled
            t = list_tag + tag_id;
            p = current_tag_pos[tag_id];
            if (c == t->s[p]) {
                found = 1;
                p++;
                if (p == t->len) {
                    if (!blitz_realloc_list_to_add(list_pos))
                        return 0;

                    tp = (tag_pos*)list_pos->end;
                    tp->pos = pos - list_tag[tag_id].len + 1;
                    tp->tag_id = tag_id;
                    list_pos->size++;
                    list_pos->end = list_pos->first + list_pos->size * list_pos->item_size;
                    for (tag_id = 0; tag_id < tag_list_len; tag_id++) {
                        current_tag_pos[tag_id] = 0;
                    }
                    break;
                }
                current_tag_pos[tag_id]++;
            } else {
                if (p > 0) {
                    current_tag_pos[tag_id] = 0;
                }
            }
        }

        if (found == 0) {
            while ((pos < pos_check_max) && 0 == check_map[*(pc + tag_min_len)]) {
                pc += tag_min_len;
                pos += tag_min_len;
            }
        }

        pc++;
        pos++;
    }

    return 1;
}

/*

c - current char in the scanned string
pos - current scan position
i_pos - scanned position shuift
buf - where final scanned/transformed values go
i_len - true length for buf size
        doesn't include variable prefix like $ or just 1 for "TRUE"(saved as 't' or "FALSE"(saved as 'f')))

*/

/* {{{ void blitz_parse_arg() */
static inline void blitz_parse_arg (char *text, char var_prefix,
    char *token_out, unsigned char *type, unsigned int *len, unsigned int *pos_out TSRMLS_DC)
{
    char *c = text;
    char *p = NULL;
    char symb = 0, i_symb = 0, is_path = 0;
    char state = BLITZ_CALL_STATE_ERROR;
    char ok = 0;
    unsigned int pos = 0, i_pos = 0, i_len = 0;
    unsigned char i_type;
    char was_escaped;
    
    *type = 0;
    *len = 0;
    *pos_out = 0;

    BLITZ_SKIP_BLANK(c,i_pos,pos);
    symb = *c;
    i_len = i_pos = ok = 0;
    p = token_out;

    if (BLITZ_DEBUG) php_printf("[F] blitz_parse_arg: %u\n", *c);

    if (var_prefix && (symb == var_prefix)) {
        ++c; ++pos;
        is_path = 0;
        BLITZ_SCAN_VAR(c,p,i_pos,i_symb,is_path);
        i_type = is_path ? BLITZ_ARG_TYPE_VAR_PATH : BLITZ_ARG_TYPE_VAR;
        i_len = i_pos;
    } else if (symb == '\'') {
        ++c; ++pos;
        BLITZ_SCAN_SINGLE_QUOTED(c,p,i_pos,i_len,ok);
        i_pos++;
        i_type = BLITZ_ARG_TYPE_STR;
    } else if (symb == '\"') {
        ++c; ++pos;
        BLITZ_SCAN_DOUBLE_QUOTED(c,p,i_pos,i_len,ok);
        i_pos++;
        i_type = BLITZ_ARG_TYPE_STR;
    } else if (BLITZ_IS_NUMBER(symb)) {
        BLITZ_SCAN_NUMBER(c,p,i_pos,i_symb);
        i_type = BLITZ_ARG_TYPE_NUM;
        i_len = i_pos;
    } else if (BLITZ_IS_ALPHA(symb)){
        is_path = 0;
        BLITZ_SCAN_VAR(c,p,i_pos,i_symb,is_path);
        i_len = 1;
        i_type = BLITZ_ARG_TYPE_BOOL;
        if (i_pos!=0) {
            ok = 1;
            if (BLITZ_STRING_IS_TRUE(token_out, i_len)) {
                token_out[0] = 't';
            } else if (BLITZ_STRING_IS_FALSE(token_out, i_len)){
                token_out[0] = 'f';
            } else { /* treat this just as variable used without var prefix */
                i_len = i_pos;
                i_type = is_path ? BLITZ_ARG_TYPE_VAR_PATH : BLITZ_ARG_TYPE_VAR;
            }
        }
    }

    *type = i_type;
    *len = i_len;
    *pos_out = pos + i_pos;
}

/* {{{ void blitz_parse_call() */
static inline void blitz_parse_call (char *text, unsigned int len_text, blitz_node *node, 
    unsigned int *true_lexem_len, char var_prefix, char *error TSRMLS_DC)
{
    char *c = text;
    char *p = NULL;
    char symb = 0, i_symb = 0, is_path = 0;
    char state = BLITZ_CALL_STATE_ERROR;
    char ok = 0;
    unsigned int pos = 0, i_pos = 0, i_len = 0;
    char was_escaped;
    char buf[BLITZ_MAX_LEXEM_LEN];
    char n_arg_alloc = 0;
    unsigned char i_type = 0;
    unsigned char arg_id = 0;
    call_arg *i_arg = NULL;
    unsigned char bool_char = 0;
    char *p_end = NULL;
    char has_namespace = 0;
    register unsigned char shift = 0, i = 0, j = 0;

    BLITZ_SKIP_BLANK(c,i_pos,pos);

    if (BLITZ_DEBUG) {
        char *tmp = estrndup((char *)text, len_text);
        tmp[len_text-1] = '\x0';
        php_printf("*** FUNCTION *** blitz_parse_call, started at pos=%u, c=%c\n", pos, *c);
        php_printf("text: %s\n", tmp);
        efree(tmp);
    }

    /* init node */
    blitz_free_node(node);
    node->type = BLITZ_TYPE_METHOD;

    *true_lexem_len = 0; /* used for parameters only */

    i_pos = 0;
    /* parameter or method? */
    if (var_prefix && *c == var_prefix) { /* scan parameter */
        ++c;
        ++pos;
        BLITZ_SCAN_VAR_NOCOPY(c, i_pos, i_symb, is_path);
        if (i_pos != 0) {
            memcpy(node->lexem, text + pos, i_pos);
            node->lexem_len = i_pos;
            *true_lexem_len = i_pos;
            if (is_path) {
                node->type = BLITZ_NODE_TYPE_VAR_PATH;
            } else {
                node->type = BLITZ_NODE_TYPE_VAR;
            }
            state = BLITZ_CALL_STATE_FINISHED;
            pos += i_pos;
        }
    } else if (BLITZ_IS_ALPHA(*c)) { /* scan function */
        is_path = 0;
        BLITZ_SCAN_VAR_NOCOPY(c, i_pos, i_symb, is_path);
        if (i_pos > 0) {
            p_end = text + pos + i_pos;
            if (*p_end == ':' && *(p_end + 1) == ':') { // static calls for plugins
                has_namespace = 1;
                i_pos += 2;
                c = text + pos + i_pos;
                BLITZ_SCAN_VAR_NOCOPY(c, i_pos, i_symb, is_path);
            }

            memcpy(node->lexem, text + pos, i_pos);
            node->lexem_len = i_pos;
            node->type = BLITZ_TYPE_METHOD;
            *true_lexem_len = i_pos;
            pos += i_pos;
            c = text + pos;

            if (BLITZ_DEBUG) php_printf("METHOD: %s, pos=%u, c=%c, type=%u\n", node->lexem, pos, *c, node->type);

            if (has_namespace) {
                if (BLITZ_STRING_PHP_NAMESPACE(node->lexem, node->lexem_len)) {
                    shift = BLITZ_PHP_NAMESPACE_SHIFT;
                    node->namespace_code = BLITZ_NODE_PHP_NAMESPACE;
                } else if (BLITZ_STRING_THIS_NAMESPACE(node->lexem, node->lexem_len)) {
                    shift = BLITZ_THIS_NAMESPACE_SHIFT;
                    node->namespace_code = BLITZ_NODE_THIS_NAMESPACE;
                } else {
                    node->namespace_code = BLITZ_NODE_UNKNOWN_NAMESPACE;
                }
                if (shift) {
                    i = 0;
                    j = shift;
                    while (i < node->lexem_len) {
                        node->lexem[i++] = node->lexem[j++];
                    }
                    node->lexem_len = node->lexem_len - shift;
                }
                if (BLITZ_DEBUG) php_printf("REDUCED STATIC CALL: %s, len=%u, namespace_code=%u\n", 
                    node->lexem, node->lexem_len, node->namespace_code);
            }

            if (*c == '(') { /* has arguments */
                ok = 1; ++pos; 
                BLITZ_SKIP_BLANK(c,i_pos,pos);
                ++c;
                if (*c == ')') { /* move to finished without any middle-state if no args */
                    ++pos;
                    state = BLITZ_CALL_STATE_FINISHED;
                } else {
                    INIT_CALL_ARGS;
                    state = BLITZ_CALL_STATE_NEXT_ARG;

                    /* predefined method? */
                    if (has_namespace == 0) {
                        if (BLITZ_STRING_IS_IF(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_IF;
                        } else if (BLITZ_STRING_IS_UNLESS(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_UNLESS;
                        } else if (BLITZ_STRING_IS_INCLUDE(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_INCLUDE;
                        } else if (BLITZ_STRING_IS_ESCAPE(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_WRAPPER_ESCAPE;
                        } else if (BLITZ_STRING_IS_DATE(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_WRAPPER_DATE;
                        } 
                    }
                }
                zend_str_tolower(node->lexem, node->lexem_len); /* case insensitivity for methods */
            } else {
                ok = 1;
                if (BLITZ_STRING_IS_BEGIN(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS; 
                    state = BLITZ_CALL_STATE_BEGIN;
                    node->type = BLITZ_NODE_TYPE_BEGIN;
                } else if (BLITZ_STRING_IS_IF(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS;
                    state = BLITZ_CALL_STATE_IF;
                    node->type = BLITZ_NODE_TYPE_IF_NF;
                } else if (BLITZ_STRING_IS_UNLESS(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS;
                    state = BLITZ_CALL_STATE_IF;
                    node->type = BLITZ_NODE_TYPE_UNLESS_NF;
                } else if (BLITZ_STRING_IS_ELSEIF(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS;
                    state = BLITZ_CALL_STATE_IF;
                    node->type = BLITZ_NODE_TYPE_ELSEIF_NF;
                } else if (BLITZ_STRING_IS_ELSE(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS;
                    state = BLITZ_CALL_STATE_ELSE;
                    node->type = BLITZ_NODE_TYPE_ELSE_NF;
                } else if (BLITZ_STRING_IS_END(node->lexem, node->lexem_len)) {
                    state = BLITZ_CALL_STATE_END;
                    node->type = BLITZ_NODE_TYPE_END;
                } else {
                    /* functions without brackets are treated as parameters */
                    if (BLITZ_NOBRAKET_FUNCTIONS_ARE_VARS) {
                        node->type = is_path ? BLITZ_NODE_TYPE_VAR_PATH : BLITZ_NODE_TYPE_VAR;
                    } else { /* case insensitivity for methods */
                        zend_str_tolower(node->lexem, node->lexem_len);
                    }
                    state = BLITZ_CALL_STATE_FINISHED;
                }
            }
        }

        c = text + pos;
        p = buf;
        while ((symb = *c) && ok) {
            switch (state) {
                case BLITZ_CALL_STATE_BEGIN:
                    symb = *c;
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    i_len = i_pos = ok = 0;
                    p = buf;
                    BLITZ_SCAN_ALNUM(c,p,i_len,i_symb);
                    if (i_len!=0) {
                        ok = 1; 
                        pos += i_len;
                        ADD_CALL_ARGS(buf, i_len, i_type);
                        state = BLITZ_CALL_STATE_FINISHED;
                    } else {
                        state = BLITZ_CALL_STATE_ERROR;
                    }
                    break;
                case BLITZ_CALL_STATE_END:
                    i_pos = 0;
                    BLITZ_SKIP_BLANK(c,i_pos,pos); i_pos = 0; symb = *c;
                    if (BLITZ_IS_ALPHA(symb)) {
                        BLITZ_SCAN_ALNUM(c,p,i_pos,i_symb); pos += i_pos; i_pos = 0;
                    }
                    state = BLITZ_CALL_STATE_FINISHED;
                    break;
               case BLITZ_CALL_STATE_IF:
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    is_path = i_len = i_pos = i_type = ok = 0;
                    blitz_parse_arg(c, var_prefix, buf, &i_type, &i_len, &i_pos TSRMLS_CC);

                    if (i_len) {
                        ok = 1;
                        pos += i_pos;
                        c = text + pos;
                        ADD_CALL_ARGS(buf, i_len, i_type);

                        BLITZ_SKIP_BLANK(c,i_pos,pos);
                        i_len = 0;
                        BLITZ_SCAN_EXPR_OPERATOR(c,i_len,i_type);
                        if (i_len) {
                            ok = 0;
                            c += i_len; pos += i_len;
                            ADD_CALL_ARGS(NULL, 0, i_type);

                            BLITZ_SKIP_BLANK(c,i_pos,pos);
                            is_path = i_len = i_pos = i_type = ok = 0;

                            blitz_parse_arg(c, var_prefix, buf, &i_type, &i_len, &i_pos TSRMLS_CC);
                            if (i_len) {
                                pos += i_pos;
                                c = text + pos;
                                ADD_CALL_ARGS(buf, i_len, i_type);
                                state = BLITZ_CALL_STATE_FINISHED;
                            } else {
                                state = BLITZ_CALL_STATE_ERROR;
                            }
                        } else {
                            state = BLITZ_CALL_STATE_FINISHED;
                        }
                    } else {
                        state = BLITZ_CALL_STATE_ERROR;
                    }
                    break;
                case BLITZ_CALL_STATE_ELSE:
                    i_pos = 0;
                    BLITZ_SKIP_BLANK(c,i_pos,pos); i_pos = 0; symb = *c;
                    if (BLITZ_IS_ALPHA(symb)) {
                        BLITZ_SCAN_ALNUM(c,p,i_pos,i_symb); pos += i_pos; i_pos = 0;
                    }
                    state = BLITZ_CALL_STATE_FINISHED;
                    break;
                case BLITZ_CALL_STATE_NEXT_ARG:

                    blitz_parse_arg(c, var_prefix, buf, &i_type, &i_len, &i_pos TSRMLS_CC);
                    if (i_len) {
                        pos += i_pos;
                        c = text + pos;
                        ADD_CALL_ARGS(buf, i_len, i_type);
                        state = BLITZ_CALL_STATE_HAS_NEXT;
                    } else {
                        state = BLITZ_CALL_STATE_ERROR;
                    }
                    break;
                case BLITZ_CALL_STATE_HAS_NEXT:
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    symb = *c;
                    if (symb == ',') {
                        state = BLITZ_CALL_STATE_NEXT_ARG;
                        ++c; ++pos;
                    } else if (symb == ')') {
                        state = BLITZ_CALL_STATE_FINISHED;
                        ++c; ++pos;
                    } else {
                        state = BLITZ_CALL_STATE_ERROR;
                    }
                    break;
                case BLITZ_CALL_STATE_FINISHED:
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    if (*c == ';') {
                        ++c; ++pos;
                    }
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    if (pos < len_text) { /* when close tag contains whitespaces SKIP_BLANK will make pos>=len_text */
                        if (BLITZ_DEBUG) php_printf("%u <> %u => error state\n", pos, len_text);
                        state = BLITZ_CALL_STATE_ERROR;
                    }
                    ok = 0;
                    break;
                default:
                    ok = 0;
                    break;
            }
        }
    }

    if (state != BLITZ_CALL_STATE_FINISHED) {
        *error = BLITZ_CALL_ERROR;
    } else if ((node->type == BLITZ_NODE_TYPE_IF || node->type == BLITZ_NODE_TYPE_UNLESS) && (node->n_args<2 || node->n_args>3)) {
        *error = BLITZ_CALL_ERROR_IF;
    } else if ((node->type == BLITZ_NODE_TYPE_INCLUDE) && (node->n_args!=1)) {
        *error = BLITZ_CALL_ERROR_INCLUDE;
    }

    /* FIXME: check arguments for wrappers (escape, date, trim) */
}
/* }}} */

static unsigned long get_line_number(const char *str, unsigned long pos) { /* {{{ */
    register const char *p = str;
    register unsigned long i = pos;
    register unsigned int n = 0;

    p += i;
    if (i > 0) {
        i++;
        while (i--) {
            if (*p == '\n') {
                n++;
            }
            p--;
        }
    }

    /* humans like to start counting with 1, not 0 */
    n += 1; 

    return n;
}
/* }}} */

static unsigned long get_line_pos(const char *str, unsigned long pos) { /* {{{ */
    register const char *p = str;
    register unsigned long i = pos;

    p += i;
    while (i) {
        if (*p == '\n') {
            return pos - i;
        }
        i--;
        p--;
    }

    /* humans like to start counting with 1, not 0 */
    return pos + 1; 
}
/* }}} */

/* {{{ void blitz_analizer_warn_unexpected_tag (blitz_tpl *tpl, unsigned int pos) */
static inline void blitz_analizer_warn_unexpected_tag (blitz_tpl *tpl, unsigned char tag_id, unsigned int pos TSRMLS_DC) {
    char *human_tag_name[BLITZ_TAG_LIST_LEN] = {
        "TAG_OPEN", "TAG_OPEN_ALT", "TAG_CLOSE", "TAG_CLOSE_ALT",
        "TAG_OPEN_COMMENT", "TAG_CLOSE_COMMENT"
    };
    char *template_name = tpl->static_data.name;
    char *body = tpl->static_data.body;

    php_error_docref(NULL TSRMLS_CC, E_WARNING,
        "SYNTAX ERROR: unexpected %s (%s: line %lu, pos %lu)",
        human_tag_name[tag_id], template_name,
        get_line_number(body,pos), get_line_pos(body,pos)
    );
}
/* }}} */

/* {{{  void blitz_dump_node_stack(analizer_ctx *ctx) */
static inline void blitz_dump_node_stack(analizer_ctx *ctx) {
    unsigned int i = 0;
    analizer_stack_elem *stack_head = ctx->node_stack;

    php_printf("NODE_STACK_DUMP:\n");
    for (i = 0; i <= ctx->node_stack_len; i++, stack_head++) {
        php_printf(" %u: parent = ",i);
        if (stack_head->parent) {
            php_printf("%s ", stack_head->parent->lexem);
        } else {
            php_printf("[empty] ");
        }

        php_printf(", last = ");
        if (stack_head->last) {
            php_printf("%s ", stack_head->last->lexem);
        } else {
            php_printf("[empty]");
        }

        php_printf(", parent->next = ");
        if (stack_head->parent && stack_head->parent->next) {
            php_printf("%s ", stack_head->parent->next->lexem);
        } else {
            php_printf("[empty]");
        }

        php_printf(", last->next = ");
        if (stack_head->last && stack_head->last->next) {
            php_printf("%s ", stack_head->last->next->lexem);
        } else {
            php_printf("[empty]");
        }

        php_printf("\n");
    }
}
/* }}} */

/* {{{ int blitz_analizer_create_parent */
static inline int blitz_analizer_create_parent(analizer_ctx *ctx, unsigned int get_next_node TSRMLS_DC)
{
    analizer_stack_elem *stack_head = NULL;
    blitz_node *i_node = ctx->node;
    unsigned int current_open = ctx->current_open;
    unsigned int current_close = ctx->current_close;
    unsigned int close_len = ctx->close_len;
 
    if (BLITZ_DEBUG)
        php_printf("*** FUNCTION *** blitz_analizer_create_parent: node = %s\n", i_node->lexem);

    i_node->pos_begin = current_open;
    i_node->pos_begin_shift = current_close + close_len;

    /* next 3 parameters need finalization when END is detected */
    i_node->pos_end = current_close + close_len;
    i_node->pos_end_shift = 0;
    i_node->lexem_len = ctx->true_lexem_len;

    stack_head = ctx->node_stack + ctx->node_stack_len;

    if (stack_head->parent && !stack_head->parent->first_child) {
        if (BLITZ_DEBUG) php_printf("STACK: adding child (%s) to %s\n", i_node->lexem, stack_head->parent->lexem);
        stack_head->parent->first_child = i_node;
    } else if (stack_head->last) {
        if (BLITZ_DEBUG) php_printf("STACK: adding next (%s) to %s\n", i_node->lexem, stack_head->last->lexem);
        stack_head->last->next = i_node;
    }

    stack_head->last = i_node;

    if (ctx->node_stack_len >= BLITZ_ANALIZER_NODE_STACK_LEN) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR,
            "INTERNAL ERROR: analizer stack length (%u) was exceeded when parsing template (%s: line %lu, pos %lu), recompile blitz with different BLITZ_ANALIZER_NODE_STACK_LEN or just don't use so complex templates", BLITZ_ANALIZER_NODE_STACK_LEN, ctx->tpl->static_data.name, 
            get_line_number(ctx->tpl->static_data.body, current_open), get_line_pos(ctx->tpl->static_data.body, current_open)
        );
    }

    ++(ctx->node_stack_len);
    stack_head = ctx->node_stack + ctx->node_stack_len;
    stack_head->parent = i_node;
    stack_head->last = i_node;

    if (get_next_node)
        ++(ctx->n_nodes);

    if (BLITZ_DEBUG) blitz_dump_node_stack(ctx);

    return 1;
}
/* }}} */

/* {{{ void blitz_analizer_finalize_parent */
static inline void blitz_analizer_finalize_parent(analizer_ctx *ctx, unsigned int attach TSRMLS_DC)
{
    analizer_stack_elem *stack_head = NULL;
    blitz_node *i_node = ctx->node, *parent = NULL, *last = NULL;
    blitz_tpl *tpl = NULL;
    char *body = NULL;
    unsigned long current_open = ctx->current_open;
    unsigned long current_close = ctx->current_close;
    unsigned long close_len = ctx->close_len;

    if (BLITZ_DEBUG)
        php_printf("*** FUNCTION *** blitz_analizer_finalize_parent: node = %s\n", i_node->lexem);

    /* end: remove node from stack, finalize node: set true close positions and new type */
    if (ctx->node_stack_len) {
        stack_head = ctx->node_stack + ctx->node_stack_len;

        /**************************************************************************/
        /* parent BEGIN/IF/ELSEIF/ELSE becomes:                                   */
        /**************************************************************************/
        /*    {{ begin   a }}         bla       {{             end a }}           */
        /*    ^--pos_begin   ^--pos_begin_shift ^--pos_end_shift       ^--pos_end */
        /**************************************************************************/
        /* child END becomes                                                      */
        /**************************************************************************/
        /*    {{ begin   a }}         bla       {{             end a }}           */
        /*                                      ^--pos_begin           ^--pos_end */
        /**************************************************************************/

        parent = stack_head->parent;
        parent->pos_end_shift = current_open;
        parent->pos_end = current_close + close_len;

        i_node->pos_begin = current_open;
        i_node->pos_end = parent->pos_end;
        i_node->pos_begin_shift = parent->pos_end;
        i_node->pos_end_shift = parent->pos_end;

        if (BLITZ_NODE_TYPE_BEGIN == parent->type) {
            parent->type = BLITZ_NODE_TYPE_CONTEXT;
        } else if (BLITZ_NODE_TYPE_IF_NF == parent->type) {
            parent->type = BLITZ_NODE_TYPE_IF_CONTEXT;
        } else if (BLITZ_NODE_TYPE_UNLESS_NF == parent->type) {
            parent->type = BLITZ_NODE_TYPE_UNLESS_CONTEXT;
        } else if (BLITZ_NODE_TYPE_ELSEIF_NF == parent->type) {
            parent->type = BLITZ_NODE_TYPE_ELSEIF_CONTEXT;
        } else if (BLITZ_NODE_TYPE_ELSE_NF == parent->type) {
            parent->type = BLITZ_NODE_TYPE_ELSE_CONTEXT;
        }

        if (attach) {
            if (!parent->first_child) {
                if (BLITZ_DEBUG)
                    php_printf("STACK: adding child (%s) to %s\n", i_node->lexem, parent->lexem);
                parent->first_child = i_node;
            } else if (stack_head->last) {
                if (BLITZ_DEBUG)
                    php_printf("STACK: adding next (%s) to %s\n", i_node->lexem, stack_head->last->lexem);
                stack_head->last->next = i_node;
            }
            stack_head->last = i_node;
        }

        --(ctx->node_stack_len);
        ++(ctx->n_nodes);

        if (BLITZ_DEBUG) blitz_dump_node_stack(ctx);

    } else {
        /* error: end with no begin */
        i_node->hidden = 1;
        tpl = ctx->tpl;
        body = tpl->static_data.body;
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "SYNTAX ERROR: end with no begin (%s: line %lu, pos %lu)",
            tpl->static_data.name, get_line_number(body,current_open),get_line_pos(body, current_open)
        );
    }

    return;
}
/* }}} */

/* {{{ int blitz_analizer_add */
static inline int blitz_analizer_add(analizer_ctx *ctx TSRMLS_DC) {
    unsigned int open_len = 0, close_len = 0;
    unsigned int lexem_len = 0;
    unsigned int true_lexem_len = 0;
    unsigned int shift = 0;
    char i_error = 0;
    unsigned long current_open = 0, current_close;
    tag_pos *tag = NULL;
    blitz_tpl *tpl = NULL;
    char *plex = NULL;
    blitz_node *i_node = NULL;
    unsigned int n_nodes = 0;
    unsigned char is_alt_tag = 0;
    char *body = NULL;
    analizer_stack_elem *stack_head = NULL;

    tag = ctx->tag;
    tpl = ctx->tpl;
    body = tpl->static_data.body;
    current_close = tag->pos;
    current_open = ctx->pos_open;

    is_alt_tag = (tag->tag_id == BLITZ_TAG_ID_CLOSE_ALT) ? 1 : 0;
    if (is_alt_tag) { 
        open_len = tpl->static_data.tag_open_alt_len;
        close_len = tpl->static_data.tag_close_alt_len;
    } else {
        open_len = tpl->static_data.tag_open_len;
        close_len = tpl->static_data.tag_close_len;
    }

    shift = current_open + open_len;
    lexem_len = current_close - shift;

    i_node = tpl->static_data.nodes + ctx->n_nodes;

    if (BLITZ_DEBUG) {
        php_printf("[F] blitz_analizer_add: open_pos = %ld, close_pos = %ld, shift = %u, lexem_len = %u, tag_id = %u\n",
            current_open, current_close, shift, lexem_len, tag->tag_id);
    }

    if (lexem_len > BLITZ_MAX_LEXEM_LEN) {
        if (tag->tag_id != BLITZ_TAG_ID_CLOSE_ALT) { /* HTML-comments fix */
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "SYNTAX ERROR: lexem is too long (%s: line %lu, pos %lu)",
                tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
            );
        }
        return 1;
    } 

    if (lexem_len <= 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "SYNTAX ERROR: zero length lexem (%s: line %lu, pos %lu)",
            tpl->static_data.name,
            get_line_number(body, current_open), get_line_pos(body, current_open)
        );
        return 1;
    }

    i_node->args = NULL;
    i_node->n_args = 0;
    i_node->pos_in_list = ctx->n_nodes;
    i_node->hidden = 0;
    i_node->pos_begin_shift = 0;
    i_node->pos_end_shift = 0;
    i_node->lexem[0] = '\x0';
    i_node->lexem_len = 0;
    i_node->first_child = NULL;
    i_node->next = NULL;

    if (tag->tag_id == BLITZ_TAG_ID_COMMENT_CLOSE) {
        i_node->hidden = 1;
        i_node->pos_begin = current_open;
        i_node->pos_end = current_close + close_len;
        i_node->type = BLITZ_NODE_TYPE_COMMENT;
        ++(ctx->n_nodes);

        stack_head = ctx->node_stack + ctx->node_stack_len;
        if (stack_head->parent && !stack_head->parent->first_child) {
            if (BLITZ_DEBUG) php_printf("STACK: adding child (%s) to %s\n", i_node->lexem, stack_head->parent->lexem);
            stack_head->parent->first_child = i_node;
        } else if (stack_head->last) {
            if (BLITZ_DEBUG) php_printf("STACK: adding next (%s) to %s\n", i_node->lexem, stack_head->last->lexem);
            stack_head->last->next = i_node;
        }
        stack_head->last = i_node;

        return 1;
    } 

    /* something to be parsed */
    i_error = 0;
    plex = tpl->static_data.body + shift;
    true_lexem_len = 0;
    is_alt_tag = (tag->tag_id == BLITZ_TAG_ID_CLOSE_ALT) ? 1 : 0;
    blitz_parse_call(plex, lexem_len, i_node, &true_lexem_len, BLITZ_G(var_prefix), &i_error TSRMLS_CC);

    if (BLITZ_DEBUG)
        php_printf("parsed lexem: %s\n", i_node->lexem);

    if (i_error) {
        if (is_alt_tag) return 1; /* alternative tags can be just HTML comment tags */
        i_node->hidden = 1;
        if (i_error == BLITZ_CALL_ERROR) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "SYNTAX ERROR: invalid method call (%s: line %lu, pos %lu)",
                tpl->static_data.name, get_line_number(body,current_open), get_line_pos(body,current_open)
            );
        } else if (i_error == BLITZ_CALL_ERROR_IF) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "SYNTAX ERROR: invalid <if> syntax, only 2 or 3 arguments allowed (%s: line %lu, pos %lu)",
                tpl->static_data.name, get_line_number(body,current_open), get_line_pos(body,current_open)
            );
        } else if (i_error == BLITZ_CALL_ERROR_INCLUDE) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "SYNTAX ERROR: invalid <inlcude> syntax, only 1 argument allowed (%s: line %lu, pos %lu)",
                tpl->static_data.name, get_line_number(body,current_open), get_line_pos(body,current_open)
            );
        }
    } else {
        i_node->hidden = 0;
    }

    ctx->node = i_node;
    ctx->current_open = current_open;
    ctx->current_close = current_close;
    ctx->close_len = close_len;
    ctx->true_lexem_len = true_lexem_len;

    if (i_node->type == BLITZ_NODE_TYPE_BEGIN || i_node->type == BLITZ_NODE_TYPE_IF_NF || i_node->type == BLITZ_NODE_TYPE_UNLESS_NF) {
        if (!blitz_analizer_create_parent(ctx, 1 TSRMLS_CC)) {
            return 0;
        }
    } else if (i_node->type == BLITZ_NODE_TYPE_END) {
        blitz_analizer_finalize_parent(ctx, 1 TSRMLS_CC); 
    } else if (i_node->type == BLITZ_NODE_TYPE_ELSEIF_NF || i_node->type == BLITZ_NODE_TYPE_ELSE_NF) {
        blitz_analizer_finalize_parent(ctx, 0 TSRMLS_CC);
        if (!blitz_analizer_create_parent(ctx, 0 TSRMLS_CC)) {
            return 0;
        }
    } else { /* just a var- or call-node */
        i_node->pos_begin = current_open;
        i_node->pos_end = current_close + close_len;
        i_node->lexem_len = true_lexem_len;

        ++(ctx->n_nodes);
        stack_head = ctx->node_stack + ctx->node_stack_len;
        if (stack_head->parent && !stack_head->parent->first_child) {
            if (BLITZ_DEBUG)
                php_printf("STACK: adding child (%s) to %s\n", i_node->lexem, stack_head->parent->lexem);
            stack_head->parent->first_child = i_node;
        } else if (stack_head->last) {
            if (BLITZ_DEBUG)
                php_printf("STACK: adding next (%s) to %s\n", i_node->lexem, stack_head->last->lexem);
            stack_head->last->next = i_node;
        }

        stack_head->last = i_node;
    }

    return 1;
}
/* }}} */

/* {{{ void blitz_analizer_process_node(analizer_ctx *ctx) */
static inline void blitz_analizer_process_node (analizer_ctx *ctx, unsigned int is_last TSRMLS_DC) {
    unsigned char action = 0;
    unsigned char state = 0;

    state = ctx->state;
    action = ctx->action;

    switch (action) {
        case BLITZ_ANALISER_ACTION_IGNORE_CURR:
            break;

        case BLITZ_ANALISER_ACTION_IGNORE_PREV:
            break;

        case BLITZ_ANALISER_ACTION_ERROR_CURR:
            blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->tag->tag_id, ctx->tag->pos TSRMLS_CC);
            break;

        case BLITZ_ANALISER_ACTION_ERROR_PREV:
            blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->prev_tag->tag_id, ctx->prev_tag->pos TSRMLS_CC);
            break;

        case BLITZ_ANALISER_ACTION_ERROR_BOTH:
            blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->tag->tag_id, ctx->tag->pos TSRMLS_CC);
            blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->prev_tag->tag_id, ctx->prev_tag->pos TSRMLS_CC);
            break;

        case BLITZ_ANALISER_ACTION_ADD:
            if (!blitz_analizer_add(ctx TSRMLS_CC)) return;
            break;

        default:
            break;
    }

    /* state is "open" and action is not "ignore"/"error" for current tag: save new open position */
    if ((action <= BLITZ_ANALISER_ACTION_ERROR_PREV) &&
        ((state == BLITZ_ANALISER_STATE_OPEN) ||
        (state == BLITZ_ANALISER_STATE_OPEN_ALT) ||
        (state == BLITZ_ANALISER_STATE_COMMENT_OPEN)))
    {
        ctx->pos_open = ctx->tag->pos;
    }

    if (is_last && state != BLITZ_ANALISER_STATE_NONE)
        blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->tag->tag_id, ctx->tag->pos TSRMLS_CC);

    if (ctx->tag)
        ctx->prev_tag = ctx->tag;

    return;
}
/* }}} */

/* {{{ int blitz_analize(blitz_tpl *tpl TSRMLS_DC) */
static inline int blitz_analize (blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{
    unsigned int i = 0, is_last = 0;
    tag_pos *tags = NULL, *i_tag = NULL;
    blitz_list list_tag;
    blitz_string body_s;
    unsigned int tags_len = 0;
    unsigned int n_open = 0, n_close = 0, n_nodes = 0;
    unsigned char i_tag_id = 0;
    unsigned char i_prev_state = 0;
    analizer_ctx ctx;

    /*********************************/
    /* State transition matrix (STM) */
    /*********************************/
    /*

    TAG ANALYSER BASICS

    S = STATE, A = ACTION
    States:
        -   = NONE (INITIAL / NO TAG)
        SO  = OPEN
        SC  = CLOSE
        SOA = OPEN_ALT
        SCA = CLOSE_ALT
        SOC = COMMENT_OPEN
        SCC = COMMENT_CLOSE
    Actions:
        -   = NONE (INITIAL)
        AA  = PARSE AND ADD NODE
        AEP = PREVIOUS TAG IS ERROR
        AEC = CURRENT TAG IS ERROR
        AIP = IGNORE (SKIP) PREVIOS TAG
        AIC = IGNORE (SKIP) CURRENT TAG
        ABE = BOTH (PREVIOUS AND CURRENT) ARE ERRORS

    Cell format : (action, next state), x = unreachable

    #########################################################################
    prev \ curr #    SO   #   SOA   #    SC   #   SCA   #   SOC   #   SCC   #
    #########################################################################
    -           #   -,SO  #   -,SOA # AEC,-   # AIC,-   #   -,SOC # AEC,-   #
    SO          # AEP,SO  # AEP,SOA #  AA,-   # AEB,-   # AEP,SOC # AEB,-   #
    SOA         # AIP,SO  # AIP,SOA # AIC,SOA #  AA,-   # AIP,SOC # AEB,-   #
    SC          #    x    #    x    #    x    #    x    #    x    #    x    #
    SCA         #    x    #    x    #    x    #    x    #    x    #    x    #
    SOC         # AIC,SOC # AIC,SOC # AIC,SOC # AIC,SOC # AIC,SOC #  AA,-   #
    SCC         #    x    #    x    #    x    #    x    #    x    #    x    #
    #########################################################################

    O/C states differ from alternative OA/CA states because
    alternative tags are just HTML-comments by default, and we shouldn't
    raise errors on simple HTML-comments. Usually it just means that when O/C
    tags lead to "error" actions, OA/CA tags lead to "ingore" actions.

    */

    unsigned char STM[BLITZ_ANALISER_STATE_LIST_LEN][BLITZ_TAG_LIST_LEN][2] = {
        {
            {BLITZ_ANALISER_ACTION_NONE,        BLITZ_ANALISER_STATE_OPEN},
            {BLITZ_ANALISER_ACTION_NONE,        BLITZ_ANALISER_STATE_OPEN_ALT},
            {BLITZ_ANALISER_ACTION_ERROR_CURR,  BLITZ_ANALISER_STATE_NONE},
            {BLITZ_ANALISER_ACTION_IGNORE_CURR, BLITZ_ANALISER_STATE_NONE},
            {BLITZ_ANALISER_ACTION_NONE,        BLITZ_ANALISER_STATE_COMMENT_OPEN},
            {BLITZ_ANALISER_ACTION_ERROR_CURR,  BLITZ_ANALISER_STATE_NONE},
        },
        {
            {BLITZ_ANALISER_ACTION_ERROR_PREV,  BLITZ_ANALISER_STATE_OPEN},
            {BLITZ_ANALISER_ACTION_ERROR_PREV,  BLITZ_ANALISER_STATE_OPEN_ALT}, 
            {BLITZ_ANALISER_ACTION_ADD,         BLITZ_ANALISER_STATE_NONE},
            {BLITZ_ANALISER_ACTION_ERROR_BOTH,  BLITZ_ANALISER_STATE_NONE},
            {BLITZ_ANALISER_ACTION_ERROR_PREV,  BLITZ_ANALISER_STATE_COMMENT_OPEN},
            {BLITZ_ANALISER_ACTION_ERROR_BOTH,  BLITZ_ANALISER_STATE_NONE},
        },
        {
            {BLITZ_ANALISER_ACTION_IGNORE_PREV, BLITZ_ANALISER_STATE_OPEN},
            {BLITZ_ANALISER_ACTION_IGNORE_PREV, BLITZ_ANALISER_STATE_OPEN_ALT}, 
            {BLITZ_ANALISER_ACTION_IGNORE_CURR, BLITZ_ANALISER_STATE_OPEN_ALT},
            {BLITZ_ANALISER_ACTION_ADD,         BLITZ_ANALISER_STATE_NONE},
            {BLITZ_ANALISER_ACTION_IGNORE_PREV, BLITZ_ANALISER_STATE_COMMENT_OPEN},
            {BLITZ_ANALISER_ACTION_ERROR_BOTH,  BLITZ_ANALISER_STATE_NONE},
        },
        {
            {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
        },
        {
            {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
        },
        {
            {BLITZ_ANALISER_ACTION_IGNORE_CURR, BLITZ_ANALISER_STATE_COMMENT_OPEN},
            {BLITZ_ANALISER_ACTION_IGNORE_CURR, BLITZ_ANALISER_STATE_COMMENT_OPEN},
            {BLITZ_ANALISER_ACTION_IGNORE_CURR, BLITZ_ANALISER_STATE_COMMENT_OPEN},
            {BLITZ_ANALISER_ACTION_IGNORE_CURR, BLITZ_ANALISER_STATE_COMMENT_OPEN},
            {BLITZ_ANALISER_ACTION_IGNORE_CURR, BLITZ_ANALISER_STATE_COMMENT_OPEN},
            {BLITZ_ANALISER_ACTION_ADD,         BLITZ_ANALISER_STATE_NONE},
        },
        {
            {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
        },
    };

    if (!tpl->static_data.body || (0 == tpl->static_data.body_len)) {
        return 0;
    }

    /* find all tag positions */
    list_tag.first = ecalloc(BLITZ_ALLOC_TAGS_STEP, sizeof(tag_pos));
    list_tag.size = 0;
    list_tag.item_size = sizeof(tag_pos);
    list_tag.allocated = BLITZ_ALLOC_TAGS_STEP;
    list_tag.end = list_tag.first;

    body_s.s = tpl->static_data.body;
    body_s.len = tpl->static_data.body_len;
    blitz_find_tag_positions(&body_s, &list_tag TSRMLS_CC);

    tags = (tag_pos*)list_tag.first;
    tags_len = list_tag.size;

    for (i=0; i<tags_len; i++) {
        i_tag_id = tags[i].tag_id;
        if ((i_tag_id == BLITZ_TAG_ID_OPEN) || (i_tag_id == BLITZ_TAG_ID_OPEN_ALT) || (i_tag_id == BLITZ_TAG_ID_COMMENT_OPEN))
            n_open++;
        if ((i_tag_id == BLITZ_TAG_ID_CLOSE) || (i_tag_id == BLITZ_TAG_ID_CLOSE_ALT) || (i_tag_id == BLITZ_TAG_ID_COMMENT_CLOSE))
            n_close++;
    }

    if (BLITZ_DEBUG) {
        for (i = 0; i < tags_len; i++) {
            php_printf("pos = %ld, %d\n", tags[i].pos, tags[i].tag_id);
        }
    }

    /* allocate memory for nodes */
    if (n_close > 0) {
        tpl->static_data.nodes = ecalloc(n_close, sizeof(blitz_node));
    }

    i_prev_state = BLITZ_ANALISER_STATE_NONE;
    i_tag = tags;

    ctx.tpl = tpl;
    memset(ctx.node_stack, 0, BLITZ_ANALIZER_NODE_STACK_LEN*sizeof(analizer_stack_elem));
    ctx.node_stack_len = 0;
    ctx.n_nodes = 0;
    ctx.n_open = n_open;
    ctx.n_close = n_close;
    ctx.action = 0;
    ctx.state = 0;
    ctx.pos_open = 0;
    ctx.tag = NULL;
    ctx.prev_tag = NULL;

    for (i=0; i < tags_len; i++, i_tag++) {
        i_tag_id = i_tag->tag_id;

        ctx.action = STM[i_prev_state][i_tag_id][0];
        ctx.state = STM[i_prev_state][i_tag_id][1];
        if (BLITZ_DEBUG) {
            php_printf("tag_id = %u, prev_state = %u, action = %u, state = %u\n", 
                i_tag_id, i_prev_state, ctx.action, ctx.state
            );
        } 

        ctx.tag = i_tag;
        is_last = (i + 1 - tags_len) ? 0 : 1;
        blitz_analizer_process_node(&ctx, is_last TSRMLS_CC);

        i_prev_state = ctx.state;
    }

    tpl->static_data.n_nodes = ctx.n_nodes;
    if (BLITZ_G(remove_spaces_around_context_tags)) {
        blitz_remove_spaces_around_context_tags(tpl TSRMLS_CC);
    }

    efree(list_tag.first);

    if (BLITZ_G(warn_context_duplicates)) {
        blitz_warn_context_duplicates(tpl TSRMLS_CC);
    }

    return 1;
}
/* }}} */

/* {{{ void blitz_remove_spaces_around_context_tags(blitz_tpl *tpl) */
static inline void blitz_remove_spaces_around_context_tags(blitz_tpl *tpl TSRMLS_DC) {
    char *pc = NULL;
    char c = 0;
    char got_end_back = 0, got_end_fwd = 0, got_non_space = 0;
    unsigned int shift = 0, shift_tmp = 0, n_nodes = 0, i = 0;
    blitz_node *i_node = NULL;

    n_nodes = tpl->static_data.n_nodes;
    // fix whitespaces
    for (i=0; i<n_nodes; i++) {
        got_end_back = 0;
        got_end_fwd = 0;
        got_non_space = 0;

        i_node = tpl->static_data.nodes + i;
        if (i_node->type != BLITZ_NODE_TYPE_CONTEXT 
            && i_node->type != BLITZ_NODE_TYPE_END 
            && i_node->type != BLITZ_NODE_TYPE_IF_CONTEXT 
            && i_node->type != BLITZ_NODE_TYPE_UNLESS_CONTEXT 
            && i_node->type != BLITZ_NODE_TYPE_ELSEIF_CONTEXT 
            && i_node->type != BLITZ_NODE_TYPE_ELSE_CONTEXT)
        {
            continue;
        }

        if (BLITZ_DEBUG) {
            php_printf("lexem: %s, pos_begin = %u, pos_begin_shift = %u, pos_end = %u, pos_end_shift = %u\n",
                i_node->lexem, i_node->pos_begin, i_node->pos_begin_shift, i_node->pos_end, i_node->pos_end_shift);
        }

        // begin tag: scan back to the endline or any nonspace symbol
        shift = i_node->pos_begin;
        if (shift) shift--;
        pc = tpl->static_data.body + shift;
        while (shift) {
            c = *pc;
            if (c == '\n') {
                if (shift) shift++;
                got_end_back = 1;
                break;
            } else if (c != ' ' && c != '\t' && c != '\r') {
                got_non_space = 1;
                break;
            }
            shift--;
            pc--;
        }

        if (got_end_back || (0 == got_non_space && 0 == shift)) { // got the line end or just moved to the very beginning
            shift_tmp = shift;
            got_non_space = 0;

            // begin tag: scan forward to the endline or any nonspace symbol
            shift = tpl->static_data.body_len - i_node->pos_begin_shift - 1;
            pc = tpl->static_data.body + i_node->pos_begin_shift - 1;
            while (shift) {
                shift--;
                pc++;
                c = *pc;
                if (c == '\n') {
                    got_end_fwd = 1;
                    break;
                } else if (c != ' ' && c != '\t' && c != '\r') {
                    got_non_space = 1;
                    break;
                }
            }

            if (got_end_fwd || (0 == got_non_space && 0 == shift)) { // got the line end or just moved to the very end
                i_node->pos_begin = shift_tmp;
                i_node->pos_begin_shift = tpl->static_data.body_len - shift - 1;
                if (BLITZ_DEBUG) {
                    php_printf("new: pos_begin = %u, pos_begin_shift = %u\n", i_node->pos_begin, i_node->pos_begin_shift);
                }
            }
        }

        got_end_back = 0;
        got_end_fwd = 0;
        got_non_space = 0;

        // end tag: scan back to the endline or any nonspace symbol
        shift = i_node->pos_end_shift;
        pc = tpl->static_data.body + shift;

        while (shift) {
            shift--;
            pc--;
            c = *pc;
            if (c == '\n') {
                got_end_back = 1;
                break;
            } else if (c != ' ' && c != '\t' && c != '\r') {
                got_non_space = 1;
                break;
            }
        }

        if (got_end_back || (0 == got_non_space && 0 == shift)) { // got the line end or just moved to the very beginning
            shift_tmp = shift;
            got_non_space = 0;

            // end tag: scan forward to the endline or any nonspace symbol
            shift = tpl->static_data.body_len - i_node->pos_end;
            pc = tpl->static_data.body + i_node->pos_end - 1;
            while (shift) {
                shift--;
                pc++;
                c = *pc;
                if (c == '\n') {
                    got_end_fwd = 1;
                    break;
                } else if (c != ' ' && c != '\t' && c != '\r' && c != '\x0') {
                    got_non_space = 1;
                    break;
                }
            }

            if (got_end_fwd || (0 == got_non_space && 0 == shift)) { // got the line end or just moved to the very end
                i_node->pos_end_shift = shift_tmp + 1;
                i_node->pos_end = tpl->static_data.body_len - shift;
                if (BLITZ_DEBUG) {
                    php_printf("new: pos_end = %u, pos_end_shift = %u\n", i_node->pos_end, i_node->pos_end_shift);
                }
            }
        }
    }
}
/* }}} */

/* {{{ int blitz_exec_wrapper() */
static inline int blitz_exec_wrapper(char **result, int *result_len, unsigned long type, int args_num, char **args, int *args_len, char *tmp_buf TSRMLS_DC)
{
    /* following wrappers are to be added: escape, date, gettext, something else?... */
    if (type == BLITZ_NODE_TYPE_WRAPPER_ESCAPE) {
        char *quote_str = args[1];
        long quote_style = ENT_QUOTES;
        char *charset = NULL;
        int all = 0, wrong_format = 0, quote_str_len = 0;
        
        if (quote_str) {
            quote_str_len = strlen(quote_str);
            if (quote_str_len == 0) {
                wrong_format = 1;
            } else if (0 == strncmp("ENT_", quote_str, 4)) {
                char *p_str = quote_str + 4;
                if (0 == strncmp("QUOTES", p_str, 6)) {
                    quote_style = ENT_QUOTES;
                } else if (0 == strncmp("COMPAT", p_str, 6)) {
                    quote_style = ENT_COMPAT;
                } else if (0 == strncmp("NOQUOTES", p_str, 8)) {
                    quote_style = ENT_NOQUOTES;
                } else {
                    wrong_format = 1;
                }
            } else {
                wrong_format = 1;
            }
        }

        if (wrong_format) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "escape format error (\"%s\"), available formats are ENT_QUOTES, ENT_COMPAT, ENT_NOQUOTES", quote_str
            );
            return 0;
        }

        if (BLITZ_G(charset))
            charset = BLITZ_G(charset);

        *result = php_escape_html_entities((unsigned char *)args[0], args_len[0], result_len, all, quote_style, charset TSRMLS_CC);
    } else if (type == BLITZ_NODE_TYPE_WRAPPER_DATE) {
/* FIXME: check how it works under Windows */
        char *format = NULL;
        time_t timestamp = 0;
        struct tm *ta = NULL, tmbuf;
        int gm = 1; /* use localtime */
        char *p = NULL;
        int i = 0;

        /* check format */
        if (args_num>0) {
            format = args[0];
        }

        if (!format) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "date syntax error"
            );
            return 0;
        }

        /* use second argument or get current time */
        if (args_num == 1) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            timestamp = tv.tv_sec;
        } else {
            p = args[1];
            i = args_len[1];
            if (!p) {
                timestamp = 0;
            } else {
                while (BLITZ_IS_NUMBER(*p) && i--) {
                    p++;
                }
                if (i>0) { /* string? try to parse */
                    timestamp = php_parse_date(args[1], NULL);
                } else { /* numirical - treat as unixtimestamp */
                    timestamp = atol(args[1]);
                }
            }
        }

        if (gm) {
            ta = php_gmtime_r(&timestamp, &tmbuf);
        } else {
            ta = php_localtime_r(&timestamp, &tmbuf);
        }
        *result_len = strftime(tmp_buf, BLITZ_TMP_BUF_MAX_LEN, format, ta);
        *result = tmp_buf;
    }

    return 1;
}
/* }}} */

static inline blitz_scope_stack_find(blitz_tpl *tpl, char *key, unsigned long key_len, zval ***zparam TSRMLS_DC) /* {{{ */
{
    unsigned long i = 1;
    unsigned long lookup_limit = BLITZ_G(scope_lookup_limit);
    zval *data = NULL;

    if (tpl->scope_stack_pos <= lookup_limit)
        lookup_limit = tpl->scope_stack_pos - 1;

    while (i <= lookup_limit) {
        data = tpl->scope_stack[tpl->scope_stack_pos - i - 1];
        if (SUCCESS == zend_hash_find(Z_ARRVAL_P(data), key, key_len, (void **) zparam)) {
            return i;
        }
        i++;
    }

    return 0;
}
/* }}} */

static inline int blitz_fetch_var_by_path(zval ***zparam, const char *lexem, int lexem_len, zval *params, blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{
    register int i = 0, j = 0, last_pos = 0, key_len = 0, is_last = 0;
    char key[256];
    char root_found = 0;
    char use_scope = 0, found_in_scope = 0;
    unsigned long loop_index = 0, stack_level = 0;

    last_pos = 0;
    j = lexem_len - 1;
    /* walk through the path */
    for (i=0; i<lexem_len; i++, j--) {
        is_last = (j == 0);
        if (('.' == lexem[i]) || is_last) {
            key_len = i - last_pos + is_last;
            memcpy(key, lexem + last_pos, key_len);
            key[key_len] = '\x0';

            /* try to get data by the key */
            if (0 == root_found) { /* globals or params? */
                root_found = (params && (SUCCESS == zend_hash_find(Z_ARRVAL_P(params), key, key_len + 1, (void **) zparam)));
                if (!root_found) {
                    root_found = (tpl->hash_globals && (SUCCESS == zend_hash_find(tpl->hash_globals, key, key_len + 1, (void**)zparam)));
                    if (!root_found) {
                        use_scope = BLITZ_G(scope_lookup_limit) && tpl->scope_stack_pos;
                        if (use_scope) {
                            stack_level = blitz_scope_stack_find(tpl, key, key_len + 1, zparam TSRMLS_CC);
                            if (stack_level == 0) {
                                return 0;
                            }
                            found_in_scope = 1;
                        }
                    }
                }
            } else if (*zparam) { /* just propagate through elem */
                if (Z_TYPE_PP(*zparam) == IS_ARRAY) {
                    if (SUCCESS != zend_hash_find(Z_ARRVAL_PP(*zparam), key, key_len + 1, (void **) zparam)) {
                        // when using scope 'list.item' can be 1) list->item and 2) list->[loop_index]->item
                        // thus when list->item is not found whe have to test list->[loop_index]->item
                        if (found_in_scope == 0) {
                            return 0;
                        } else {
                            loop_index = tpl->loop_stack[tpl->loop_stack_level - stack_level + 1].current;
                            // move to list->[loop_index]
                            if (SUCCESS != zend_hash_index_find(Z_ARRVAL_PP(*zparam), loop_index, (void **) zparam))
                                return 0;
                            // try to find the key again
                            if (SUCCESS != zend_hash_find(Z_ARRVAL_PP(*zparam), key, key_len + 1, (void **) zparam))
                                return 0;
                            // when we found everything - decrease stack_level (we may have long loop path)
                            stack_level--;
                        }
                    }
                } else if (Z_TYPE_PP(*zparam) == IS_OBJECT) {
                    if (SUCCESS != zend_hash_find(Z_OBJPROP_PP(*zparam), key, key_len + 1, (void **) zparam)) {
                        return 0;
                    }
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
            root_found = 1;
            last_pos = i + 1;
        }
    }

    return 1;
}
/* }}} */

/* {{{ int blitz_exec_predefined_method() */
static inline int blitz_exec_predefined_method(blitz_tpl *tpl, blitz_node *node, zval *iteration_params, zval *id,
    char **result, char **p_result, unsigned long *result_len, unsigned long *result_alloc_len, char *tmp_buf TSRMLS_DC)
{
    zval **z = NULL;
    unsigned long buf_len = 0, new_len = 0;
    char is_var = 0, is_var_path = 0, is_found = 0;
    char predefined_buf[BLITZ_PREDEFINED_BUF_LEN];

    if (node->type == BLITZ_NODE_TYPE_IF || node->type == BLITZ_NODE_TYPE_UNLESS) {
        char not_empty = 0;
        int predefined = -1;
        unsigned int i_arg = 0;
        call_arg *arg = NULL;
        zval **z = NULL;
        unsigned int use_scope = 0;

        arg = node->args;
        BLITZ_GET_PREDEFINED_VAR(tpl, arg->name, arg->len, predefined);
        if (predefined >=0) {
            if (predefined != 0) {
                not_empty = 1;
            }
        } else if (arg->type == BLITZ_ARG_TYPE_VAR) {
            if (iteration_params) {
                BLITZ_ARG_NOT_EMPTY(*arg, Z_ARRVAL_P(iteration_params), not_empty);
                if (not_empty == -1) {
                    BLITZ_ARG_NOT_EMPTY(*arg, tpl->hash_globals, not_empty);
                    if (not_empty == -1) {
                        use_scope = BLITZ_G(scope_lookup_limit) && tpl->scope_stack_pos;
                        if (use_scope && blitz_scope_stack_find(tpl, arg->name, arg->len + 1, &z TSRMLS_CC)) {
                            BLITZ_ZVAL_NOT_EMPTY(z, not_empty);
                        }
                    }
                }
            }
        } else if (arg->type == BLITZ_ARG_TYPE_VAR_PATH) {
            if (blitz_fetch_var_by_path(&z, arg->name, arg->len, iteration_params, tpl TSRMLS_CC)) {
                BLITZ_ZVAL_NOT_EMPTY(z, not_empty);
            }
        } else if (arg->type == BLITZ_ARG_TYPE_BOOL) {
            not_empty = (arg->name[0] == 't');
        }

        /* not_empty = 
            1: found, not empty 
            0: found, empty 
           -1: not found, "empty" */
        if (not_empty == 1) { 
            if (node->type == BLITZ_NODE_TYPE_IF) {
                i_arg = 1; /* if ($a, 1) or if ($a, 1, 2) and $a is not empty */
            } else {
                if (node->n_args == 3) { 
                    i_arg = 2; /* unless($a, 1, 2) and $a is not empty */
                } else {
                    return 1;  /* unless($a, 1) and $a is not empty */
                }
            }
        } else {
            if (node->type == BLITZ_NODE_TYPE_UNLESS) {
                i_arg = 1; /* unless($a, 1) or unless($a, 1, 2) and $a is empty */
            } else {
                if (node->n_args == 3) { 
                    i_arg = 2; /* if($a, 1, 2) and $a is empty */
                } else { 
                    return 1; /* if($a, 1) and $a is empty */
                }
            }
        }

        arg = node->args + i_arg;
        BLITZ_GET_PREDEFINED_VAR(tpl, arg->name, arg->len, predefined);
        if (predefined >= 0) {
            snprintf(predefined_buf, BLITZ_PREDEFINED_BUF_LEN, "%u", predefined);
            buf_len = strlen(predefined_buf);
            BLITZ_REALLOC_RESULT(buf_len, new_len, *result_len, *result_alloc_len, *result, *p_result);
            *p_result = (char*)memcpy(*p_result, predefined_buf, buf_len);
            *result_len += buf_len;
            p_result+=*result_len;
            (*result)[*result_len] = '\0';
        } else {
            is_var = (arg->type == BLITZ_ARG_TYPE_VAR);
            is_var_path = (arg->type == BLITZ_ARG_TYPE_VAR_PATH);
            is_found = 0;
            if (is_var || is_var_path) { /* argument is variable  */
                if (is_var &&
                    (
                        (iteration_params && SUCCESS == zend_hash_find(Z_ARRVAL_P(iteration_params),arg->name,1+arg->len,(void**)&z))
                        || (SUCCESS == zend_hash_find(tpl->hash_globals,arg->name,1+arg->len,(void**)&z))
                    ))
                {
                    is_found = 1;
                }
                else if (is_var_path 
                    && blitz_fetch_var_by_path(&z, arg->name, arg->len, iteration_params, tpl TSRMLS_CC))
                {
                    is_found = 1;
                }

                if (is_found) {
                    convert_to_string_ex(z);
                    buf_len = Z_STRLEN_PP(z);
                    BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,*p_result);
                    *p_result = (char*)memcpy(*p_result,Z_STRVAL_PP(z),buf_len);
                    *result_len += buf_len;
                    p_result+=*result_len;
                    (*result)[*result_len] = '\0';
                }
            } else { /* argument is string or number */
                buf_len = (unsigned long)arg->len;
                BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,*p_result);
                *p_result = (char*)memcpy(*p_result,node->args[i_arg].name,buf_len);
                *result_len += buf_len;
                p_result+=*result_len;
                (*result)[*result_len] = '\0';
            }
        }
    } else if (node->type == BLITZ_NODE_TYPE_INCLUDE) {
        call_arg *arg = node->args; 
        char *filename = arg->name;
        unsigned int arg_type = arg->type;
        int filename_len = arg->len;
        char *inner_result = NULL;
        unsigned long inner_result_len = 0;
        blitz_tpl *itpl = NULL;
        int res = 0, found = 0;

        if (BLITZ_G(disable_include)) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "disable_include restriction in effect: include() ignored");
            return 0;
        }

        if (arg_type != BLITZ_ARG_TYPE_STR) {
            if (arg_type == BLITZ_ARG_TYPE_VAR) {
                if ((iteration_params && SUCCESS == zend_hash_find(Z_ARRVAL_P(iteration_params),arg->name,1+arg->len,(void**)&z))
                    || (SUCCESS == zend_hash_find(tpl->hash_globals,arg->name,1+arg->len,(void**)&z)))
                {
                    found = 1;
                }
            } else if (arg_type == BLITZ_ARG_TYPE_VAR_PATH) {
                if (blitz_fetch_var_by_path(&z, arg->name, arg->len, iteration_params, tpl TSRMLS_CC)) {
                    found = 1;
                }
            }

            if (found) {
                convert_to_string_ex(z);
                filename = Z_STRVAL_PP(z);
                filename_len = Z_STRLEN_PP(z);
            }
        }

        if (!blitz_include_tpl_cached(tpl, filename, filename_len, iteration_params, &itpl TSRMLS_CC)) {
            return 0;
        }

        /* parse */
        if ((res = blitz_exec_template(itpl,id,&inner_result,&inner_result_len TSRMLS_CC))) {
            BLITZ_REALLOC_RESULT(inner_result_len,new_len,*result_len,*result_alloc_len,*result,*p_result);
            *p_result = (char*)memcpy(*p_result,inner_result,inner_result_len);
            *result_len += inner_result_len;
            p_result+=*result_len;
            (*result)[*result_len] = '\0';

            if (res == 1) {
                efree(inner_result);
            }
        }

    } else if (node->type >= BLITZ_NODE_TYPE_WRAPPER_ESCAPE && node->type < BLITZ_NODE_TYPE_IF_NF) {
        char *wrapper_args[BLITZ_WRAPPER_MAX_ARGS];
        int  wrapper_args_len[BLITZ_WRAPPER_MAX_ARGS];
        char *str = NULL;
        int str_len = 0;
        call_arg *p_arg = NULL;
        int i = 0;
        int wrapper_args_num = 0;

        for(i=0; i<BLITZ_WRAPPER_MAX_ARGS; i++) {
            wrapper_args[i] = NULL;
            wrapper_args_len[i] = 0;

            if (i<node->n_args) {
                p_arg = node->args + i;
                wrapper_args_num++;
                if (p_arg->type == BLITZ_ARG_TYPE_VAR) {
                    if ((iteration_params && SUCCESS == zend_hash_find(Z_ARRVAL_P(iteration_params),p_arg->name,1+p_arg->len,(void**)&z))
                        || (SUCCESS == zend_hash_find(tpl->hash_globals,p_arg->name,1+p_arg->len,(void**)&z)))
                    {
                        convert_to_string_ex(z);
                        wrapper_args[i] = Z_STRVAL_PP(z);
                        wrapper_args_len[i] = Z_STRLEN_PP(z);
                    }
                } else if (p_arg->type == BLITZ_ARG_TYPE_VAR_PATH) {
                    if (blitz_fetch_var_by_path(&z, p_arg->name, p_arg->len, iteration_params, tpl TSRMLS_CC)) {
                        convert_to_string_ex(z);
                        wrapper_args[i] = Z_STRVAL_PP(z);
                        wrapper_args_len[i] = Z_STRLEN_PP(z);
                    }
                } else {
                    wrapper_args[i] = p_arg->name;
                    wrapper_args_len[i] = p_arg->len;
                }
            }
        }

        if (blitz_exec_wrapper(&str, &str_len, node->type, wrapper_args_num, wrapper_args, wrapper_args_len, tmp_buf TSRMLS_CC)) {
            BLITZ_REALLOC_RESULT(str_len, new_len, *result_len, *result_alloc_len, *result, *p_result);
            *p_result = (char*)memcpy(*p_result, str, str_len);
            *result_len += str_len;
            p_result+=*result_len;
            (*result)[*result_len] = '\0';
            if (str && str != tmp_buf) {
                efree(str);
            }
        } else {
            return 0;
        }
    }

    return 1;
}
/* }}} */

/* {{{ int blitz_exec_user_method() */
static inline int blitz_exec_user_method(blitz_tpl *tpl, blitz_node *node, zval **iteration_params, zval *obj, char **result, char **p_result, unsigned long *result_len, unsigned long *result_alloc_len TSRMLS_DC)
{
    zval *retval = NULL, *zmethod = NULL, **ztmp = NULL;
    int method_res = 0;
    unsigned long buf_len = 0, new_len = 0;
    zval ***args = NULL; 
    zval **pargs = NULL;
    unsigned int i = 0;
    call_arg *i_arg = NULL;
    char i_arg_type = 0;
    char cl = 0;
    int predefined = -1, has_iterations = 0;
    zval **old_caller_iteration = NULL; 
    blitz_tpl *tpl_caller = NULL;
    zend_function *func = NULL;
    HashTable *function_table = NULL;
   
    MAKE_STD_ZVAL(zmethod);
    ZVAL_STRING(zmethod, node->lexem, 1);

    has_iterations = ((iteration_params > 0) && (*iteration_params) > 0); // fetch has NULL *iteration_params

    /* prepare arguments if needed */
    /* 2DO: probably there's much more easy way to prepare arguments without two emalloc */
    if (node->n_args>0 && node->args) {
        /* dirty trick to pass arguments */
        /* pargs are zval ** with parameter data */
        /* args just point to pargs */
        args = emalloc(node->n_args*sizeof(zval*));
        pargs = emalloc(node->n_args*sizeof(zval));
        for(i=0; i<node->n_args; i++) {
            args[i] = NULL;
            ALLOC_INIT_ZVAL(pargs[i]);
            ZVAL_NULL(pargs[i]);
            i_arg  = node->args + i;
            i_arg_type = i_arg->type;
            if (i_arg_type == BLITZ_ARG_TYPE_VAR) {
                predefined = -1;
                BLITZ_GET_PREDEFINED_VAR(tpl, i_arg->name, i_arg->len, predefined);
                if (predefined>=0) {
                    ZVAL_LONG(pargs[i],(long)predefined);
                } else {
                    if ((has_iterations && (SUCCESS == zend_hash_find(Z_ARRVAL_PP(iteration_params),i_arg->name,1+i_arg->len,(void**)&ztmp)))
                        || (SUCCESS == zend_hash_find(tpl->hash_globals,i_arg->name,1+i_arg->len,(void**)&ztmp)))
                    {
                        args[i] = ztmp;
                    }
                }
            } else if (i_arg_type == BLITZ_ARG_TYPE_VAR_PATH) {
                if (has_iterations && blitz_fetch_var_by_path(&ztmp, i_arg->name, i_arg->len, *iteration_params, tpl TSRMLS_CC)) {
                    args[i] = ztmp;
                }
            } else if (i_arg_type == BLITZ_ARG_TYPE_NUM) {
                ZVAL_LONG(pargs[i],atol(i_arg->name));
            } else if (i_arg_type == BLITZ_ARG_TYPE_BOOL) {
                cl = *i_arg->name;
                if (cl == 't') { 
                    ZVAL_TRUE(pargs[i]);
                } else if (cl == 'f') {
                    ZVAL_FALSE(pargs[i]);
                }
            } else { 
                ZVAL_STRING(pargs[i],i_arg->name,1);
            }
            if (!args[i]) args[i] = & pargs[i];
        }
    } 

    /* call object method */
    tpl->flags |= BLITZ_FLAG_CALLED_USER_METHOD;

    // Caller trick is used to pass parent iteration structure correctly when includes are
    // called inside the method. When calling includes recursively "tpl" objects have one and
    // the same parent because only parent template is "object" represented by obj. 
    // All included templates obtain their initial iteration states through parent->caller_iteration poiner.
    // Thus included template which includes another template saves parent caller state, sets new parent
    // caller state, executes included template, and sets parent caller state back.
    if (tpl->tpl_parent) {
        tpl_caller = tpl->tpl_parent;
    } else {
        tpl_caller = tpl;
    }

    old_caller_iteration = tpl_caller->caller_iteration;
    tpl_caller->caller_iteration = iteration_params;

    // class method or external (PHP/plugin) function?
    function_table = &Z_OBJCE_P(obj)->function_table;
    if (node->namespace_code == BLITZ_NODE_THIS_NAMESPACE) {
        method_res = call_user_function_ex(function_table, &obj, zmethod, &retval, node->n_args, args, 1, NULL TSRMLS_CC);
    } else if (node->namespace_code) {
        method_res = call_user_function_ex(NULL, NULL, zmethod, &retval, node->n_args, args, 1, NULL TSRMLS_CC);
    } else {
        method_res = call_user_function_ex(NULL, NULL, zmethod, &retval, node->n_args, args, 1, NULL TSRMLS_CC);
        if (method_res == FAILURE) {
            method_res = call_user_function_ex(function_table, &obj, zmethod, &retval, node->n_args, args, 1, NULL TSRMLS_CC);
        }
    }

    tpl_caller->caller_iteration = old_caller_iteration;

    tpl->flags &= ~BLITZ_FLAG_CALLED_USER_METHOD;

    if (method_res == FAILURE) { /* failure: */
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "INTERNAL ERROR: calling function \"%s\" failed, check if this function exists or parameters are valid", node->lexem);
    } else if (retval) { /* retval can be empty even in success: method throws exception */
        convert_to_string_ex(&retval);
        buf_len = Z_STRLEN_P(retval);
        BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,*p_result);
        *p_result = (char*)memcpy(*p_result,Z_STRVAL_P(retval), buf_len);
        *result_len += buf_len;
        p_result+=*result_len;
        (*result)[*result_len] = '\0';
    }

    zval_ptr_dtor(&zmethod);
    
    if (retval)
        zval_ptr_dtor(&retval);

    if (pargs) {
         for(i=0; i<node->n_args; i++) {
             zval_ptr_dtor(pargs + i);
         }
         efree(args);
         efree(pargs);
    }

    return 1;
}
/* }}} */

/* {{{ int blitz_exec_var() */
static inline void blitz_exec_var(blitz_tpl *tpl, const char *lexem, zval *params, char **result, unsigned long *result_len, unsigned long *result_alloc_len TSRMLS_DC)
{
    /* FIXME: there should be just node->lexem_len+1, but method.phpt test becomes broken. REMOVE STRLEN! */
    unsigned int lexem_len = strlen(lexem);
    unsigned int lexem_len_p1 = lexem_len + 1;
    unsigned long buf_len = 0, new_len = 0;
    zval **zparam = NULL;
    char *p_result = *result;
    int predefined = -1;
    char predefined_buf[BLITZ_PREDEFINED_BUF_LEN];

    BLITZ_GET_PREDEFINED_VAR(tpl, lexem, lexem_len, predefined);
    if (predefined>=0) {
        snprintf(predefined_buf, BLITZ_PREDEFINED_BUF_LEN, "%u", predefined);
        buf_len = strlen(predefined_buf);
        BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,p_result);
        p_result = (char*)memcpy(p_result, predefined_buf, buf_len);
        *result_len += buf_len;
        p_result+=*result_len;
        (*result)[*result_len] = '\0';
    } else {
        if ( 
            (params && (SUCCESS == zend_hash_find(Z_ARRVAL_P(params), (char *)lexem, lexem_len_p1, (void**)&zparam)))
            ||
            (tpl->hash_globals && (SUCCESS == zend_hash_find(tpl->hash_globals, (char *)lexem, lexem_len_p1, (void**)&zparam)))
            || 
            (tpl->scope_stack_pos && BLITZ_G(scope_lookup_limit) && blitz_scope_stack_find(tpl, (char *)lexem, lexem_len_p1, &zparam TSRMLS_CC))
        ) {
            if (Z_TYPE_PP(zparam) != IS_STRING) {
                zval p;

                p = **zparam;
                zval_copy_ctor(&p);
                convert_to_string(&p);

                buf_len = Z_STRLEN(p);
                BLITZ_REALLOC_RESULT(buf_len, new_len, *result_len, *result_alloc_len, *result,p_result);
                p_result = (char*)memcpy(p_result, Z_STRVAL(p), buf_len);
				zval_dtor(&p);
            } else {
               buf_len = Z_STRLEN_PP(zparam);
               BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,p_result);
               p_result = (char*)memcpy(p_result,Z_STRVAL_PP(zparam), buf_len);
            }

            *result_len += buf_len;
            p_result+=*result_len;
            (*result)[*result_len] = '\0';
        }
    }
}
/* }}} */

/* {{{ int blitz_exec_var_path() */
static inline void blitz_exec_var_path(blitz_tpl *tpl, const char *lexem, zval *params, char **result, unsigned long *result_len, unsigned long *result_alloc_len TSRMLS_DC)
{
    /* FIXME: there should be just node->lexem_len+1, but method.phpt test becomes broken. REMOVE STRLEN! */
    unsigned int lexem_len = strlen(lexem);
    unsigned long buf_len = 0, new_len = 0;
    char *p_result = *result;
    zval **elem = NULL;

    if (!blitz_fetch_var_by_path(&elem, lexem, lexem_len, params, tpl TSRMLS_CC))
        return;

    if (!elem)
        return;

    if (Z_TYPE_PP(elem) != IS_STRING) {
        zval p;
        
        p = **elem;
        zval_copy_ctor(&p);
        convert_to_string(&p);

        buf_len = Z_STRLEN(p);
        BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,p_result);
        p_result = (char*)memcpy(p_result,Z_STRVAL(p), buf_len);
        zval_dtor(&p);
    } else {
       buf_len = Z_STRLEN_PP(elem);
       BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,p_result);
       p_result = (char*)memcpy(p_result,Z_STRVAL_PP(elem), buf_len);
    }

    *result_len += buf_len;
    p_result+=*result_len;
    (*result)[*result_len] = '\0';

}
/* }}} */

/* {{{ int blitz_exec_context() */
static void blitz_exec_context(blitz_tpl *tpl, blitz_node *node, zval *parent_params, zval *id,
    char **result, unsigned long *result_len, unsigned long *result_alloc_len TSRMLS_DC)
{
    char *key = NULL;
    unsigned int key_len = 0;
    unsigned long key_index = 0;
    int check_key = 0, not_empty = 0;
    zval **ctx_iterations = NULL;
    zval **ctx_data = NULL;
    call_arg *arg = node->args;
    zval **z = NULL;
    int predefined = -1;
    int use_scope = 0;

    if (BLITZ_DEBUG) php_printf("blitz_exec_context: %s\n",node->args->name);

    if (!parent_params) return; 

    check_key = zend_hash_find(Z_ARRVAL_P(parent_params), arg->name, arg->len + 1, (void**)&ctx_iterations);
    if (check_key == FAILURE) {
        if (node->type == BLITZ_NODE_TYPE_CONTEXT) {
            return;
        } else {
            not_empty = 0;
        }
    } else {
        BLITZ_ZVAL_NOT_EMPTY(ctx_iterations, not_empty);
    }

    if (BLITZ_DEBUG) {
        php_printf("will exec context (type=%u) with iterations:\n", node->type);
        php_var_dump(ctx_iterations, 0 TSRMLS_CC);
        php_printf("not_empty = %u\n", not_empty);
    }

    if (BLITZ_DEBUG) php_printf("data found in parent params:%s\n",arg->name);
    if (Z_TYPE_PP(ctx_iterations) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_PP(ctx_iterations))) {
        if (BLITZ_DEBUG) php_printf("will walk through iterations\n");

        zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(ctx_iterations), NULL);
        check_key = zend_hash_get_current_key_ex(Z_ARRVAL_PP(ctx_iterations), &key, &key_len, &key_index, 0, NULL);
        if (BLITZ_DEBUG) php_printf("KEY_CHECK: %d\n", check_key);

        if (HASH_KEY_IS_STRING == check_key) {
            if (BLITZ_DEBUG) php_printf("KEY_CHECK: string\n");
            blitz_exec_nodes(tpl, node->first_child, id,
                result, result_len, result_alloc_len,
                node->pos_begin_shift,
                node->pos_end_shift,
                *ctx_iterations TSRMLS_CC
            );
        } else if (HASH_KEY_IS_LONG == check_key) {
            if (BLITZ_DEBUG) php_printf("KEY_CHECK: long\n");
            BLITZ_LOOP_INIT(tpl, zend_hash_num_elements(Z_ARRVAL_PP(ctx_iterations)));
            while (zend_hash_get_current_data_ex(Z_ARRVAL_PP(ctx_iterations),(void**) &ctx_data, NULL) == SUCCESS) {

                if (BLITZ_DEBUG) {
                    php_printf("GO subnode, params:\n");
                    php_var_dump(ctx_data,0 TSRMLS_CC);
                }
                /* mix of num/str errors: array(0=>array(), 'key' => 'val') */
                if (IS_ARRAY != Z_TYPE_PP(ctx_data)) {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING,
                        "ERROR: You have a mix of numerical and non-numerical keys in the iteration set "
                        "(context: %s, line %lu, pos %lu), key was ignored",
                        node->args[0].name,
                        get_line_number(tpl->static_data.body, node->pos_begin),
                        get_line_pos(tpl->static_data.body, node->pos_begin)
                    );
                    zend_hash_move_forward_ex(Z_ARRVAL_PP(ctx_iterations), NULL);
                    continue;
                }

                blitz_exec_nodes(tpl, node->first_child, id,
                    result,result_len,result_alloc_len,
                    node->pos_begin_shift,
                    node->pos_end_shift,
                    *ctx_data TSRMLS_CC
                );
                BLITZ_LOOP_INCREMENT(tpl);
                zend_hash_move_forward_ex(Z_ARRVAL_PP(ctx_iterations), NULL);
            }
        } else {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "INTERNAL ERROR: non existant hash key");
        }
    }
}
/* }}} */

/* {{{ int blitz_exec_if_context() */
static void blitz_exec_if_context(blitz_tpl *tpl, unsigned long node_id, zval *parent_params, zval *id,
    char **result, unsigned long *result_len, unsigned long *result_alloc_len, unsigned long *jump TSRMLS_DC)
{
    char *key = NULL;
    unsigned int key_len = 0;
    unsigned long key_index = 0;
    int check_key = 0, condition = 0, not_empty = 0;
    zval **ctx_iterations = NULL;
    zval **ctx_data = NULL;
    call_arg *arg = NULL;
    zval **z = NULL;
    int predefined = -1;
    int use_scope = 0;
    blitz_node *node = NULL, *subnodes = NULL;
    unsigned int i = 0, i_jump = 0, n_subnodes = 0, n_nodes = 0, pos_end = 0;

    node = tpl->static_data.nodes + node_id;
    i = node_id;
    n_nodes = tpl->static_data.n_nodes;

    // find node to execute
    node = tpl->static_data.nodes + i;

    if (BLITZ_DEBUG)
        php_printf("*** FUNCTION *** blitz_exec_if_context\n");

    while (node) {
        if (node->n_args) {
            arg = node->args;
            BLITZ_GET_PREDEFINED_VAR(tpl, arg->name, arg->len, predefined);
            if (predefined>0) {
                not_empty = 1;
            } else if (arg->type == BLITZ_ARG_TYPE_VAR) {
                if (parent_params) {
                    BLITZ_ARG_NOT_EMPTY(*arg,Z_ARRVAL_P(parent_params), not_empty);
                } else {
                    not_empty = -1;
                }

                if (not_empty == -1) {
                    BLITZ_ARG_NOT_EMPTY(*arg,tpl->hash_globals, not_empty);
                    if (not_empty == -1) {
                        use_scope = BLITZ_G(scope_lookup_limit) && tpl->scope_stack_pos;
                        if (use_scope && blitz_scope_stack_find(tpl, arg->name, arg->len + 1, &z TSRMLS_CC)) {
                            BLITZ_ZVAL_NOT_EMPTY(z, not_empty);
                        }
                    }
                }
            } else if (arg->type == BLITZ_ARG_TYPE_VAR_PATH) {
                if (blitz_fetch_var_by_path(&z, arg->name, arg->len, parent_params, tpl TSRMLS_CC)) {
                    BLITZ_ZVAL_NOT_EMPTY(z, not_empty);
                }
            } else if (arg->type == BLITZ_ARG_TYPE_BOOL) {
                not_empty = (arg->name[0] == 't');
            }

            if (not_empty == -1) // generally -1 means 'unknown', but it's equal to 'empty' here
                not_empty = 0;
        }

        if (node->type == BLITZ_NODE_TYPE_ELSE_CONTEXT) {
            condition = 1;
        } else {
            if (node->type == BLITZ_NODE_TYPE_UNLESS_CONTEXT) {
                condition = not_empty ? 0 : 1;
            } else {
                condition = not_empty ? 1 : 0;
            }
        }

        if (BLITZ_DEBUG) {
            php_printf("checking context %s (type=%u)\n", node->lexem, node->type);
            php_printf("condition = %u, not_empty = %u\n", condition, not_empty);
        }

        if (!condition) { // if condition is false - move to the next node in this chain
            pos_end = node->pos_end;
            i_jump = 0;
            while (node = node->next) {
                if (node->pos_begin >= pos_end || node->pos_begin_shift >= pos_end) { /* vars have pos_begin_shift = 0 */
                    pos_end = node->pos_end;
                    if (node->type == BLITZ_NODE_TYPE_ELSEIF_CONTEXT || node->type == BLITZ_NODE_TYPE_ELSE_CONTEXT) {
                        // found next node in the condition chain
                        break;
                    } else {
                        // moved out of the chain
                        *jump = i_jump;
                        return;
                    }
                }
                i_jump++;
            }
        } else {
            // run nodes over parent_params
            blitz_exec_nodes(tpl, node->first_child, id,
                result, result_len, result_alloc_len,
                node->pos_begin_shift, node->pos_end_shift,
                parent_params TSRMLS_CC
            );

            // find the end of this chain
            pos_end = node->pos_end;
            while (node = node->next) {
                if (node->pos_begin >= pos_end || node->pos_begin_shift >= pos_end) {
                    pos_end = node->pos_end;
                    if (node->type != BLITZ_NODE_TYPE_ELSEIF_CONTEXT || node->type != BLITZ_NODE_TYPE_ELSE_CONTEXT) {
                        break;
                    }
                }
                i_jump++;
            }
            *jump = i_jump;
            return;
        }
    }
}
/* }}} */

/* {{{ int blitz_exec_nodes() */
static int blitz_exec_nodes(blitz_tpl *tpl, blitz_node *first_child,
    zval *id, char **result, unsigned long *result_len, unsigned long *result_alloc_len,
    unsigned long parent_begin, unsigned long parent_end, zval *parent_ctx_data TSRMLS_DC)
{
    char *p_result = NULL;
    unsigned long buf_len = 0, new_len = 0;
    unsigned long shift = 0, last_close = 0, current_open = 0, i = 0, n_jump = 0;
    zval *parent_params = NULL;
    blitz_node *node = NULL;

    /* check parent data (once in the beginning) - user could put non-array here.  */
    /* if we use hash_find on non-array - we get segfaults. */
    if (parent_ctx_data && Z_TYPE_P(parent_ctx_data) == IS_ARRAY) {
        parent_params = parent_ctx_data;
    }

    p_result = *result;
    shift = 0;
    last_close = parent_begin;

    if (BLITZ_DEBUG) {
        php_printf("*** FUNCTION *** blitz_exec_nodes\n");
        if (parent_ctx_data) php_var_dump(&parent_ctx_data, 0 TSRMLS_CC);
        if (parent_params) php_var_dump(&parent_params, 0 TSRMLS_CC);
    }

    if (BLITZ_G(scope_lookup_limit)) BLITZ_SCOPE_STACK_PUSH(tpl, parent_params);

    node = first_child;
    while (node) {
        if (BLITZ_DEBUG)
            php_printf("[EXEC] node:%s, pos = %ld, lc = %ld, next = %p\n", node->lexem, node->pos_begin, last_close, node->next);

        current_open = node->pos_begin;
        n_jump = 0;

        /* between nodes */
        if (current_open > last_close) {
            if (BLITZ_DEBUG) php_printf("copy part netween nodes [%u,%u]\n", last_close, current_open);
            buf_len = current_open - last_close;
            BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,p_result);
            p_result = (char*)memcpy(p_result, tpl->static_data.body + last_close, buf_len); 
            *result_len += buf_len;
            p_result+=*result_len;
            (*result)[*result_len] = '\0';
        }

        if (node->lexem && !node->hidden) {
            if (node->type == BLITZ_NODE_TYPE_VAR) { 
                blitz_exec_var(tpl, node->lexem, parent_params, result, result_len, result_alloc_len TSRMLS_CC);
            } else if (node->type == BLITZ_NODE_TYPE_VAR_PATH) {
                blitz_exec_var_path(tpl, node->lexem, parent_params, result, result_len, result_alloc_len TSRMLS_CC);
            } else if (BLITZ_IS_METHOD(node->type)) {
                if (node->type == BLITZ_NODE_TYPE_CONTEXT) {
                    BLITZ_LOOP_MOVE_FORWARD(tpl);
                    blitz_exec_context(tpl, node, parent_params, id, result, result_len, result_alloc_len TSRMLS_CC);
                    BLITZ_LOOP_MOVE_BACK(tpl);
                } else if (node->type == BLITZ_NODE_TYPE_IF_CONTEXT || node->type == BLITZ_NODE_TYPE_UNLESS_CONTEXT) {
                    blitz_exec_if_context(tpl, node->pos_in_list, parent_params, id,
                        result, result_len, result_alloc_len, &n_jump TSRMLS_CC);
                } else {
                    zval *iteration_params = parent_params ? parent_params : NULL;
                    if (BLITZ_IS_PREDEF_METHOD(node->type)) {
                        blitz_exec_predefined_method(
                            tpl, node, iteration_params, id,
                            result, &p_result, result_len, result_alloc_len, tpl->tmp_buf TSRMLS_CC
                        );
                    } else {
                        blitz_exec_user_method(
                            tpl, node, &iteration_params, id,
                            result, &p_result, result_len, result_alloc_len TSRMLS_CC
                        );
                    }
                }
            }
        } 

        last_close = node->pos_end;
        if (n_jump) {
            if (BLITZ_DEBUG) php_printf("JUMP FROM: %s, n_jump = %u\n", node->lexem, n_jump);
            i = 0;
            while(i++ < n_jump) {
                node = node->next;
            }
        } else {
            node = node->next;
        }
    }

    if (BLITZ_G(scope_lookup_limit)) BLITZ_SCOPE_STACK_SHIFT(tpl);

    if (BLITZ_DEBUG)
        php_printf("== D:b3  %ld,%ld,%ld\n",last_close,parent_begin,parent_end);

    if (parent_end>last_close) {
        buf_len = parent_end - last_close;
        BLITZ_REALLOC_RESULT(buf_len,new_len,*result_len,*result_alloc_len,*result,p_result);
        p_result = (char*)memcpy(p_result, tpl->static_data.body + last_close, buf_len);
        *result_len += buf_len;
        p_result+=*result_len;
        (*result)[*result_len] = '\0';
    }

    if (BLITZ_DEBUG)
        php_printf("== END NODES\n");

    return 1;
}
/* }}} */

static inline int blitz_populate_root (blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{
    zval *empty_array;

    if (BLITZ_DEBUG) php_printf("will populate the root iteration\n");

    MAKE_STD_ZVAL(empty_array);
    array_init(empty_array);
    add_next_index_zval(tpl->iterations, empty_array);

    return 1;
}
/* }}} */

static int blitz_exec_template(blitz_tpl *tpl, zval *id, char **result, unsigned long *result_len TSRMLS_DC) /* {{{ */
{
    unsigned long result_alloc_len = 0;

    /* quick return if there was no nodes */
    if (0 == tpl->static_data.n_nodes) {
        *result = tpl->static_data.body; /* won't copy */
        *result_len += tpl->static_data.body_len;
        return 2; /* will not call efree on result */
    }

    /* build result, initial alloc of twice bigger than body */
    *result_len = 0;
    result_alloc_len = 2*tpl->static_data.body_len; 
    *result = (char*)ecalloc(result_alloc_len, sizeof(char));

    if (0 == zend_hash_num_elements(Z_ARRVAL_P(tpl->iterations))) {
        blitz_populate_root(tpl TSRMLS_CC);
    }

    if (tpl->iterations) {
        zval **iteration_data = NULL;
        char *key;
        unsigned int key_len;
        unsigned long key_index;

        /* if it's an array of numbers - treat this as single iteration and pass as a parameter */
        /* otherwise walk and iterate all the array elements */
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(tpl->iterations));
        if (HASH_KEY_IS_LONG != zend_hash_get_current_key_ex(Z_ARRVAL_P(tpl->iterations), &key, &key_len, &key_index, 0, NULL)) {
            blitz_exec_nodes(tpl, tpl->static_data.nodes, id, result, result_len, &result_alloc_len, 0,
                tpl->static_data.body_len, tpl->iterations TSRMLS_CC);
        } else {
            BLITZ_LOOP_INIT(tpl, zend_hash_num_elements(Z_ARRVAL_P(tpl->iterations)));
            while (zend_hash_get_current_data(Z_ARRVAL_P(tpl->iterations), (void**) &iteration_data) == SUCCESS) {
                if (
                    HASH_KEY_IS_LONG != zend_hash_get_current_key_ex(Z_ARRVAL_P(tpl->iterations), &key, &key_len, &key_index, 0, NULL)
                    || IS_ARRAY != Z_TYPE_PP(iteration_data)) 
                {
                   zend_hash_move_forward(Z_ARRVAL_P(tpl->iterations));
                   continue;
                }
                blitz_exec_nodes(tpl, tpl->static_data.nodes, id, 
                    result, result_len, &result_alloc_len, 0, tpl->static_data.body_len, *iteration_data TSRMLS_CC);
                BLITZ_LOOP_INCREMENT(tpl);
                zend_hash_move_forward(Z_ARRVAL_P(tpl->iterations));
            }
        }
    } 

    return 1;
}
/* }}} */

static inline int blitz_normalize_path(char **dest, const char *local, int local_len, char *global, int global_len TSRMLS_DC) /* {{{ */
{
    int buf_len = 0;
    char *buf = *dest;
    register char  *p = NULL, *q = NULL;

    if (local_len && local[0] == '/') {
        if (local_len+1>BLITZ_CONTEXT_PATH_MAX_LEN) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "context path %s is too long (limit %d)",local,BLITZ_CONTEXT_PATH_MAX_LEN);
            return 0;
        }
        memcpy(buf, local, local_len + 1);
        buf_len = local_len;
    } else {
        if (local_len + global_len + 2 > BLITZ_CONTEXT_PATH_MAX_LEN) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "context path %s is too long (limit %d)",local,BLITZ_CONTEXT_PATH_MAX_LEN);
            return 0;
        }

        memcpy(buf, global, global_len);
        buf[global_len] = '/';
        buf_len = 1 + global_len;
        if (local) {
            memcpy(buf + 1 + global_len, local, local_len + 1);
            buf_len += local_len;
        }
    }

    buf[buf_len] = '\x0';
    while ((p = strstr(buf, "//"))) {
        for(q = p+1; *q; q++) *(q-1) = *q;
        *(q-1) = 0;
        --buf_len;
    }

    /* check for `..` in the path */
    /* first, remove path elements to the left of `..` */
    for(p = buf; p <= (buf+buf_len-3); p++) {
        if (memcmp(p, "/..", 3) != 0 || (*(p+3) != '/' && *(p+3) != 0)) continue;
        for(q = p-1; q >= buf && *q != '/'; q--, buf_len--);
        --buf_len;
        if (*q == '/') {
            p += 3;
            while (*p) *(q++) = *(p++);
            *q = 0;
            buf_len -= 3;
            p = buf;
        }
    }

    /* second, clear all `..` in the begining of the path
       because `/../` = `/` */
    while (buf_len > 2 && memcmp(buf, "/..", 3) == 0) {
        for(p = buf+3; *p; p++) *(p-3) = *p;
        *(p-3) = 0;
        buf_len -= 3;
    }

    /* clear `/` at the end of the path */
    while (buf_len > 1 && buf[buf_len-1] == '/') buf[--buf_len] = 0;
    if (!buf_len) { memcpy(buf, "/", 2); buf_len = 1; }

    buf[buf_len] = '\x0';

    return 1;
}
/* }}} */

static inline int blitz_iterate_by_path(blitz_tpl *tpl, const char *path, int path_len,
    int is_current_iteration, int create_new TSRMLS_DC) /* {{{ */
{
    zval **tmp;
    int i = 1, ilast = 1, j = 0, k = 0;
    const char *p = path;
    int pmax = path_len;
    char key[BLITZ_CONTEXT_PATH_MAX_LEN];
    int key_len  = 0;
    int found = 1; /* create new iteration (add new item to parent list) */
    int is_root = 0;

    k = pmax - 1;
    tmp = &tpl->iterations;

    if (BLITZ_DEBUG) {
        php_printf("[debug] BLITZ_FUNCTION: blitz_iterate_by_path, path=%s\n", path);
        php_printf("%p %p %p\n", tpl->current_iteration, tpl->last_iteration, tpl->iterations);
        if (tpl->current_iteration) php_var_dump(tpl->current_iteration,1 TSRMLS_CC);
    }

    /* check root */
    if (*p == '/' && pmax == 1) {
        is_root = 1;
    }

    if ((0 == zend_hash_num_elements(Z_ARRVAL_PP(tmp)) || (is_root && create_new))) {
        blitz_populate_root(tpl TSRMLS_CC);
    }

    /* iterate root  */
    if (is_root) {
        zend_hash_internal_pointer_end(Z_ARRVAL_PP(tmp));
        if (SUCCESS == zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &tpl->last_iteration)) {
            if (is_current_iteration) {
                /*zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &tpl->current_iteration); */
                tpl->current_iteration = tpl->last_iteration;
                tpl->current_iteration_parent = & tpl->iterations;
            } 
            if (BLITZ_DEBUG) {
                php_printf("last iteration becomes:\n");
                if (tpl->last_iteration) {
                    php_var_dump(tpl->last_iteration,1 TSRMLS_CC);
                } else {
                    php_printf("empty\n");
                }
            }
        } else {
            return 0;
        }
        return 1;
    }

    *p++;
    while (i<pmax) {
        if (*p == '/' || i == k) {
            j = i - ilast;
            key_len = j + (i == k ? 1 : 0);
            memcpy(key, p-j, key_len);
            key[key_len] = '\x0';

            if (BLITZ_DEBUG) php_printf("[debug] going move to:%s\n",key);

            zend_hash_internal_pointer_end(Z_ARRVAL_PP(tmp));
            if (SUCCESS != zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &tmp)) {
                zval *empty_array;
                if (BLITZ_DEBUG) php_printf("[debug] current_data not found, will populate the list \n");
                MAKE_STD_ZVAL(empty_array);
                array_init(empty_array);
                add_next_index_zval(*tmp,empty_array);
                if (SUCCESS != zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &tmp)) {
                    return 0;
                }
                if (BLITZ_DEBUG) {
                    php_printf("[debug] tmp becomes:\n");
                    php_var_dump(tmp,0 TSRMLS_CC);
                }
            } else {
                if (BLITZ_DEBUG) {
                    php_printf("[debug] tmp dump (node):\n");
                    php_var_dump(tmp,0 TSRMLS_CC);
                }
            }

            if (Z_TYPE_PP(tmp) != IS_ARRAY) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
                    "OPERATION ERROR: unable to iterate context \"%s\" in \"%s\" "
                    "because parent iteration was not set as array of arrays before. "
                    "Correct your previous iteration sets.", key, path);
                return 0;
            }

            if (SUCCESS != zend_hash_find(Z_ARRVAL_PP(tmp),key,key_len+1,(void **)&tmp)) {
                zval *empty_array;
                zval *init_array;

                if (BLITZ_DEBUG) php_printf("[debug] key \"%s\" was not found, will populate the list \n",key);
                found = 0;

                /* create */
                MAKE_STD_ZVAL(empty_array);
                array_init(empty_array);

                MAKE_STD_ZVAL(init_array);
                array_init(init_array);
                /* [] = array(0 => array()) */
                if (BLITZ_DEBUG) {
                    php_printf("D-1: %p %p\n", tpl->current_iteration, tpl->last_iteration);
                    if (tpl->current_iteration) php_var_dump(tpl->current_iteration,1 TSRMLS_CC);
                }

                add_assoc_zval_ex(*tmp, key, key_len+1, init_array);

                if (BLITZ_DEBUG) {
                    php_printf("D-2: %p %p\n", tpl->current_iteration, tpl->last_iteration);
                   if (tpl->current_iteration) php_var_dump(tpl->current_iteration,1 TSRMLS_CC);
                }

                add_next_index_zval(init_array,empty_array);
                zend_hash_internal_pointer_end(Z_ARRVAL_PP(tmp));

                if (BLITZ_DEBUG) {
                    php_var_dump(tmp,0 TSRMLS_CC);
                }

                /* 2DO: getting tmp and current_iteration_parent can be done by 1 call of zend_hash_get_current_data */
                if (is_current_iteration) {
                    if (SUCCESS != zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &tpl->current_iteration_parent)) {
                        return 0;
                    }
                }

                if (SUCCESS != zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &tmp)) {
                    return 0;
                }
            }

            if (Z_TYPE_PP(tmp) != IS_ARRAY) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, 
                    "OPERATION ERROR: unable to iterate context \"%s\" in \"%s\" "
                    "because it was set as \"scalar\" value before. "
                    "Correct your previous iteration sets.", key, path);
                return 0;
            }

            ilast = i + 1;
            if (BLITZ_DEBUG) {
                php_printf("[debug] tmp dump (item \"%s\"):\n",key);
                php_var_dump(tmp,0 TSRMLS_CC);
            }
        }
        ++p;
        ++i;
    }

    /* logic notes: 
        - new iteration can be created while propagating through the path - then created 
          inside upper loop and found set to 0.
        - new iteration can be created if not found while propagating through the path and 
          called from block or iterate - then create_new=1 is used
        - in most cases new iteration will not be created if called from set, not found while 
          propagating through the path - then create_new=0 is used.
          but when we used fetch and then set to the same context - it is cleaned, 
          and there are no iterations at all.
          so in this particular case we do create an empty iteration.
    */

    if (found && (create_new || 0 == zend_hash_num_elements(Z_ARRVAL_PP(tmp)))) {
        zval *empty_array;
        MAKE_STD_ZVAL(empty_array);
        array_init(empty_array);

        add_next_index_zval(*tmp, empty_array);
        zend_hash_internal_pointer_end(Z_ARRVAL_PP(tmp));

        if (BLITZ_DEBUG) {
            php_printf("[debug] found, will add new iteration\n");
            php_var_dump(tmp,0 TSRMLS_CC);
        }
    }

    if (SUCCESS != zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &tpl->last_iteration)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "INTERNAL ERROR: unable fetch last_iteration in blitz_iterate_by_path");
        tpl->last_iteration = NULL;
    }

    if (is_current_iteration) {
        tpl->current_iteration = tpl->last_iteration; /* was: fetch from tmp */
    }

    if (BLITZ_DEBUG) {
        php_printf("Iteration pointers: %p %p %p\n", tpl->current_iteration, tpl->current_iteration_parent, tpl->last_iteration);
        tpl->current_iteration ? php_var_dump(tpl->current_iteration,1 TSRMLS_CC) : php_printf("current_iteration is empty\n");
        tpl->current_iteration_parent ? php_var_dump(tpl->current_iteration_parent,1 TSRMLS_CC) : php_printf("current_iteration_parent is empty\n");
        tpl->last_iteration ? php_var_dump(tpl->last_iteration,1 TSRMLS_CC) : php_printf("last_iteration is empty\n");
    }

    return 1;
}
/* }}} */

static int blitz_find_iteration_by_path(blitz_tpl *tpl, const char *path, int path_len,
    zval **iteration, zval **iteration_parent TSRMLS_DC) /* {{{ */
{
    zval **tmp, **entry;
    register int i = 1;
    int ilast = 1, j = 0, k = 0, key_len = 0;
    register const char *p = path;
    register int pmax = path_len;
    char buffer[BLITZ_CONTEXT_PATH_MAX_LEN];
    char *key = NULL;
    zval **dummy;
    unsigned long index = 0;

    k = pmax - 1;
    tmp = & tpl->iterations;
    key = buffer;

    /* check root */
    if (BLITZ_DEBUG) php_printf("D:-0 %s(%d)\n", path, path_len);

    if (*p == '/' && pmax == 1) {
        if (0 == zend_hash_num_elements(Z_ARRVAL_PP(tmp))) {
            blitz_populate_root(tpl TSRMLS_CC);
        }
        *iteration = NULL;
        zend_hash_internal_pointer_end(Z_ARRVAL_PP(tmp));
        if (SUCCESS != zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &entry)) {
            return 0;
        }
        *iteration = *entry;
        *iteration_parent = tpl->iterations;
        return 1;
    }

    if (i>=pmax) {
        return 0;
    }

    *p++;
    if (BLITZ_DEBUG) php_printf("%d/%d\n", i, pmax);
    while (i<pmax) {
        if (BLITZ_DEBUG) php_printf("%d/%d\n", i, pmax);
        if (*p == '/' || i == k) {
            j = i - ilast;
            key_len = j + (i == k ? 1 : 0);
            memcpy(key, p-j, key_len);
            key[key_len] = '\x0';
            if (BLITZ_DEBUG) php_printf("key:%s\n", key);

            /* check the last key: if numerical - move to the end of array. otherwise search in this array. */
            /* this logic handles two iteration formats:  */
            /* 'parent' => [0 => {'child' => {'var' => 'val'}] (parent iterations is array with numerical indexes) */
            /* and 'block' => {'child' => {'var' => 'val'}} (just an assosiative array) */
            zend_hash_internal_pointer_end(Z_ARRVAL_PP(tmp));
            if (zend_hash_get_current_key(Z_ARRVAL_PP(tmp), &key, &index, 0) == HASH_KEY_IS_LONG) {
                if (SUCCESS != zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &entry)) {
                    return 0;
                }
                if (BLITZ_DEBUG) {
                    php_printf("moving to the last array element:\n");
                    php_var_dump(entry, 0 TSRMLS_CC);
                }
                if (SUCCESS != zend_hash_find(Z_ARRVAL_PP(entry),key,key_len+1,(void **) &tmp)) {
                    return 0;
                }
            } else {
                if (SUCCESS != zend_hash_find(Z_ARRVAL_PP(tmp),key,key_len+1,(void **) &tmp)) {
                    return 0;
                }
            }

            if (BLITZ_DEBUG) {
                php_printf("key %s: we are here:\n", key);
                php_var_dump(tmp, 0 TSRMLS_CC);
            }

            ilast = i + 1;
        }
        ++p;
        ++i;
    }

    /* can be not an array (tried to iterate scalar) */
    if (IS_ARRAY != Z_TYPE_PP(tmp)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "ERROR: unable to find context '%s', "
            "it was set as \"scalar\" value - check your iteration params", path); 
        return 0;
    }

    zend_hash_internal_pointer_end(Z_ARRVAL_PP(tmp));

    /* if not array - stop searching, otherwise get the latest data */
    if (zend_hash_get_current_key(Z_ARRVAL_PP(tmp), &key, &index, 0) == HASH_KEY_IS_STRING) {
        *iteration = *tmp;
        *iteration_parent = *tmp;
        return 1;
    }

    if (SUCCESS == zend_hash_get_current_data(Z_ARRVAL_PP(tmp), (void **) &dummy)) {
        if (BLITZ_DEBUG) php_printf("%p %p %p %p\n", dummy, *dummy, iteration, *iteration);
        *iteration = *dummy;
        *iteration_parent = *tmp;
    } else {
        return 0;
    }

    if (BLITZ_DEBUG) {
        php_printf("%p %p %p %p\n", dummy, *dummy, iteration, *iteration);
        php_printf("parent:\n");
        php_var_dump(iteration_parent,0 TSRMLS_CC);
        php_printf("found:\n");
        php_var_dump(iteration,0 TSRMLS_CC);
    }

    return 1;
}
/* }}} */

static void blitz_build_fetch_index_node(blitz_tpl *tpl, blitz_node *node, const char *path, unsigned int path_len) /* {{{ */
{
    unsigned long j = 0;
    unsigned int current_path_len = 0;
    char current_path[BLITZ_MAX_FETCH_INDEX_KEY_SIZE] = "";
    char *lexem = NULL;
    unsigned int lexem_len = 0;
    zval *temp = NULL;
    unsigned int skip_node = 0;

    if (!node) 
        return;
    
    if (path_len > 0) {
        memcpy(current_path, path, path_len);
        current_path_len = path_len;
    }

    if (node->type == BLITZ_NODE_TYPE_CONTEXT) {
        lexem = node->args[0].name;
        lexem_len = node->args[0].len;
    } else if (BLITZ_IS_VAR(node->type)) {
        lexem = node->lexem;
        lexem_len = node->lexem_len;
    } else if (
        (node->type == BLITZ_NODE_TYPE_IF_CONTEXT) || (node->type == BLITZ_NODE_TYPE_UNLESS_CONTEXT) || 
        (node->type == BLITZ_NODE_TYPE_ELSE_CONTEXT) || (node->type == BLITZ_NODE_TYPE_ELSEIF_CONTEXT))
    {
        skip_node = 1;
    } else {
        return;
    }

    if (0 == skip_node) {
        memcpy(current_path + path_len, "/", 1);
        memcpy(current_path + path_len + 1, lexem, lexem_len);
        current_path_len = strlen(current_path);
        current_path[current_path_len] = '\x0';

        if (BLITZ_DEBUG) php_printf("building index for fetch_index, lexem = %s, path=%s\n", lexem, current_path);

        MAKE_STD_ZVAL(temp);
        ZVAL_LONG(temp, node->pos_in_list);
        zend_hash_update(tpl->static_data.fetch_index, current_path, current_path_len+1, &temp, sizeof(zval *), NULL);
    }

    if (node->first_child) {
        blitz_node *child = node->first_child;
        while (child) {
            blitz_build_fetch_index_node(tpl, child, current_path, current_path_len);
            child = child->next;
        }
    }
}
/* }}} */

static int blitz_build_fetch_index(blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{
    unsigned long i = 0, last_close = 0;
    char path[BLITZ_MAX_FETCH_INDEX_KEY_SIZE] = "";
    unsigned int path_len = 0;
    blitz_node *i_node = NULL;

    ALLOC_HASHTABLE(tpl->static_data.fetch_index);
    zend_hash_init(tpl->static_data.fetch_index, 8, NULL, ZVAL_PTR_DTOR, 0);

    i_node = tpl->static_data.nodes;
    while (i_node) {
        blitz_build_fetch_index_node(tpl, i_node, path, path_len);
        i_node = i_node->next;
    }

    if (BLITZ_DEBUG) {
        zval *elem = NULL;
        char *key = NULL;
        int key_len = 0;
        long index = 0;
        HashTable *ht = tpl->static_data.fetch_index;

        zend_hash_internal_pointer_reset(ht);
        php_printf("fetch_index dump:\n");

        while (zend_hash_get_current_data(ht, (void**) &elem) == SUCCESS) {
            if (zend_hash_get_current_key_ex(ht, &key, &key_len, &index, 0, NULL) != HASH_KEY_IS_STRING) {
                zend_hash_move_forward(ht);
                continue;
            }
            php_printf("key: %s(%u)\n", key, key_len);
            php_var_dump((zval**)elem, 0 TSRMLS_CC);
            zend_hash_move_forward(ht);
        }
    }

    return 1;
}
/* }}} */

static inline int blitz_touch_fetch_index(blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{
    if (!(tpl->flags & BLITZ_FLAG_FETCH_INDEX_BUILT)) {
        if (blitz_build_fetch_index(tpl TSRMLS_CC)) {
            tpl->flags |= BLITZ_FLAG_FETCH_INDEX_BUILT;
        } else { 
            return 0;
        } 
    }
    return 1;
}
/* }}} */

/* {{{ int blitz_fetch_node_by_path() */
static int blitz_fetch_node_by_path(blitz_tpl *tpl, zval *id, const char *path, unsigned int path_len,
    zval *input_params, char **result, unsigned long *result_len TSRMLS_DC)
{
    blitz_node *i_node = NULL;
    unsigned long result_alloc_len = 0;
    zval **z = NULL;

    if ((path[0] == '/') && (path_len == 1)) {
        return blitz_exec_template(tpl,id,result,result_len TSRMLS_CC);
    }

    if (!blitz_touch_fetch_index(tpl TSRMLS_CC)) {
        return 0;
    }

    /* find node by path   */
    if (SUCCESS == zend_hash_find(tpl->static_data.fetch_index, (char *)path, path_len + 1, (void**)&z)) {
        i_node = tpl->static_data.nodes + Z_LVAL_PP(z);
    } else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot find context %s in template %s", path, tpl->static_data.name);
        return 0;
    }

    /* fetch result */
    if (i_node->first_child) {
        result_alloc_len = 2*(i_node->pos_end_shift - i_node->pos_begin_shift);
        *result = (char*)ecalloc(result_alloc_len,sizeof(char));

        return blitz_exec_nodes(tpl, i_node->first_child, id,
            result, result_len, &result_alloc_len,
            i_node->pos_begin_shift, i_node->pos_end_shift,
            input_params TSRMLS_CC
        );
    } else {
        unsigned long rlen = i_node->pos_end_shift - i_node->pos_begin_shift;
        *result_len = rlen;
        *result = (char*)emalloc(rlen+1);
        if (!*result)
            return 0;
        if (!memcpy(*result, tpl->static_data.body + i_node->pos_begin_shift, rlen)) {
            return 0;
        }
        *(*result + rlen) = '\x0';
    }

    return 1;
}
/* }}} */

static inline int blitz_iterate_current(blitz_tpl *tpl TSRMLS_DC) /* {{{ */
{

    if (BLITZ_DEBUG) php_printf("[debug] BLITZ_FUNCTION: blitz_iterate_current, path=%s\n", tpl->current_path);

    blitz_iterate_by_path(tpl, tpl->current_path, strlen(tpl->current_path), 1, 1 TSRMLS_CC);
    tpl->last_iteration = tpl->current_iteration;

    return 1;
}
/* }}} */

static inline int blitz_prepare_iteration(blitz_tpl *tpl, const char *path, int path_len, int iterate_nonexistant TSRMLS_DC) /* {{{ */
{
    int res = 0;

    if (BLITZ_DEBUG) php_printf("[debug] BLITZ_FUNCTION: blitz_prepare_iteration, path=%s\n", path);

    if (path_len == 0) {
        return blitz_iterate_current(tpl TSRMLS_CC);
    } else {
        int current_len = strlen(tpl->current_path);
        int norm_len = 0;
        res = blitz_normalize_path(&tpl->tmp_buf, path, path_len, tpl->current_path, current_len TSRMLS_CC);

        if (!res) return 0;
        norm_len = strlen(tpl->tmp_buf);

        /* check if path exists */
        if (norm_len>1) {
            zval *dummy = NULL;
            if (!blitz_touch_fetch_index(tpl TSRMLS_CC)) {
                return 0;
            }

            if ((0 == iterate_nonexistant) 
                && SUCCESS != zend_hash_find(tpl->static_data.fetch_index, tpl->tmp_buf, norm_len + 1, (void**)&dummy))
            {
                return -1;
            }
        }

        if (tpl->current_iteration_parent
            && (current_len == norm_len)
            && (0 == strncmp(tpl->tmp_buf,tpl->current_path,norm_len)))
        {
            return blitz_iterate_current(tpl TSRMLS_CC);
        } else {
            return blitz_iterate_by_path(tpl, tpl->tmp_buf, norm_len, 0, 1 TSRMLS_CC);
        }
    }

    return 0;
}
/* }}} */

static inline int blitz_merge_iterations_by_str_keys(zval **target, zval *input TSRMLS_DC) /* {{{ */
{ 
    zval **elem;
    HashTable *input_ht = NULL;
    char *key = NULL;
    unsigned int key_len = 0;
    unsigned long index = 0;

    if (!input || (IS_ARRAY != Z_TYPE_P(input)) || (IS_ARRAY != Z_TYPE_PP(target))) {
        return 0;
    }

    if (0 == zend_hash_num_elements(Z_ARRVAL_P(input))) {
        return 1;
    }

    input_ht = Z_ARRVAL_P(input);
    while (zend_hash_get_current_data(input_ht, (void**) &elem) == SUCCESS) {
        if (zend_hash_get_current_key_ex(input_ht, &key, &key_len, &index, 0, NULL) != HASH_KEY_IS_STRING) {
            zend_hash_move_forward(input_ht);
            continue;
        }

        if (key && key_len) {
            Z_ADDREF_PP(elem);
            zend_hash_update(Z_ARRVAL_PP(target), key, key_len, elem, sizeof(zval *), NULL);
        }
        zend_hash_move_forward(input_ht);
    }

    return 1;
}
/* }}} */

static inline int blitz_merge_iterations_by_num_keys(zval **target, zval *input TSRMLS_DC) /* {{{ */
{
    zval **elem;
    HashTable *input_ht = NULL;
    char *key = NULL;
    unsigned int key_len = 0;
    unsigned long index = 0;

    if (!input || (IS_ARRAY != Z_TYPE_P(input))) {
        return 0;
    }

    if (0 == zend_hash_num_elements(Z_ARRVAL_P(input))) {
        return 1;
    }

    input_ht = Z_ARRVAL_P(input);
    while (zend_hash_get_current_data(input_ht, (void**) &elem) == SUCCESS) {
        if (zend_hash_get_current_key_ex(input_ht, &key, &key_len, &index, 0, NULL) != HASH_KEY_IS_LONG) {
            zend_hash_move_forward(input_ht);
            continue;
        }

        Z_ADDREF_PP(elem);
        zend_hash_index_update(Z_ARRVAL_PP(target), index, elem, sizeof(zval *), NULL);
        zend_hash_move_forward(input_ht);
    }

    return 1;
}
/* }}} */

static inline int blitz_merge_iterations_set(blitz_tpl *tpl, zval *input_arr TSRMLS_DC) /* {{{ */
{
    HashTable *input_ht = NULL;
    char *key = NULL;
    unsigned int key_len = 0;
    unsigned long index = 0;
    int res = 0, is_current_iteration = 0, first_key_type = 0;
    zval **target_iteration;

    if (0 == zend_hash_num_elements(Z_ARRVAL_P(input_arr))) {
        return 0;
    }

    /* *** DATA STRUCTURE FORMAT *** */
    /* set works differently for numerical keys and string keys: */
    /*     (1) STRING: set(array('a' => 'a_val')) will update current_iteration keys */
    /*     (2) LONG: set(array(0=>array('a'=>'a_val'))) will reset current_iteration_parent */
    input_ht = Z_ARRVAL_P(input_arr);
    zend_hash_internal_pointer_reset(input_ht);
    first_key_type = zend_hash_get_current_key_ex(input_ht, &key, &key_len, &index, 0, NULL);

    /* *** FIXME *** */
    /* blitz_iterate_by_path here should have is_current_iteration = 1 ALWAYS. */
    /* BUT for some reasons this broke tests. Until the bug is found for the backward compatibility */
    /* is_current_iteration is 0 for string key set and 1 for numerical (otherwise current_iteration_parrent */
    /* is not set properly) in blitz_iterate_by_path */

    /*    is_current_iteration = 1; */
    if (HASH_KEY_IS_LONG == first_key_type)
        is_current_iteration = 1;

    if (!tpl->current_iteration) {
        blitz_iterate_by_path(tpl, tpl->current_path, strlen(tpl->current_path), is_current_iteration, 0 TSRMLS_CC);
    } else {
        /* fix last_iteration: if we did iterate('/some') before and now set in '/', */
        /* then current_iteration is not empty but last_iteration points to '/some' */
        tpl->last_iteration = tpl->current_iteration;
    }

    if (IS_ARRAY == Z_TYPE_PP(tpl->last_iteration))
        zend_hash_internal_pointer_reset(Z_ARRVAL_PP(tpl->last_iteration));

    if (HASH_KEY_IS_STRING == first_key_type) {
        target_iteration = tpl->last_iteration;
        res = blitz_merge_iterations_by_str_keys(target_iteration, input_arr TSRMLS_CC);
    } else {
        if (!tpl->current_iteration_parent) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "INTERNAL ERROR: unable to set into current_iteration_parent, is NULL");
            return 0;
        }
        target_iteration = tpl->current_iteration_parent;
        zend_hash_clean(Z_ARRVAL_PP(target_iteration));
        tpl->current_iteration = NULL;  /* parent was cleaned */
        res = blitz_merge_iterations_by_num_keys(target_iteration, input_arr TSRMLS_CC);
    }

    return 1;
}
/* }}} */

/**********************************************************************************************************************
* Blitz CLASS methods
**********************************************************************************************************************/

/* {{{ proto new Blitz([string filename]) */
static PHP_FUNCTION(blitz_init)
{
    blitz_tpl *tpl;
    int filename_len = 0, ret;
    char *filename = NULL;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|s", &filename, &filename_len)) {
        return;
    }

    if (filename_len >= MAXPATHLEN) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Filename exceeds the maximum allowed length of %d characters", MAXPATHLEN);
        RETURN_FALSE;
    }

    if (getThis() && zend_hash_exists(Z_OBJPROP_P(getThis()), "tpl", sizeof("tpl"))) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "ERROR: the object has already been initialized");
        RETURN_FALSE;
    }

    /* initialize template  */
    if (!(tpl = blitz_init_tpl(filename, filename_len, NULL, NULL, NULL TSRMLS_CC))) {
        RETURN_FALSE;
    }

    if (tpl->static_data.body_len) {
        /* analize template  */
        if (!blitz_analize(tpl TSRMLS_CC)) {
            blitz_free_tpl(tpl);
            RETURN_FALSE;
        }
    }

    ret = zend_list_insert(tpl, le_blitz);
    add_property_resource(getThis(), "tpl", ret);
}
/* }}} */

/* {{{ proto bool Blitz->load(string body) */
static PHP_FUNCTION(blitz_load) 
{
    blitz_tpl *tpl;
    char *body;
    int body_len;
    zval *id, **desc;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (BLITZ_CALLED_USER_METHOD(tpl)) RETURN_FALSE;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &body, &body_len)) {
        return;
    }

    if (tpl->static_data.body) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,"INTERNAL ERROR: template is already loaded");
        RETURN_FALSE;
    }

    /* load body */
    if (!blitz_load_body(tpl, body, body_len TSRMLS_CC)) {
        RETURN_FALSE;
    }

    /* analize template */
    if (!blitz_analize(tpl TSRMLS_CC)) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Blitz->dumpStruct(void) */
static PHP_FUNCTION(blitz_dump_struct)
{
    zval *id, **desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    php_blitz_dump_struct(tpl);

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto void Blitz->getStruct(void) */
static PHP_FUNCTION(blitz_get_struct)
{
    zval *id, **desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    array_init(return_value);

    blitz_get_path_list(tpl, return_value, 0, 0);
}
/* }}} */

/* {{{ proto array Blitz->getIterations(void) */
static PHP_FUNCTION(blitz_get_iterations)
{
    zval *id, **desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (tpl->iterations) {
        *return_value = *tpl->iterations; 
        zval_copy_ctor(return_value);
        INIT_PZVAL(return_value);
    } else {
        array_init(return_value);
    }
}
/* }}} */

/* {{{ proto bool Blitz->setGlobals(array values) */
static PHP_FUNCTION(blitz_set_global)
{
    zval *id, **desc, *input_arr, **elem, *temp;
    blitz_tpl *tpl;
    HashTable *input_ht;
    char *key;
    unsigned int key_len;
    unsigned long index;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"a",&input_arr)) {
        return;
    }

    input_ht = Z_ARRVAL_P(input_arr);
    zend_hash_internal_pointer_reset(tpl->hash_globals);
    zend_hash_internal_pointer_reset(input_ht);

    while (zend_hash_get_current_data(input_ht, (void**) &elem) == SUCCESS) {
        if (zend_hash_get_current_key_ex(input_ht, &key, &key_len, &index, 0, NULL) != HASH_KEY_IS_STRING) {
            zend_hash_move_forward(input_ht);
            continue;
        } 

        /* disallow empty keys */
        if (!key_len || !key) {
            zend_hash_move_forward(input_ht);
            continue;
        }

        ALLOC_ZVAL(temp);
        *temp = **elem;
        zval_copy_ctor(temp);
        INIT_PZVAL(temp);

        zend_hash_update(tpl->hash_globals, key, key_len, (void*)&temp, sizeof(zval *), NULL);
        zend_hash_move_forward(input_ht);
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto array Blitz->getGlobals(void) */
static PHP_FUNCTION(blitz_get_globals)
{
    zval *id, **desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    array_init(return_value);
    zend_hash_copy(Z_ARRVAL_P(return_value), tpl->hash_globals, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
}
/* }}} */

/* {{{ proto bool Blitz->hasContext(string context) */
static PHP_FUNCTION(blitz_has_context)
{
    zval *id, **desc;
    blitz_tpl *tpl;
    char *path;
    int path_len, norm_len = 0, current_len = 0;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len)) {
        return;
    }

    if (path_len == 1 && path[0] == '/') {
        RETURN_TRUE;
    }

    current_len = strlen(tpl->current_path);
    if (!blitz_normalize_path(&tpl->tmp_buf, path, path_len, tpl->current_path, current_len TSRMLS_CC)) {
        RETURN_FALSE;
    }
    norm_len = strlen(tpl->tmp_buf);

    if (!blitz_touch_fetch_index(tpl TSRMLS_CC)) {
        RETURN_FALSE;
    }

    /* find node by path */
    if (zend_hash_exists(tpl->static_data.fetch_index, tpl->tmp_buf, norm_len + 1)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
/* }}} */

/* {{{ proto string Blitz->parse([array iterations]) */
static PHP_FUNCTION(blitz_parse)
{
    zval *id, **desc, *input_arr = NULL;
    blitz_tpl *tpl;
    char *result;
    unsigned long result_len = 0;
    int res;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);
    if (BLITZ_CALLED_USER_METHOD(tpl)) RETURN_FALSE;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &input_arr)) {
        return;
    } 

    if (!tpl->static_data.body) { /* body was not loaded */
        RETURN_FALSE;
    }

    if (input_arr && 0 < zend_hash_num_elements(Z_ARRVAL_P(input_arr))) {
        if (!blitz_merge_iterations_set(tpl, input_arr TSRMLS_CC)) {
            RETURN_FALSE;
        }
    } 

    res = blitz_exec_template(tpl, id, &result, &result_len TSRMLS_CC);

    if (res) {
        ZVAL_STRINGL(return_value, result, result_len, 1);
        if (res == 1) efree(result);
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto string Blitz->display([array iterations]) */
static PHP_FUNCTION(blitz_display)
{
    zval *id, **desc, *input_arr = NULL;
    blitz_tpl *tpl;
    char *result;
    unsigned long result_len = 0;
    int res;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);
    if (BLITZ_CALLED_USER_METHOD(tpl)) RETURN_FALSE;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &input_arr)) {
        return;
    }

    if (!tpl->static_data.body) { /* body was not loaded */
        RETURN_FALSE;
    }

    if (input_arr && 0 < zend_hash_num_elements(Z_ARRVAL_P(input_arr))) {
        if (!blitz_merge_iterations_set(tpl, input_arr TSRMLS_CC)) {
            RETURN_FALSE;
        }
    }

    res = blitz_exec_template(tpl, id, &result, &result_len TSRMLS_CC);

    if (res) {
        PHPWRITE(result, result_len);
        if (res == 1) efree(result);
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto string Blitz->context(string path) */
static PHP_FUNCTION(blitz_context)
{
    zval *id, **desc;
    blitz_tpl *tpl;
    char *path;
    int current_len, norm_len = 0, path_len, res;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len)) {
        return;
    }

    current_len = strlen(tpl->current_path);
    RETVAL_STRINGL(tpl->current_path, current_len, 1);

    if (path && path_len == current_len && 0 == strncmp(path, tpl->current_path, path_len)) {
        return;
    }

    res = blitz_normalize_path(&tpl->tmp_buf, path, path_len, tpl->current_path, current_len TSRMLS_CC);
    if (res) {
        norm_len = strlen(tpl->tmp_buf);
    }

    if ((current_len != norm_len) || (0 != strncmp(tpl->tmp_buf,tpl->current_path,norm_len))) {
        tpl->current_iteration = NULL; 
    }
	
    if (tpl->current_path) {
       efree(tpl->current_path);
    }

    tpl->current_path = estrndup((char *)tpl->tmp_buf,norm_len);
}
/* }}} */

/* {{{ proto string Blitz->getContext(void) */
static PHP_FUNCTION(blitz_get_context) {
    zval *id, **desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);
    RETVAL_STRING(tpl->current_path, 1);
}
/* }}} */

/* {{{ proto bool Blitz->iterate([string path [, mixed nonexistent]]) */
static PHP_FUNCTION(blitz_iterate)
{
    zval *id, **desc, *p_iterate_nonexistant = NULL;
    blitz_tpl *tpl;
    char *path = NULL;
    int path_len = 0;
    int iterate_nonexistant = 0;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|sz",&path, &path_len, &p_iterate_nonexistant)) {
        return;
    }

    if (p_iterate_nonexistant) {
         BLITZ_ZVAL_NOT_EMPTY(&p_iterate_nonexistant, iterate_nonexistant);
    }

    if (blitz_prepare_iteration(tpl, path, path_len, iterate_nonexistant TSRMLS_CC)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool Blitz->set(array input) */
static PHP_FUNCTION(blitz_set)
{
    zval *id, **desc, *input_arr;
    blitz_tpl *tpl;
    int res;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &input_arr)) {
        return;
    }

    res = blitz_merge_iterations_set(tpl, input_arr TSRMLS_CC);

    if (res) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool Blitz->block(mixed p1 [, mixed p2[, mixed nonexistent]]) */
static PHP_FUNCTION(blitz_block) {
    zval *id, **desc;
    blitz_tpl *tpl;
    zval *p1, *p2 = NULL, *p_iterate_nonexistant = NULL, *input_arr = NULL;
    char *path = NULL;
    int path_len = 0;
    int iterate_nonexistant = 0;
    int res = 0;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z|zz", &p1, &p2, &p_iterate_nonexistant)) {
        return;
    }

    if (IS_STRING == Z_TYPE_P(p1)) {
        path = Z_STRVAL_P(p1);
        path_len = Z_STRLEN_P(p1);
        if (p2 && (IS_ARRAY == Z_TYPE_P(p2))) {
            input_arr = p2;
        }
    } else if (IS_NULL == Z_TYPE_P(p1)) {
        if (p2 && (IS_ARRAY == Z_TYPE_P(p2))) {
            input_arr = p2;
        }
    } else if (IS_ARRAY == Z_TYPE_P(p1)) {
        input_arr = p1;
    } else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "first pararmeter can be only NULL, string or array");
        RETURN_FALSE;
    }

    if (p_iterate_nonexistant) {
        BLITZ_ZVAL_NOT_EMPTY(&p_iterate_nonexistant, iterate_nonexistant)
    }

    res = blitz_prepare_iteration(tpl, path, path_len, iterate_nonexistant TSRMLS_CC);
    if (res == -1) { /* context doesn't exist */
        RETURN_TRUE;
    } else if (res == 0) { /* error */
        RETURN_FALSE;
    }

    /* copy params array to current iteration */
    if (input_arr && zend_hash_num_elements(Z_ARRVAL_P(input_arr)) > 0) {
        if (tpl->last_iteration && *tpl->last_iteration) {
            zval_dtor(*(tpl->last_iteration));
            **(tpl->last_iteration) = *input_arr;
            zval_copy_ctor(*tpl->last_iteration);
            INIT_PZVAL(*tpl->last_iteration);
        } else {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "INTERNAL ERROR: last_iteration is empty, it's a bug. Send the test case to developer, please.\n"); 
            RETURN_FALSE;
        }
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string Blitz->include(string filename [, array params] ) */
static PHP_FUNCTION(blitz_include)
{
    blitz_tpl *tpl, *itpl;
    char *filename;
    int filename_len;
    zval **desc, *id;
    char *result = NULL;
    unsigned long result_len = 0;
    zval *input_arr = NULL;
    zval *iteration_params = NULL;
    int res = 0;
    int do_merge = 0;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &filename, &filename_len, &input_arr)) {
        return;
    }

    if (!filename) RETURN_FALSE;

    if (input_arr && (IS_ARRAY == Z_TYPE_P(input_arr)) && (0 < zend_hash_num_elements(Z_ARRVAL_P(input_arr)))) {
        do_merge = 1;
    } else {
        // we cannot mix this with merging (blitz_merge_iterations_set), 
        // because merging is writing to the parent iteration set
        if (tpl->caller_iteration)
            iteration_params = *tpl->caller_iteration;
    }

    if (!blitz_include_tpl_cached(tpl, filename, filename_len, iteration_params, &itpl TSRMLS_CC))
        RETURN_FALSE;

    if (do_merge) {
        if (!blitz_merge_iterations_set(itpl, input_arr TSRMLS_CC)) {
            RETURN_FALSE;
        }
    }

    res = blitz_exec_template(itpl, id, &result, &result_len TSRMLS_CC);

    if (res) {
        RETVAL_STRINGL(result, result_len, 1);
        if (res == 1) efree(result);
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto string Blitz->fetch(string path, array params) */
static PHP_FUNCTION(blitz_fetch)
{
    zval *id, **desc;
    blitz_tpl *tpl;
    char *result = NULL;
    unsigned long result_len = 0;
    char exec_status = 0;
    char *path;
    int path_len, norm_len, res, current_len = 0;
    zval *input_arr = NULL, **elem;
    HashTable *input_ht = NULL;
    char *key = NULL;
    unsigned int key_len = 0;
    unsigned long index = 0;
    zval *path_iteration = NULL;
    zval *path_iteration_parent = NULL;
    zval *final_params = NULL;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|a", &path, &path_len, &input_arr)) {
        return;
    }

    /* find corresponding iteration data */
    res = blitz_normalize_path(&tpl->tmp_buf, path, path_len, tpl->current_path, current_len TSRMLS_CC);
    current_len = strlen(tpl->current_path);
    norm_len = strlen(tpl->tmp_buf);

    /* 2DO: using something like current_iteration and current_iteration_parent can speed up next step,  */
    /* but for other speed-up purposes it's not guaranteed that current_iteration and current_iteration_parent  */
    /* point to really current values. that's why we cannot just check tpl->current_path == tpl->tmp_buf and use them */
    /* we always find iteration by path instead */
    res = blitz_find_iteration_by_path(tpl, tpl->tmp_buf, norm_len, &path_iteration, &path_iteration_parent TSRMLS_CC);
    if (!res) {
        if (BLITZ_DEBUG) php_printf("blitz_find_iteration_by_path failed!\n");
        final_params = input_arr;
    } else {
        if (BLITZ_DEBUG) php_printf("blitz_find_iteration_by_path: pi=%p ti=%p &ti=%p\n", path_iteration, tpl->iterations, &tpl->iterations);
        final_params = path_iteration;
    }

    if (BLITZ_DEBUG) {
        php_printf("tpl->iterations:\n");
        if (tpl->iterations) php_var_dump(&tpl->iterations,1 TSRMLS_CC);
    }

    /* merge data with input params */
    if (input_arr && path_iteration) {
        if (BLITZ_DEBUG) php_printf("merging current iteration and params in fetch\n");
        input_ht = Z_ARRVAL_P(input_arr);
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(path_iteration));
        zend_hash_internal_pointer_reset(input_ht);

        while (zend_hash_get_current_data(input_ht, (void**) &elem) == SUCCESS) {
            if (zend_hash_get_current_key_ex(input_ht, &key, &key_len, &index, 0, NULL) != HASH_KEY_IS_STRING) {
                zend_hash_move_forward(input_ht);
                continue;
            }

            if (key && key_len) {
                zval *temp;

                ALLOC_ZVAL(temp);
                *temp = **elem;
                zval_copy_ctor(temp);
                INIT_PZVAL(temp);

                zend_hash_update(Z_ARRVAL_P(path_iteration), key, key_len, (void*)&temp, sizeof(zval *), NULL);
            }
            zend_hash_move_forward(input_ht);
        }
    }

    if (BLITZ_DEBUG) {
        php_printf("tpl->iterations:\n");
        if (tpl->iterations && &tpl->iterations) php_var_dump(&tpl->iterations,1 TSRMLS_CC);
        php_printf("final_params:\n");
        if (final_params) php_var_dump(&final_params,1 TSRMLS_CC);
    }

    exec_status = blitz_fetch_node_by_path(tpl, id, tpl->tmp_buf, norm_len, final_params, &result, &result_len TSRMLS_CC);
    
    if (exec_status) {
        RETVAL_STRINGL(result, result_len, 1);
        if (exec_status == 1) efree(result);

        /* clean-up parent path iteration after the fetch */
        if (path_iteration_parent) {
            zend_hash_internal_pointer_end(Z_ARRVAL_P(path_iteration_parent));
            if (HASH_KEY_IS_LONG == zend_hash_get_current_key_ex(Z_ARRVAL_P(path_iteration_parent), &key, &key_len, &index, 0, NULL)) {
                /*tpl->last_iteration = NULL; */
                zend_hash_index_del(Z_ARRVAL_P(path_iteration_parent),index);
            } else {
                zend_hash_clean(Z_ARRVAL_P(path_iteration_parent));
            }
        } 

        /* reset current iteration pointer if it's current iteration */
        if ((current_len == norm_len) && (0 == strncmp(tpl->tmp_buf, tpl->current_path, norm_len))) {
            tpl->current_iteration = NULL;
        }

    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ pool bool Blitz->clean([string path [, mixed warn_not_found ]]) */
static PHP_FUNCTION(blitz_clean)
{
    zval *id, **desc;
    blitz_tpl *tpl;
    char *path = NULL;
    int path_len = 0, norm_len = 0, res = 0, current_len = 0;
    zval *path_iteration = NULL, *path_iteration_parent = NULL, *warn_param = NULL;
    int warn_not_found = 1;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz", &path, &path_len, &warn_param)) {
        return;
    }

    /* warn_param = FALSE: throw no warning when iteration is not found. All other cases - do please. */
    if (warn_param && (IS_BOOL == Z_TYPE_P(warn_param)) && (0 == Z_LVAL_P(warn_param))) {
        warn_not_found = 0;
    }

    /* find corresponding iteration data */
    res = blitz_normalize_path(&tpl->tmp_buf, path, path_len, tpl->current_path, current_len TSRMLS_CC);

    current_len = strlen(tpl->current_path);
    norm_len = strlen(tpl->tmp_buf);

    res = blitz_find_iteration_by_path(tpl, tpl->tmp_buf, norm_len, &path_iteration, &path_iteration_parent TSRMLS_CC);
    if (!res) {
        if (warn_not_found) {
            php_error(E_WARNING, "unable to find iteration by path %s", tpl->tmp_buf);
            RETURN_FALSE;
        } else {
            RETURN_TRUE;
        }
    }

    /* clean-up parent iteration */
    zend_hash_clean(Z_ARRVAL_P(path_iteration_parent));

    /* reset current iteration pointer if it's current iteration  */
    if ((current_len == norm_len) && (0 == strncmp(tpl->tmp_buf, tpl->current_path, norm_len))) {
        tpl->current_iteration = NULL;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ blitz_functions[] : Blitz class */
static const zend_function_entry blitz_functions[] = {
    PHP_FALIAS(blitz,               blitz_init,                 NULL)
    PHP_FALIAS(load,                blitz_load,                 NULL)
    PHP_FALIAS(dump_struct,         blitz_dump_struct,          NULL)
    PHP_FALIAS(get_struct,          blitz_get_struct,           NULL)
    PHP_FALIAS(get_iterations,      blitz_get_iterations,       NULL)
    PHP_FALIAS(get_context,         blitz_get_context,          NULL)
    PHP_FALIAS(has_context,         blitz_has_context,          NULL)
    PHP_FALIAS(set_global,          blitz_set_global,           NULL)
    PHP_FALIAS(set_globals,         blitz_set_global,           NULL)
    PHP_FALIAS(get_globals,         blitz_get_globals,          NULL)
    PHP_FALIAS(set,                 blitz_set,                  NULL)
    PHP_FALIAS(assign,              blitz_set,                  NULL)
    PHP_FALIAS(parse,               blitz_parse,                NULL)
    PHP_FALIAS(display,             blitz_display,              NULL)
    PHP_FALIAS(include,             blitz_include,              NULL)
    PHP_FALIAS(iterate,             blitz_iterate,              NULL)
    PHP_FALIAS(context,             blitz_context,              NULL)
    PHP_FALIAS(block,               blitz_block,                NULL)
    PHP_FALIAS(fetch,               blitz_fetch,                NULL)
    PHP_FALIAS(clean,               blitz_clean,                NULL)
    PHP_FALIAS(dumpstruct,          blitz_dump_struct,          NULL)
    PHP_FALIAS(getstruct,           blitz_get_struct,           NULL)
    PHP_FALIAS(getiterations,       blitz_get_iterations,       NULL)
    PHP_FALIAS(hascontext,          blitz_has_context,          NULL)
    PHP_FALIAS(getcontext,          blitz_get_context,          NULL)
    PHP_FALIAS(setglobal,           blitz_set_global,           NULL)
    PHP_FALIAS(setglobals,          blitz_set_global,           NULL)
    PHP_FALIAS(getglobals,          blitz_get_globals,          NULL)
    {NULL, NULL, NULL}
};
/* }}} */

PHP_MINIT_FUNCTION(blitz) /* {{{ */
{
    ZEND_INIT_MODULE_GLOBALS(blitz, php_blitz_init_globals, NULL);
    REGISTER_INI_ENTRIES();

    le_blitz = zend_register_list_destructors_ex(blitz_resource_dtor, NULL, "Blitz template", module_number);

    INIT_CLASS_ENTRY(blitz_class_entry, "blitz", blitz_functions);
    zend_register_internal_class(&blitz_class_entry TSRMLS_CC);

    return SUCCESS;
}
/* }}} */

PHP_MSHUTDOWN_FUNCTION(blitz) /* {{{ */
{
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}
/* }}} */

PHP_MINFO_FUNCTION(blitz) /* {{{ */
{
    php_info_print_table_start();
    php_info_print_table_row(2, "Blitz support", "enabled");
    php_info_print_table_row(2, "Version", BLITZ_VERSION_STRING);
    php_info_print_table_row(2, "Revision", "$Revision$");
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ blitz_module_entry */
zend_module_entry blitz_module_entry = {
    STANDARD_MODULE_HEADER,
    "blitz",
    NULL,
    PHP_MINIT(blitz),
    PHP_MSHUTDOWN(blitz),
    NULL,
    NULL,
    PHP_MINFO(blitz),
    BLITZ_VERSION_STRING,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker expandtab
 * vim<600: sw=4 ts=4
 */
