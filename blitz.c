/*
  +----------------------------------------------------------------------+
  | Blitz Templates                                                      |
  +----------------------------------------------------------------------+
  | Downloads and online documentation:                                  |
  |     http://alexeyrybak.com/blitz/                                    |
  |     http://github.com/alexeyrybak/blitz/                             |
  | Author:                                                              |
  |     Alexey Rybak [fisher] <alexey.rybak at gmail dot com>            |
  | Bugfixes and patches:                                                |
  |     Antony Dovgal [tony2001] <tony at daylessday dot org>            |
  |     Konstantin Baryshnikov [fixxxer] <konstantin at symbi dot info>  |
  |     and many others, please see full list on github                  |
  | Mailing list:                                                        |
  |     blitz-php at googlegroups dot com                                |
  +----------------------------------------------------------------------+
*/

#define BLITZ_DEBUG 0
#define BLITZ_VERSION_STRING "0.10.5"

#ifndef PHP_WIN32
#include <sys/mman.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if PHP_API_VERSION < 20100412
#include "safe_mode.h"
#endif

#include "php_globals.h"
#include "php_ini.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/info.h"
#include "ext/standard/html.h"
#include "Zend/zend_exceptions.h"
#include <fcntl.h>

#ifdef BLITZ_DEBUG
#include "ext/standard/php_smart_string.h"
#include "zend_smart_str.h"
#endif

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

#include "SAPI.h"

#include "php_blitz.h"

ZEND_DECLARE_MODULE_GLOBALS(blitz)

/* some declarations  */
static void blitz_error (blitz_tpl *tpl, unsigned int level, char *format, ...);
static int blitz_exec_template(blitz_tpl *tpl, zval *id, smart_str *result);
static int blitz_exec_nodes(blitz_tpl *tpl, blitz_node *first, zval *id,
    smart_str *result, unsigned long *result_alloc_len,
    unsigned long parent_begin, unsigned long parent_end, zval *ctx_data, zval *parent_params);
static int blitz_exec_nodes_ex(blitz_tpl *tpl, blitz_node *first, zval *id,
    smart_str *result, unsigned long *result_alloc_len,
    unsigned long parent_begin, unsigned long parent_end, zval *ctx_data, zval *parent_params, int push_stack);
static inline int blitz_analize (blitz_tpl *tpl);
static inline void blitz_remove_spaces_around_context_tags(blitz_tpl *tpl);
static inline unsigned int blitz_extract_var(blitz_tpl *tpl, char *name, unsigned long len, unsigned char is_path, zval *params, long *l, zval **z);
static inline int blitz_arg_to_zval(blitz_tpl *tpl, blitz_node *node, zval *parent_params, call_arg *arg, zval *return_value);
static inline void blitz_check_arg(blitz_tpl *tpl, blitz_node *node, zval *parent_params, int *not_empty);
static inline void blitz_check_expr(blitz_tpl *tpl, blitz_node *node, zval *id, unsigned int node_count, zval *parent_params, int *is_true);

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

#define INIT_CALL_ARGS(n)                                                       \
    (n)->args = ecalloc(BLITZ_CALL_ALLOC_ARG_INIT, sizeof(call_arg));           \
    (n)->n_args = 0;                                                            \
    (n)->n_if_args = 0;                                                         \
    (n)->n_arg_alloc = BLITZ_CALL_ALLOC_ARG_INIT;

#define REALLOC_ARG_IF_EXCEEDS(n)                                               \
    if ((n)->n_args >= (n)->n_arg_alloc) {                                      \
        (n)->n_arg_alloc = (n)->n_arg_alloc << 1;                               \
        (n)->args = erealloc((n)->args,(n)->n_arg_alloc*sizeof(call_arg));      \
    }

#define ADD_CALL_ARGS(n, buf, i_len, i_type)                                    \
    REALLOC_ARG_IF_EXCEEDS(n);                                                  \
    i_arg = (n)->args + (n)->n_args;                                            \
    if (i_len) {                                                                \
        i_arg->name = estrndup((char *)(buf),(i_len));                          \
        i_arg->len = (i_len);                                                   \
    } else {                                                                    \
        i_arg->name = "";                                                       \
        i_arg->len = 0;                                                         \
    }                                                                           \
    i_arg->type = (i_type);                                                     \
    ++(n)->n_args;

#define GET_CALL_ARGS_SIZE(n)                                                   \
    ((n)->n_args)

#define INDIRECT_CONTINUE_FORWARD(hash, zv)    \
    if (Z_TYPE_P(zv) == IS_INDIRECT) {         \
        zv = Z_INDIRECT_P(zv);                 \
    }                                          \
    if (Z_TYPE_P(zv) == IS_UNDEF) {            \
        zend_hash_move_forward(hash); \
        continue;                              \
    }

#define INDIRECT_CONTINUE_FORWARD_EX(hash, pos, zv)    \
    if (Z_TYPE_P(zv) == IS_INDIRECT) {                 \
        zv = Z_INDIRECT_P(zv);                         \
    }                                                  \
    if (Z_TYPE_P(zv) == IS_UNDEF) {                    \
        zend_hash_move_forward_ex(hash, pos);          \
        continue;                                      \
    }

#define INDIRECT_RETURN(zv, val)               \
    if (Z_TYPE_P(zv) == IS_INDIRECT) {         \
        zv = Z_INDIRECT_P(zv);                 \
    }                                          \
    if (Z_TYPE_P(zv) == IS_UNDEF) {            \
        return val;                            \
    }

#define DEREF_ZVAL(zv)                       \
        if (Z_TYPE_P(zv) == IS_REFERENCE) {  \
            zv = Z_REFVAL_P(zv);             \
        }

static zend_always_inline zval *blitz_hash_str_find_ind(const HashTable *ht, const char *str, size_t len) /* {{{ */
{
   zval *zv;
   zv = zend_hash_str_find_ind(ht, str, len);
   if (zv && Z_TYPE_P(zv) == IS_REFERENCE) {
       zv = Z_REFVAL_P(zv);
   }
   return zv;
}
/* }}} */

static zend_always_inline zval *blitz_hash_index_find(const HashTable *ht, zend_ulong h) /* {{{ */
{
   zval *zv;
   zv = zend_hash_index_find(ht, h);
   if (zv && Z_TYPE_P(zv) == IS_REFERENCE) {
       zv = Z_REFVAL_P(zv);
   }
   return zv;
}
/* }}} */

static zend_always_inline zval *blitz_hash_get_current_data(const HashTable *ht) /* {{{ */
{
   zval *zv;
   zv = zend_hash_get_current_data((HashTable*)ht);
   if (zv && Z_TYPE_P(zv) == IS_REFERENCE) {
       zv = Z_REFVAL_P(zv);
   }
   return zv;
}
/* }}} */

static zend_always_inline zval *blitz_hash_get_current_data_ex(const HashTable *ht, HashPosition *pos) /* {{{ */
{
   zval *zv;
   zv = zend_hash_get_current_data_ex((HashTable*)ht, pos);
   if (zv && Z_TYPE_P(zv) == IS_REFERENCE) {
       zv = Z_REFVAL_P(zv);
   }
   return zv;
}
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

    if (new_value && new_value->len == 0) {
        *p = '\x0';
    } else {
        if (!new_value || new_value->len != 1) {
            blitz_error(NULL, E_WARNING, "failed to set blitz.var_prefix (only one character is allowed, like $ or %%)");
            return FAILURE;
        }
        *p = new_value->val[0];
    }

    return SUCCESS;
}
/* }}} */

/* {{{ ini options */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("blitz.throw_exceptions", "0", PHP_INI_ALL,
        OnUpdateBool, throw_exceptions, zend_blitz_globals, blitz_globals)
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
    STD_PHP_INI_ENTRY("blitz.remove_spaces_around_context_tags", "1", PHP_INI_ALL,
        OnUpdateBool, remove_spaces_around_context_tags, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.warn_context_duplicates", "0", PHP_INI_ALL,
        OnUpdateBool, warn_context_duplicates, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.check_recursion", "1", PHP_INI_ALL,
        OnUpdateBool, check_recursion, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.scope_lookup_limit", "0", PHP_INI_ALL,
        OnUpdateLongLegacy, scope_lookup_limit, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.lower_case_method_names", "1", PHP_INI_ALL,
        OnUpdateBool, lower_case_method_names, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.enable_include", "1", PHP_INI_ALL,
        OnUpdateBool, enable_include, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.enable_callbacks", "1", PHP_INI_ALL,
        OnUpdateBool, enable_callbacks, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.enable_php_callbacks", "1", PHP_INI_ALL,
        OnUpdateBool, enable_php_callbacks, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.php_callbacks_first", "1", PHP_INI_ALL,
        OnUpdateBool, php_callbacks_first, zend_blitz_globals, blitz_globals)
    STD_PHP_INI_ENTRY("blitz.auto_escape", "0", PHP_INI_ALL,
        OnUpdateBool, auto_escape, zend_blitz_globals, blitz_globals)

PHP_INI_END()
/* }}} */

static void dump_call_args(blitz_node *node) {
    int i;
    php_printf("[D] call args (%u):\n", node->n_args);
    call_arg *n_args = node->args;
    for (i = 0; i < node->n_args; i++) {
        n_args = node->args + i;
        php_printf("- %d: name:%s len:%lu type:%s (%d)\n", i, n_args->name,
            n_args->len, BLITZ_ARG_TO_STRING(n_args->type), n_args->type);
    }
}

static void dump_if_stack(call_arg *stack, int stack_level) {
    int i;
    char *str;
    php_printf("[D] if stack (%u):\n", stack_level + 1);
    for (i = 0; i <= stack_level; i++) {
        str = estrndup(stack[i].name, stack[i].len);
        php_printf("- %d: type:%s (%d)\n", i, str, stack[i].type);
        efree(str);
    }
}

static void blitz_error (blitz_tpl *tpl, unsigned int level, char *format, ...) { /* {{{ */
    char *msg = NULL;
    unsigned char free_msg = 0;

    va_list arg;
    va_start(arg, format);

    if (tpl) {
        if (tpl->error) {
            efree(tpl->error);
        }

        vspprintf(&tpl->error, BLITZ_ERROR_MAX_LEN, format, arg);
        msg = tpl->error;
    } else {
        vspprintf(&msg, BLITZ_ERROR_MAX_LEN, format, arg);
        free_msg = 1;
    }

    va_end(arg);

    php_error_docref(NULL, level, "%s", msg);

    if (BLITZ_G(throw_exceptions) && level == E_WARNING) {
        zend_throw_exception_ex(zend_exception_get_default(), 0, "%s", msg);
    }

    if (free_msg) {
        efree(msg);
    }
}
/* }}} */

static inline unsigned int blitz_realloc_list (blitz_list *list, unsigned long new_size) { /* {{{ */
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

static inline int blitz_read_with_stream(blitz_tpl *tpl) /* {{{ */
{
    char *filename = tpl->static_data.name;
    php_stream *stream;
    zend_string *body;

    if (php_check_open_basedir(filename)) {
        return 0;
    }

    stream = php_stream_open_wrapper(filename, "rb", IGNORE_PATH|IGNORE_URL|IGNORE_URL_WIN|REPORT_ERRORS, NULL);
    if (!stream) {
        return 0;
    }

    body = php_stream_copy_to_mem(stream, PHP_STREAM_COPY_ALL, 0);
    if (body) {
        tpl->static_data.body.s = body;
        tpl->static_data.body.a = ZSTR_LEN(body);
    }
    php_stream_close(stream);
    return 1;
}
/* }}} */

static inline int blitz_read_with_fread(blitz_tpl *tpl) /* {{{ */
{
    FILE *stream;
    unsigned int get_len;
    char *filename = tpl->static_data.name;
    char buf[BLITZ_INPUT_BUF_SIZE];

    if (php_check_open_basedir(filename)) {
        return 0;
    }

    /* VCWD_FOPEN() fixes problems with relative paths in multithreaded environments */
    if (!(stream = VCWD_FOPEN(filename, "rb"))) {
        blitz_error(tpl, E_WARNING, "unable to open file \"%s\"", filename);
        return 0;
    }

    while ((get_len = fread(buf, 1, BLITZ_INPUT_BUF_SIZE, stream)) > 0) {
        smart_str_appendl(&tpl->static_data.body, buf, get_len);
    }
    fclose(stream);

    return 1;
}
/* }}} */

static blitz_tpl *blitz_init_tpl_base(HashTable *globals, zval *iterations, blitz_tpl *tpl_parent) /* {{{ */
{
    blitz_static_data *static_data = NULL;
    blitz_tpl *tpl = ecalloc(1, sizeof(blitz_tpl));


    static_data = & tpl->static_data;
    /* make sure body is always not-NULL */
    smart_str_alloc(&static_data->body, 1, 0);
    smart_str_0(&static_data->body);

    tpl->flags = 0;
    static_data->n_nodes = 0;
    static_data->nodes = NULL;
    static_data->tag_open_len = strlen(BLITZ_G(tag_open));
    static_data->tag_close_len = strlen(BLITZ_G(tag_close));
    static_data->tag_open_alt_len = strlen(BLITZ_G(tag_open_alt));
    static_data->tag_close_alt_len = strlen(BLITZ_G(tag_close_alt));
    static_data->tag_comment_open_len = strlen(BLITZ_G(tag_comment_open));
    static_data->tag_comment_close_len = strlen(BLITZ_G(tag_comment_close));


    tpl->loop_stack_level = 0;

    if (iterations) {
        /*
           "Inherit" iterations from opener, used for incudes. Just make a copy mark template
           with special flag BLITZ_FLAG_ITERATION_IS_OTHER not to free this object.
        */
        ZVAL_COPY_VALUE(&tpl->iterations, iterations);
        tpl->flags |= BLITZ_FLAG_ITERATION_IS_OTHER;
    } else {
        array_init(&tpl->iterations);
    }

    if (tpl_parent) {
        tpl->tpl_parent = tpl_parent;
    } else {
        tpl->tpl_parent = NULL;
    }

    tpl->error = NULL;
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
    call_arg *a = NULL;

    a = node->args;
    for (i = 0; i < node->n_args; i++, a++) {
        if (a->len) {
            efree(a->name);
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

    smart_str_free(&tpl->static_data.body);

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

    if (!(tpl->flags & BLITZ_FLAG_ITERATION_IS_OTHER)) { /* XXX */
        zval_ptr_dtor(&tpl->iterations);
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

    if (tpl->error) {
        efree(tpl->error);
    }

    efree(tpl);
}
/* }}} */

static blitz_tpl *blitz_init_tpl(const char *filename, int filename_len, HashTable *globals, zval *iterations, blitz_tpl *tpl_parent) /* {{{ */
{
    int global_path_len = 0;
    unsigned int filename_normalized_len = 0;
    unsigned int add_buffer_len = 0;
    int result = 0;
    blitz_tpl *tpl = NULL;
    blitz_tpl *i_tpl = NULL;
    char *s = NULL;
    int i = 0, check_loop_max = BLITZ_LOOP_STACK_MAX;
    blitz_static_data *static_data;

    tpl = blitz_init_tpl_base(globals, iterations, tpl_parent);

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
            blitz_error(NULL, E_WARNING, "INTERNAL ERROR: file path is too long (limited by MAXPATHLEN)");
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
            blitz_error(NULL, E_WARNING, "unable to open file \"%s\" (realpath failed)", filename);
            blitz_free_tpl(tpl);
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
                blitz_error(NULL, E_WARNING,
                    "ERROR: include recursion detected for \"%s\". You can disable this check setting blitz.check_recursion to 0 (please, don't do this if you don't know what you are doing)",
                    tpl->static_data.name
                );
                blitz_free_tpl(tpl);
                return NULL;
            }
        }
    }

#ifdef BLITZ_USE_STREAMS
    result = blitz_read_with_stream(tpl);
#else
    result = blitz_read_with_fread(tpl);
#endif

    if (!result) {
        return tpl;
    }

    /* search algorithm requires lager buffer: body_len + add_buffer */
    static_data = & tpl->static_data;
    add_buffer_len = MAX(
        MAX(static_data->tag_open_len, static_data->tag_close_len),
        MAX(
            MAX(static_data->tag_open_alt_len, static_data->tag_close_alt_len),
            MAX(static_data->tag_comment_open_len, static_data->tag_comment_close_len)
        )
    );

    smart_str_erealloc(&tpl->static_data.body, ZSTR_LEN(tpl->static_data.body.s) + add_buffer_len);
    memset(ZSTR_VAL(tpl->static_data.body.s) + ZSTR_LEN(tpl->static_data.body.s), '\0', add_buffer_len);

    return tpl;
}
/* }}} */

static int blitz_load_body(blitz_tpl *tpl, zend_string *body) /* {{{ */
{
    unsigned int add_buffer_len = 0;
    char *name = "noname_loaded_from_zval";
    int name_len = sizeof("noname_loaded_from_zval") - 1;

    if (!tpl || !body || !ZSTR_LEN(body)) {
        return 0;
    }

    add_buffer_len = MAX(
        MAX(tpl->static_data.tag_open_len, tpl->static_data.tag_close_len),
        MAX(
            MAX(tpl->static_data.tag_open_alt_len, tpl->static_data.tag_close_alt_len),
            MAX(tpl->static_data.tag_comment_open_len, tpl->static_data.tag_comment_close_len)
        )
    );

    smart_str_append_ex(&tpl->static_data.body, body, 0);
    smart_str_erealloc(&tpl->static_data.body, ZSTR_LEN(tpl->static_data.body.s) + add_buffer_len);
    smart_str_0(&tpl->static_data.body);

    memcpy(tpl->static_data.name, name, name_len);

    return 1;
}
/* }}} */

static int blitz_include_tpl_cached(blitz_tpl *tpl, const char *filename, unsigned int filename_len,
    zval *iteration_params, blitz_tpl **itpl) /* {{{ */
{
    zval *desc;
    unsigned long itpl_idx = 0;
    zval temp;
    unsigned char has_protected_iteration = 0;

    /* try to find already parsed tpl index */
    desc = blitz_hash_str_find_ind(tpl->ht_tpl_name, (char *)filename, filename_len);
    if (desc) {
        *itpl = tpl->itpl_list[zval_get_long(desc)];
        if (iteration_params) {
            ZVAL_COPY_VALUE(&(*itpl)->iterations, iteration_params);
            (*itpl)->flags |= BLITZ_FLAG_ITERATION_IS_OTHER;
        } else {
            has_protected_iteration = (*itpl)->flags & BLITZ_FLAG_ITERATION_IS_OTHER;

            if (!has_protected_iteration) {
                zend_hash_clean(HASH_OF(&(*itpl)->iterations));
            } else if (Z_ISUNDEF_P(&(*itpl)->iterations)) {
                array_init(&(*itpl)->iterations);
                (*itpl)->flags ^= BLITZ_FLAG_ITERATION_IS_OTHER;
            }
        }

        return 1;
    }

    if (filename_len >= MAXPATHLEN) {
        blitz_error(NULL, E_WARNING, "Filename exceeds the maximum allowed length of %d characters", MAXPATHLEN);
        return 0;
    }

    /* initialize template */

    if (!(*itpl = blitz_init_tpl(filename, filename_len, tpl->hash_globals, iteration_params, tpl))) {
        return 0;
    }

    /* analize template */
    if (!blitz_analize(*itpl)) {
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
    ZVAL_LONG(&temp, itpl_idx);
    zend_hash_str_update(tpl->ht_tpl_name, (char *)filename, filename_len, &temp);
    tpl->itpl_list_len++;

    return 1;
}
/* }}} */

static void blitz_resource_dtor(zend_resource *rsrc) /* {{{ */
{
    blitz_tpl *tpl = NULL;
    tpl = (blitz_tpl*)rsrc->ptr;
    blitz_free_tpl(tpl);
}
/* }}} */

static void php_blitz_init_globals(zend_blitz_globals *blitz_globals) /* {{{ */
{
    memset(blitz_globals, 0, sizeof(zend_blitz_globals));
    blitz_globals->var_prefix = BLITZ_TAG_VAR_PREFIX;
    blitz_globals->tag_open = BLITZ_TAG_OPEN;
    blitz_globals->tag_close = BLITZ_TAG_CLOSE;
    blitz_globals->tag_open_alt = BLITZ_TAG_OPEN_ALT;
    blitz_globals->tag_close_alt = BLITZ_TAG_CLOSE_ALT;
    blitz_globals->enable_alternative_tags = 1;
    blitz_globals->enable_comments = 0;
    blitz_globals->enable_include = 1;
    blitz_globals->enable_callbacks = 1;
    blitz_globals->enable_php_callbacks = 1;
    blitz_globals->php_callbacks_first = 1;
    blitz_globals->path = "";
    blitz_globals->remove_spaces_around_context_tags = 1;
    blitz_globals->warn_context_duplicates = 0;
    blitz_globals->check_recursion = 1;
    blitz_globals->tag_comment_open = BLITZ_TAG_COMMENT_OPEN;
    blitz_globals->tag_comment_close = BLITZ_TAG_COMMENT_CLOSE;
    blitz_globals->scope_lookup_limit = 0;
    blitz_globals->auto_escape = 0;
    blitz_globals->throw_exceptions = 0;
}
/* }}} */

static void blitz_get_node_paths(zval *list, blitz_node *node, const char *parent_path, unsigned int skip_vars, unsigned int with_type) /* {{{ */
{

    char suffix[2] = "\x0";
    char path[BLITZ_CONTEXT_PATH_MAX_LEN] = "\x0";
    blitz_node *i_node = NULL;

    if (!node)
        return;

    /* hidden, error or non-finalized node (end was not found) */
    if (node->hidden || node->type == BLITZ_NODE_TYPE_BEGIN)
        return;

    if (node->type == BLITZ_NODE_TYPE_CONTEXT ||
        node->type == BLITZ_NODE_TYPE_LITERAL ||
        node->type == BLITZ_NODE_TYPE_IF_CONTEXT ||
        node->type == BLITZ_NODE_TYPE_UNLESS_CONTEXT ||
        node->type == BLITZ_NODE_TYPE_ELSEIF_CONTEXT ||
        node->type == BLITZ_NODE_TYPE_ELSE_CONTEXT)
    {
        suffix[0] = '/';

        if (node->type == BLITZ_NODE_TYPE_CONTEXT) { /* contexts: use context name from args instead of useless "BEGIN" */
            sprintf(path, "%s%s%s", parent_path, node->args[0].name, suffix);
        } else {
            sprintf(path, "%s%s%s", parent_path, node->lexem, suffix);
        }
        add_next_index_string(list, path);
    } else if (!skip_vars) {
        sprintf(path, "%s%s%s", parent_path, node->lexem, suffix);
        add_next_index_string(list, path);
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
    unsigned int level = 0;
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

static void php_blitz_get_node_tokens(blitz_node *node, zval *list) /* {{{ */
{
    unsigned long j = 0;
    blitz_node *child = NULL;
    zval node_token;
    zval args_list;
    zval child_list;
    zval arg;

    if (!node) {
        return;
    }

    array_init(&node_token);

    add_assoc_string(&node_token, "lexem", node->lexem);
    add_assoc_long(&node_token, "type", (unsigned int)node->type);
    add_assoc_long(&node_token, "begin", node->pos_begin);
    add_assoc_long(&node_token, "begin_shift", node->pos_begin_shift);
    add_assoc_long(&node_token, "end", node->pos_end);
    add_assoc_long(&node_token, "end_shift", node->pos_end_shift);

    if (BLITZ_IS_METHOD(node->type)) {

        if (node->n_args > 0 ) {
            array_init(&args_list);
            for (j = 0; j < node->n_args; ++j) {
                array_init(&arg);
                add_assoc_stringl(&arg, "name", node->args[j].name, node->args[j].len);
                add_assoc_long(&arg, "type", node->args[j].type);

                add_next_index_zval(&args_list, &arg);
            }

            add_assoc_zval(&node_token, "args", &args_list);
        }

        if (node->first_child) {
            child = node->first_child;
            array_init(&child_list);
            while (child) {
                php_blitz_get_node_tokens(child, &child_list);
                child = child->next;
            }

            add_assoc_zval(&node_token, "children", &child_list);
        }
    }

    add_next_index_zval(list, &node_token);
}
/* }}} */

static void php_blitz_get_tokens(blitz_tpl *tpl, zval *list) /* {{{ */
{
    blitz_node *node = NULL;
    node = tpl->static_data.nodes;

    while (node) {
        php_blitz_get_node_tokens(node, list);
        node = node->next;
    }
}
/* }}} */

void blitz_warn_context_duplicates(blitz_tpl *tpl) /* {{{ */
{
    zval path_list, dummy;
    HashTable uk_path;
    zval *path, *path_type;

    array_init(&path_list);
    ZVAL_NULL(&dummy);

    zend_hash_init(&uk_path, 10, NULL, ZVAL_PTR_DTOR, 0);

    blitz_get_path_list(tpl, &path_list, 1, 1);

    zend_hash_internal_pointer_reset(HASH_OF(&path_list));
    while ((path = blitz_hash_get_current_data(HASH_OF(&path_list))) != NULL) {
        INDIRECT_CONTINUE_FORWARD(HASH_OF(&path_list), path);
        zend_hash_move_forward(HASH_OF(&path_list));
        path_type = blitz_hash_get_current_data(HASH_OF(&path_list));
        INDIRECT_CONTINUE_FORWARD(HASH_OF(&path_list), path_type);

        if (Z_LVAL_P(path_type) == BLITZ_NODE_PATH_NON_UNIQUE) {
            zend_hash_move_forward(HASH_OF(&path_list));
            continue;
        }

        if (zend_hash_str_exists(&uk_path, Z_STRVAL_P(path), Z_STRLEN_P(path))) {
            blitz_error(tpl, E_WARNING,
                "WARNING: context name \"%s\" duplicate in %s",
                 Z_STRVAL_P(path), tpl->static_data.name
            );
        } else {
            zend_hash_str_add(&uk_path, Z_STRVAL_P(path), Z_STRLEN_P(path), &dummy);
        }

        zend_hash_move_forward(HASH_OF(&path_list));
    }

    zval_ptr_dtor(&path_list);
    zend_hash_destroy(&uk_path);

    return;
}
/* }}} */

static int blitz_find_tag_positions(blitz_string *body, blitz_list *list_pos) { /* {{{ */
    unsigned char current_tag_pos[BLITZ_TAG_LIST_LEN];
    unsigned char idx_tag[BLITZ_TAG_LIST_LEN];
    blitz_string list_tag[BLITZ_TAG_LIST_LEN];
    unsigned char tag_list_len = BLITZ_TAG_LIST_LEN;
    unsigned long pos = 0, pos_check_max = 0;
    unsigned char c = 0, p = 0, tag_id = 0, idx_tag_id = 0, found = 0;
    unsigned char *pc = NULL;
    unsigned int tag_min_len = 0;
    unsigned char tag_char_exists_map[BLITZ_CHAR_EXISTS_MAP_SIZE];
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

    memset(tag_char_exists_map, 0, BLITZ_CHAR_EXISTS_MAP_SIZE*sizeof(unsigned char));
    tag_min_len = strlen(list_tag[0].s);
    for (idx_tag_id = 0; idx_tag_id < tag_list_len; idx_tag_id++) {
        tag_id = idx_tag[idx_tag_id]; // loop through index, some tags can be disabled
        list_tag[tag_id].len = strlen(list_tag[tag_id].s);
        current_tag_pos[tag_id] = 0;
        if (list_tag[tag_id].len < tag_min_len) {
            tag_min_len = list_tag[tag_id].len;
        }

        pc = (unsigned char *) list_tag[tag_id].s;
        while (*pc) {
            tag_char_exists_map[*pc] = 1;
            pc++;
        }
    }

    if (body->len < tag_min_len) {
        return 1;
    }

    pos_check_max = body->len - tag_min_len;
    pc = (unsigned char *) body->s;
    c = *pc;
    pos = 0;

    while (c) {
        c = *pc;
        if (BLITZ_DEBUG) php_printf("moving at symbol: %c\n", c);
        found = 0;
        idx_tag_id = 0;
        while (idx_tag_id < tag_list_len) {
            tag_id = idx_tag[idx_tag_id]; // loop through index, some tags can be disabled
            t = list_tag + tag_id;
            p = current_tag_pos[tag_id];
            if (c == t->s[p]) {
                if (BLITZ_DEBUG) php_printf("checking tag_id = %d...\n", tag_id);
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
                    continue; // check the same tag from beginning
                }
            }

            idx_tag_id++;
        }

        if (found == 0) {
            while ((pos < pos_check_max) && 0 == tag_char_exists_map[*(pc + tag_min_len)]) {
                pc += tag_min_len;
                pos += tag_min_len;
            }
        }

        pc++;
        pos++;
    }

    return 1;
}
/* }}} */

/*

c - current char in the scanned string
pos - current scan position
i_pos - scanned position shuift
buf - where final scanned/transformed values go
i_len - true length for buf size
        doesn't include variable prefix like $ or just 1 for "TRUE"(saved as 't' or "FALSE"(saved as 'f')))
parse_comma - whether or not we can parse comma (","), a hack to allow IF parser to work in IF(a, b, c) syntax
*/

/* {{{ void blitz_parse_arg() */
static inline void blitz_parse_arg (char *text, char var_prefix,
    char *token_out, unsigned char *type, unsigned int *len, unsigned int *pos_out, int parse_comma)
{
    char *c = text;
    char *p = NULL;
    char symb = 0, i_symb = 0, is_path = 0;
    char ok = 0;
    unsigned int pos = 0, i_pos = 0, i_len = 0;
    unsigned char i_type = 0;
    char was_escaped = 0, has_dot = 0, is_operator = 0;

    *type = 0;
    *len = 0;
    *pos_out = 0;

    BLITZ_SKIP_BLANK(c,i_pos,pos);
    symb = *c;
    i_len = i_pos = ok = 0;
    p = token_out;

    if (BLITZ_DEBUG) php_printf("[F] blitz_parse_arg: %u '%c'\n", *c, *c);

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
        has_dot = is_operator = 0;
        BLITZ_SCAN_NUMBER(c, p, i_pos, i_symb, has_dot, is_operator);
        if (has_dot) {
            i_type = BLITZ_ARG_TYPE_FLOAT;
        } else if (is_operator) { // Make sure we don't scan the operator '-' as a number
            i_type = BLITZ_EXPR_OPERATOR_SUB;
        } else {
            i_type = BLITZ_ARG_TYPE_NUM;
        }
        i_len = i_pos;
    } else if (BLITZ_IS_ALPHA(symb)){
        is_path = 0;
        BLITZ_SCAN_VAR(c,p,i_pos,i_symb,is_path);
        i_len = i_pos;
        if (i_pos != 0) {
            ok = 1;
            if (BLITZ_STRING_IS_TRUE(token_out, i_len)) {
                i_len = 0;
                i_type = BLITZ_ARG_TYPE_TRUE;
            } else if (BLITZ_STRING_IS_FALSE(token_out, i_len)){
                i_len = 0;
                i_type = BLITZ_ARG_TYPE_FALSE;
            } else { /* treat this as variable name or as a method name */
                i_len = i_pos;
                /* variable can be used in a method call context, check for it */
                while (isspace(*c)) c++;
                if (is_path) {
                    i_type = BLITZ_ARG_TYPE_VAR_PATH;
                } else if (*c == '(') {
                    i_type = BLITZ_EXPR_OPERATOR_METHOD;
                } else {
                    i_type = BLITZ_ARG_TYPE_VAR;
                }
            }
        }
    } else if (BLITZ_IS_OPERATOR(symb) && (parse_comma || symb != ',')) {
        BLITZ_SCAN_EXPR_OPERATOR(c,i_pos,i_type);
        i_len = i_pos;
    }

    *type = i_type;
    *len = i_len;
    *pos_out = pos + i_pos;
}
/* }}} */

/* {{{ void blitz_parse_if() */
static inline void blitz_parse_if (char *text, unsigned int len_text, blitz_node *node, char var_prefix,
    unsigned int *pos_out, char *error_out)
{
    char *c = text;
    unsigned int pos = 0, i_pos = 0, i_len = 0, args_on_list = 0, args_since_bracket = 0;
    char buf[BLITZ_MAX_LEXEM_LEN];
    unsigned char i_type = 0;
    call_arg op_stack[BLITZ_IF_STACK_MAX];
    int op_len = -1;
    int method_is_first = 1, have_method_call = 0, brace_depth = 0; /* flags used to check whether or not method name is first or not */
    call_arg *i_arg = NULL;

    *error_out = 0;
    *pos_out = 0;

    if (BLITZ_DEBUG) {
        char *tmp = estrndup((char *)text, len_text + 1);
        tmp[len_text] = '\x0';
        php_printf("*** FUNCTION *** blitz_parse_if, started at pos=%u, c=%c, len=%u\n", pos, *c, len_text);
        php_printf("text: %s\n", tmp);
        efree(tmp);
    }

    /*
       As we accept complex conditions we need to convert the arguments to the postfix notation
       (see http://en.wikipedia.org/wiki/Reverse_Polish_notation)

       Thanks to Evgeniy Makhrov's (https://github.com/eelf/) research, we can also use RPN for method calls with
       variable arguments count in a very special case when expression looks like "callback(<args>)". Args cannot
       contain any other function calls
    */

    // Check for at least 1 valid argument
    blitz_parse_arg(c, var_prefix, buf, &i_type, &i_len, &i_pos, 1);
    if (!i_pos) {
        *error_out = BLITZ_CALL_ERROR_IF_CONTEXT;
        return;
    }

    // Push left bracket onto the stack
    BLITZ_IF_STACK_PUSH(op_stack, op_len, "(", 1, BLITZ_EXPR_OPERATOR_LP, *error_out);
    if (*error_out) {
        return;
    }

    args_since_bracket = 0;
    if (BLITZ_DEBUG) {
        dump_call_args(node);
        dump_if_stack(op_stack, op_len);
    }

    // While we have an argument and we have still text to parse
    while (i_pos && pos < len_text) {
        if (BLITZ_IS_ARG_EXPR(i_type)) {
            // Argument is an operator
            if (i_type == BLITZ_EXPR_OPERATOR_LP) {
                // Left bracket, just add it to the stack
                brace_depth++;
                BLITZ_IF_STACK_PUSH(op_stack, op_len, c, i_len, i_type, *error_out);
                if (*error_out) {
                    return;
                }
            } else if (i_type == BLITZ_EXPR_OPERATOR_METHOD) {
                if (!method_is_first) {
                    *error_out = BLITZ_CALL_ERROR_IF_METHOD_CALL_TOO_COMPLEX;
                    return;
                }
                have_method_call = 1;
                // Method, just add it to the stack
                BLITZ_IF_STACK_PUSH(op_stack, op_len, c, i_len, i_type, *error_out);
                if (*error_out) {
                    return;
                }
            } else if (i_type == BLITZ_EXPR_OPERATOR_COMMA) {
                *error_out = BLITZ_CALL_ERROR_IF_MISSING_BRACKETS;
                args_since_bracket = 0;
                /*
                 Until the topmost element of the stack is a left parenthesis, pop the element onto the output queue.
                 If no left parentheses are encountered, either the separator was misplaced or parentheses were mismatched.
                 */
                if (BLITZ_DEBUG) php_printf("Got comma\n");
                while (op_len >= 0) {
                    if (BLITZ_DEBUG) php_printf("On stack: %d\n", op_stack[op_len].type);
                    if (op_stack[op_len].type == BLITZ_EXPR_OPERATOR_LP) {
                        *error_out = 0;
                        break;
                    }
                    ADD_CALL_ARGS(node, op_stack[op_len].name, op_stack[op_len].len, op_stack[op_len].type);
                    ++node->n_if_args;
                    --op_len;
                }
                if (*error_out) {
                    return;
                }

            } else if (i_type != BLITZ_EXPR_OPERATOR_RP) { // Not a right bracket
                // Add all the previous operations
                if (BLITZ_DEBUG) {
                    php_printf(
                        "Checking operator %s (%d) vs top stack (%d elements on stack): %s (%d)\n",
                        BLITZ_OPERATOR_TO_STRING(i_type),
                        BLITZ_OPERATOR_GET_PRECEDENCE(i_type),
                        op_len + 1,
                        op_len >= 0 ?
                            BLITZ_OPERATOR_TO_STRING(op_stack[op_len].type) :
                            "-null-",
                        op_len >= 0 ? BLITZ_OPERATOR_GET_PRECEDENCE(op_stack[op_len].type) : -1
                    );
                }

                while (op_len >= 0 &&
                        op_stack[op_len].type != BLITZ_EXPR_OPERATOR_LP &&
                        BLITZ_OPERATOR_HAS_LOWER_PRECEDENCE(i_type, op_stack[op_len].type)) {
                    if (BLITZ_DEBUG) {
                        php_printf(
                            "--> op %s has lower precedence (%d vs %d) over %s\n",
                            BLITZ_OPERATOR_TO_STRING(i_type),
                            BLITZ_OPERATOR_GET_PRECEDENCE(i_type),
                            BLITZ_OPERATOR_GET_PRECEDENCE(op_stack[op_len].type),
                            BLITZ_OPERATOR_TO_STRING(op_stack[op_len].type)
                        );
                    }

                    // Check if we have sufficient args on the stack
                    if (args_on_list < BLITZ_OPERATOR_GET_NUM_OPERANDS(op_stack[op_len].type)) {
                        // Possibly not enough arguments to execute this operator
                        *error_out = BLITZ_CALL_ERROR_IF_NOT_ENOUGH_OPERANDS;
                        return;
                    }

                    args_on_list = args_on_list - BLITZ_OPERATOR_GET_NUM_OPERANDS(op_stack[op_len].type) + 1; // produces one result

                    ADD_CALL_ARGS(node, op_stack[op_len].name, op_stack[op_len].len, op_stack[op_len].type);
                    if (BLITZ_DEBUG) dump_call_args(node);
                    ++node->n_if_args;
                    --op_len;

                    if (GET_CALL_ARGS_SIZE(node) >= BLITZ_IF_STACK_MAX) {
                        *error_out = BLITZ_CALL_ERROR_IF_TOO_COMPLEX;
                        return;
                    }
                }

                // Push operator to the stack
                BLITZ_IF_STACK_PUSH(op_stack, op_len, c, i_len, i_type, *error_out);
                if (BLITZ_DEBUG) dump_if_stack(op_stack, op_len);
                if (*error_out) {
                    return;
                }
            } else { // Right bracket
                brace_depth--;
                // Check if it was immediately preceded by a left bracket (= invalid syntax)
                if (!have_method_call && args_since_bracket == 0 && op_stack[op_len].type == BLITZ_EXPR_OPERATOR_LP) {
                    if (BLITZ_DEBUG) {
                        php_printf(
                            "IF expression: empty expression in brackets: stack (%u - %u - %u)\n",
                            op_len,
                            args_on_list,
                            args_since_bracket
                        );
                    }
                    *error_out = (args_since_bracket > 0 ? BLITZ_CALL_ERROR_IF_MISSING_BRACKETS : BLITZ_CALL_ERROR_IF_EMPTY_EXPRESSION);
                    return;
                }

                // While the top of the stack is not a left bracket
                while (op_len >= 0 && op_stack[op_len].type != BLITZ_EXPR_OPERATOR_LP) {

                    // Check if we have sufficient args on the stack
                    if (args_on_list < BLITZ_OPERATOR_GET_NUM_OPERANDS(op_stack[op_len].type)) {
                        // Possibly not enough arguments to execute this operator
                        *error_out = BLITZ_CALL_ERROR_IF_NOT_ENOUGH_OPERANDS;
                        return;
                    }

                    args_on_list = args_on_list - BLITZ_OPERATOR_GET_NUM_OPERANDS(op_stack[op_len].type) + 1; // produces one result

                    ADD_CALL_ARGS(node, op_stack[op_len].name, op_stack[op_len].len, op_stack[op_len].type);
                    if (BLITZ_DEBUG) dump_call_args(node);
                    ++node->n_if_args;
                    --op_len;

                    if (GET_CALL_ARGS_SIZE(node) >= BLITZ_IF_STACK_MAX) {
                        *error_out = BLITZ_CALL_ERROR_IF_TOO_COMPLEX;
                        return;
                    }
                }

                if (op_len < 0 || op_stack[op_len].type != BLITZ_EXPR_OPERATOR_LP) {
                    if (BLITZ_DEBUG) php_printf("IF expression: missing left bracket: stack (%u), mismatch?\n", op_len);
                    *error_out = BLITZ_CALL_ERROR_IF_MISSING_BRACKETS;
                    return;
                }

                // Discard left paren
                op_len--;

                if (op_len >= 0 && op_stack[op_len].type == BLITZ_EXPR_OPERATOR_METHOD) {
                    ADD_CALL_ARGS(node, op_stack[op_len].name, op_stack[op_len].len, op_stack[op_len].type);
                    ++node->n_if_args;
                    --op_len;
                }
            }
        } else {
            // operand
            ADD_CALL_ARGS(node, buf, i_len, i_type);
            if (BLITZ_DEBUG) dump_call_args(node);
            ++node->n_if_args;
            args_on_list++;
            args_since_bracket++;

            if (GET_CALL_ARGS_SIZE(node) >= BLITZ_IF_STACK_MAX) {
                *error_out = BLITZ_CALL_ERROR_IF_TOO_COMPLEX;
                return;
            }
        }

        pos += i_pos;
        c = text + pos;

        BLITZ_SKIP_BLANK(c,i_pos,pos);

        if (i_type == BLITZ_EXPR_OPERATOR_RP && have_method_call && brace_depth == 0) {
            // If we have parsed ")" and we had a method call, stop parsing
            // because we only support expressions like "{{IF callback(...)}}"
            break;
        }

        if (pos >= len_text) {
            // If we're at the end of our text, break and don't do the blitz_parse_arg call again (just optimalization)
            break;
        }

        method_is_first = 0;

        // Check if we can scan another argument (and loop again)
        i_len = i_pos = i_type = 0;
        blitz_parse_arg(c, var_prefix, buf, &i_type, &i_len, &i_pos, (brace_depth == 1));
    }

    // Finished parsing all arguments, now do some closing, and finish the parsing tree
    while (op_len >= 0 && op_stack[op_len].type != BLITZ_EXPR_OPERATOR_LP) {

        // Check if we have sufficient args on the stack
        if (args_on_list < BLITZ_OPERATOR_GET_NUM_OPERANDS(op_stack[op_len].type)) {
            // Possibly not enough arguments to execute this operator

            *error_out = BLITZ_CALL_ERROR_IF_NOT_ENOUGH_OPERANDS;
            return;
        }

        args_on_list = args_on_list - BLITZ_OPERATOR_GET_NUM_OPERANDS(op_stack[op_len].type) + 1; // produces one result

        ADD_CALL_ARGS(node, op_stack[op_len].name, op_stack[op_len].len, op_stack[op_len].type);
        if (BLITZ_DEBUG) dump_call_args(node);
        ++node->n_if_args;
        --op_len;

        if (GET_CALL_ARGS_SIZE(node) >= BLITZ_IF_STACK_MAX) {
            *error_out = BLITZ_CALL_ERROR_IF_TOO_COMPLEX;
            return;
        }
    }

    if (op_len != 0 || (!have_method_call && args_on_list != 1)) {
        if (BLITZ_DEBUG) {
            php_printf(
                "IF expression: missing %s at the begin of stack, mismatch?\n",
                (op_len < 0 ? "left bracket" : args_on_list == 1 ? "right bracket" : "operand")
            );
        }
        *error_out = BLITZ_CALL_ERROR_IF_MISSING_BRACKETS;
        return;
    }

    *pos_out = pos + i_pos;

    // We finished parsing all arguments, in RPN!!! Remember that IF a <operator> b became a b <operator>
    if (BLITZ_DEBUG) {
        php_printf("Successfully parsed if: dumping...\n");
        dump_call_args(node);
    }
    return;
}
/* }}} */

/* {{{ void blitz_parse_scope() */
static inline void blitz_parse_scope (char *text, unsigned int len_text, blitz_node *node, char var_prefix,
    unsigned int *pos_out, char *error_out)
{
    char *c = text;
    unsigned int pos = 0, i_pos = 0, i_len = 0, key_len = 0;
    char key[BLITZ_MAX_LEXEM_LEN];
    char buf[BLITZ_MAX_LEXEM_LEN];
    unsigned char i_type = 0;
    int has_key = 0;
    call_arg *i_arg = NULL;

    *error_out = 0;
    *pos_out = 0;

    if (BLITZ_DEBUG) {
        char *tmp = estrndup((char *)text, len_text + 1);
        tmp[len_text] = '\x0';
        php_printf("*** FUNCTION *** blitz_parse_scope, started at pos=%u, c=%c, len=%u\n", pos, *c, len_text);
        php_printf("text: %s\n", tmp);
        efree(tmp);
    }

    i_pos = has_key = 0;

    // While we have an argument and we have still text to parse
    while (pos < len_text && *c != ')') {
        pos += i_pos;
        c = text + pos;

        BLITZ_SKIP_BLANK(c,i_pos,pos);
        if (pos >= len_text) {
            // If we're at the end of our text, break and don't do the blitz_parse_arg call again (just optimalization)
            break;
        }

        if (*c == ')') {
            break;
        } else if (has_key == 0 && *c == ',') {
            i_pos = 1;
            continue; // Okay!
        } else if (has_key == 1 && *c == '=') {
            i_pos = 1;
            continue; // Okay!
        } else if (has_key == 1) {
            i_len = i_pos = i_type = 0;
            blitz_parse_arg(c, var_prefix, buf, &i_type, &i_len, &i_pos, 0);
            if (!i_pos || BLITZ_IS_ARG_EXPR(i_type)) {
                *error_out = BLITZ_CALL_ERROR_SCOPE;
                return;
            }

            // Add it to the zval
            ADD_CALL_ARGS(node, key, key_len, BLITZ_ARG_TYPE_VAR)
            ADD_CALL_ARGS(node, buf, i_len, i_type)

            has_key = 0;
        } else {
            i_len = i_pos = i_type = 0;
            blitz_parse_arg(c, var_prefix, key, &i_type, &i_len, &i_pos, 0);
            if (!i_pos || i_type != BLITZ_ARG_TYPE_VAR) {
                *error_out = BLITZ_CALL_ERROR_SCOPE;
                return;
            }
            key_len = i_len;
            has_key = 1;
        }
    }

    *pos_out = pos;

    // We finished parsing all arguments, in RPN!!! Remember that IF a <operator> b became a b <operator>
    return;
}
/* }}} */

/* {{{ void blitz_parse_call() */
static inline void blitz_parse_call (char *text, unsigned int len_text, blitz_node *node,
    unsigned int *true_lexem_len, char var_prefix, char *error)
{
    char *c = text;
    char *p = NULL;
    char symb = 0, i_symb = 0, is_path = 0;
    char state = BLITZ_CALL_STATE_ERROR;
    char ok = 0;
    unsigned int pos = 0, i_pos = 0, i_len = 0;
    char buf[BLITZ_MAX_LEXEM_LEN];
    unsigned char i_type = 0;
    call_arg *i_arg = NULL;
    char *p_end = NULL;
    char has_namespace = 0;
    register unsigned char shift = 0, i = 0, j = 0;

    BLITZ_SKIP_BLANK(c,i_pos,pos);

    if (BLITZ_DEBUG) {
        char *tmp = estrndup((char *)text, len_text + 1);
        tmp[len_text] = '\x0';
        php_printf("*** FUNCTION *** blitz_parse_call, started at pos=%u, c=%c, len=%u\n", pos, *c, len_text);
        php_printf("text: %s\n", tmp);
        efree(tmp);
    }

    /* init node */
    blitz_free_node(node);
    node->type = BLITZ_TYPE_METHOD;
    node->escape_mode = BLITZ_ESCAPE_DEFAULT;

    *true_lexem_len = 0; /* used for parameters only */

    i_pos = 0;
    if (var_prefix && *c == var_prefix) { /* scan prefixed parameter */
        ++c;
        ++pos;
        BLITZ_SCAN_VAR_NOCOPY(c, i_pos, i_symb, is_path);
        if (i_pos != 0) {
            memcpy(node->lexem, text + pos, i_pos);
            node->lexem_len = i_pos;
            node->lexem[i_pos] = '\x0';
            *true_lexem_len = i_pos;
            if (is_path) {
                node->type = BLITZ_NODE_TYPE_VAR_PATH;
            } else {
                node->type = BLITZ_NODE_TYPE_VAR;
            }
            state = BLITZ_CALL_STATE_FINISHED;
            pos += i_pos;
        }
    } else if (BLITZ_IS_ALPHA(*c) || (*c == '\\')) { /* scan method/callback (can go with a namespace) or unprefixed parameter */
        is_path = 0;
        if (*c == '\\') {
            BLITZ_SCAN_NAMESPACE_NOCOPY(c, i_pos, i_symb, is_path);
        } else {
            BLITZ_SCAN_VAR_NOCOPY(c, i_pos, i_symb, is_path);
        }

        if (i_pos > 0) {
            if (*c == '\\') { // Space\Class with no leading "\"
                BLITZ_SCAN_NAMESPACE_NOCOPY(c, i_pos, i_symb, is_path);
            }

            p_end = text + pos + i_pos;
            if (*p_end == ':' && *(p_end + 1) == ':') { // :: separates helper classname/namespace path and the callback name
                has_namespace = 1;
                i_pos += 2;
                c = text + pos + i_pos;
                BLITZ_SCAN_VAR_NOCOPY(c, i_pos, i_symb, is_path);
            }

            memcpy(node->lexem, text + pos, i_pos);
            node->lexem_len = i_pos;
            node->lexem[i_pos] = '\x0';
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
                    node->namespace_code = BLITZ_NODE_CUSTOM_NAMESPACE;
                }
                if (shift) {
                    i = 0;
                    j = shift;
                    while (i < node->lexem_len) {
                        node->lexem[i++] = node->lexem[j++];
                    }
                    node->lexem_len = node->lexem_len - shift;
                    node->lexem[node->lexem_len] = '\x0';
                }
                if (BLITZ_DEBUG) php_printf("REDUCED STATIC CALL: %s, len=%lu, namespace_code=%u\n",
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
                    INIT_CALL_ARGS(node);
                    state = BLITZ_CALL_STATE_NEXT_ARG;

                    /* predefined method? */
                    if (has_namespace == 0) {
                        if (BLITZ_STRING_IS_IF(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_IF;
                            state = BLITZ_CALL_STATE_NEXT_ARG_IF;
                        } else if (BLITZ_STRING_IS_UNLESS(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_UNLESS;
                            state = BLITZ_CALL_STATE_NEXT_ARG_IF;
                        } else if (BLITZ_STRING_IS_INCLUDE(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_INCLUDE;
                        } else if (BLITZ_STRING_IS_ESCAPE(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_WRAPPER_ESCAPE;
                        } else if (BLITZ_STRING_IS_DATE(node->lexem, node->lexem_len)) {
                            node->type = BLITZ_NODE_TYPE_WRAPPER_DATE;
                        }
                    }
                }

                if (BLITZ_G(lower_case_method_names)) {
                    zend_str_tolower(node->lexem, node->lexem_len);
                }

            } else {
                ok = 1;
                if (BLITZ_STRING_IS_BEGIN(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS(node);
                    state = BLITZ_CALL_STATE_BEGIN;
                    node->type = BLITZ_NODE_TYPE_BEGIN;
                } else if (BLITZ_STRING_IS_LITERAL(node->lexem, node->lexem_len)) {
                    state = BLITZ_CALL_STATE_LITERAL;
                    node->type = BLITZ_NODE_TYPE_LITERAL;
                } else if (BLITZ_STRING_IS_IF(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS(node);
                    state = BLITZ_CALL_STATE_IF;
                    node->type = BLITZ_NODE_TYPE_IF_NF;
                } else if (BLITZ_STRING_IS_UNLESS(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS(node);
                    state = BLITZ_CALL_STATE_IF;
                    node->type = BLITZ_NODE_TYPE_UNLESS_NF;
                } else if (BLITZ_STRING_IS_ELSEIF(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS(node);
                    state = BLITZ_CALL_STATE_IF;
                    node->type = BLITZ_NODE_TYPE_ELSEIF_NF;
                } else if (BLITZ_STRING_IS_ELSE(node->lexem, node->lexem_len)) {
                    INIT_CALL_ARGS(node);
                    state = BLITZ_CALL_STATE_ELSE;
                    node->type = BLITZ_NODE_TYPE_ELSE_NF;
                } else if (BLITZ_STRING_IS_END(node->lexem, node->lexem_len)) {
                    state = BLITZ_CALL_STATE_END;
                    node->type = BLITZ_NODE_TYPE_END;
                } else {
                    /* functions without brackets are treated as parameters */
                    if (BLITZ_NOBRAKET_FUNCTIONS_ARE_VARS) {
                        node->type = is_path ? BLITZ_NODE_TYPE_VAR_PATH : BLITZ_NODE_TYPE_VAR;
                    } else if (BLITZ_G(lower_case_method_names)) {
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
                    BLITZ_SCAN_VAR(c,p,i_len,i_symb,is_path);
                    if (i_len!=0) {
                        ok = 1;
                        pos += i_len;
                        ADD_CALL_ARGS(node, buf, i_len, is_path ? BLITZ_ARG_TYPE_VAR_PATH : i_type);
                        state = BLITZ_CALL_STATE_FINISHED;
                    } else {
                        state = BLITZ_CALL_STATE_ERROR;
                    }
                    break;
                case BLITZ_CALL_STATE_LITERAL:
                    i_pos = 0;
                    BLITZ_SKIP_BLANK(c,i_pos,pos); i_pos = 0; symb = *c;
                    if (BLITZ_IS_ALPHA(symb)) {
                        BLITZ_SCAN_VAR(c,p,i_pos,i_symb,is_path); pos += i_pos; i_pos = 0;
                    }
                    state = BLITZ_CALL_STATE_FINISHED;
                    break;
                case BLITZ_CALL_STATE_END:
                    i_pos = 0;
                    BLITZ_SKIP_BLANK(c,i_pos,pos); i_pos = 0; symb = *c;
                    if (BLITZ_IS_ALPHA(symb)) {
                        BLITZ_SCAN_VAR(c,p,i_pos,i_symb,is_path); pos += i_pos; i_pos = 0;
                    }
                    state = BLITZ_CALL_STATE_FINISHED;
                    break;
               case BLITZ_CALL_STATE_IF:
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    is_path = i_len = i_pos = i_type = 0;

                    blitz_parse_if(c, len_text-pos, node, var_prefix, &i_pos, error);
                    if (*error != 0) {
                        return;
                    }
                    pos += i_pos; i_pos = 0;
                    state = BLITZ_CALL_STATE_FINISHED;
                    break;
                case BLITZ_CALL_STATE_ELSE:
                    i_pos = 0;
                    BLITZ_SKIP_BLANK(c,i_pos,pos); i_pos = 0; symb = *c;
                    if (BLITZ_IS_ALPHA(symb)) {
                        BLITZ_SCAN_ALNUM(c,p,i_pos,i_symb); pos += i_pos; i_pos = 0;
                    }
                    state = BLITZ_CALL_STATE_FINISHED;
                    break;
                case BLITZ_CALL_STATE_NEXT_ARG_IF:
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    is_path = i_len = i_pos = i_type = 0;

                    blitz_parse_if(c, len_text-pos, node, var_prefix, &i_pos, error);
                    if (*error != 0) {
                        return;
                    }
                    pos += i_pos; i_pos = 0;
                    c = text + pos;
                    state = BLITZ_CALL_STATE_HAS_NEXT;
                    break;
                case BLITZ_CALL_STATE_NEXT_ARG_SCOPE:
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    is_path = i_len = i_pos = i_type = 0;

                    blitz_parse_scope(c, len_text-pos, node, var_prefix, &i_pos, error);
                    if (*error != 0) {
                        return;
                    }
                    pos += i_pos; i_pos = 0;
                    c = text + pos;
                    state = BLITZ_CALL_STATE_HAS_NEXT;
                    break;
                case BLITZ_CALL_STATE_NEXT_ARG:
                    blitz_parse_arg(c, var_prefix, buf, &i_type, &i_len, &i_pos, 1);
                    if (i_pos && !BLITZ_IS_ARG_EXPR(i_type)) {
                        pos += i_pos;
                        c = text + pos;
                        ADD_CALL_ARGS(node, buf, i_len, i_type);
                        state = BLITZ_CALL_STATE_HAS_NEXT;
                    } else {
                        state = BLITZ_CALL_STATE_ERROR;
                    }
                    break;
                case BLITZ_CALL_STATE_HAS_NEXT:
                    BLITZ_SKIP_BLANK(c,i_pos,pos);
                    symb = *c;
                    if (symb == ',') {
                        // Include supports a scope, so if node type is include, read scope
                        state = (node->type == BLITZ_NODE_TYPE_INCLUDE ? BLITZ_CALL_STATE_NEXT_ARG_SCOPE : BLITZ_CALL_STATE_NEXT_ARG);
                        ++c; ++pos;
                    } else if (symb == ')') {
                        state = BLITZ_CALL_STATE_FINISHED;
                        ++c; ++pos;
                    } else {
                        state = BLITZ_CALL_STATE_ERROR;
                    }
                    break;
                default:
                    ok = 0;
                    break;
            }
        }
    }

    if (state == BLITZ_CALL_STATE_FINISHED) {
        BLITZ_SKIP_BLANK(c, i_pos, pos);
        if (*c == ';') {
            ++c; ++pos;
        } else if (*c == '|') {
            ++c; ++pos;
            BLITZ_SKIP_BLANK(c, i_pos, pos);
            i_pos = 0;
            p = buf;
            BLITZ_SCAN_ALNUM(c, p, i_pos, i_symb);

            // dirty hack to support "raw" or "escape" filters
            // FIXME: change the parser so we can use something generic like {{ $var | bla("params", $p) }}
            if (i_pos == 3 && buf[0] == 'r' && buf[1] == 'a' && buf[2] == 'w') {
                node->escape_mode = BLITZ_ESCAPE_NO;
            } else if (i_pos == 6 && buf[0] == 'e' && buf[1] == 's' && buf[2] == 'c' && buf[3] == 'a' && buf[4] == 'p' && buf[5] == 'e') {
                node->escape_mode = BLITZ_ESCAPE_YES;
            } else if (i_pos == 5 && buf[0] == 'n' && buf[1] == 'l' && buf[2] == '2' && buf[3] == 'b' && buf[4] == 'r') {
                node->escape_mode = BLITZ_ESCAPE_NL2BR;
            }
            pos += i_pos;
        }
        BLITZ_SKIP_BLANK(c, i_pos, pos);
        if (pos < len_text) { /* when close tag contains whitespaces SKIP_BLANK will make pos>=len_text */
            if (BLITZ_DEBUG) php_printf("%u <> %u => error state\n", pos, len_text);
            state = BLITZ_CALL_STATE_ERROR;
        }
    }

    if (state == BLITZ_CALL_STATE_ERROR && (node->type == BLITZ_NODE_TYPE_IF_NF || node->type == BLITZ_NODE_TYPE_UNLESS_NF || node->type == BLITZ_NODE_TYPE_ELSEIF_NF)) {
        *error = BLITZ_CALL_ERROR_IF_CONTEXT;
    } else if (node->type == BLITZ_NODE_TYPE_INCLUDE && (node->n_args < 1 || (node->n_args % 2 != 1) || state == BLITZ_CALL_STATE_ERROR)) {
        *error = BLITZ_CALL_ERROR_INCLUDE;
    } else if (state != BLITZ_CALL_STATE_FINISHED) {
        *error = BLITZ_CALL_ERROR;
    } else if ((node->type == BLITZ_NODE_TYPE_IF || node->type == BLITZ_NODE_TYPE_UNLESS) && ((node->n_args - node->n_if_args) < 1 || (node->n_args - node->n_if_args) > 2)) {
        *error = BLITZ_CALL_ERROR_IF;
    }

    /* FIXME: check arguments for wrappers (escape, date, trim) */
}
/* }}} */

static unsigned long get_line_number(smart_str *str, unsigned long pos) { /* {{{ */
    register const char *p = ZSTR_VAL(str->s);
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

static unsigned long get_line_pos(const smart_str *str, unsigned long pos) { /* {{{ */
    register const char *p = ZSTR_VAL(str->s);
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
static inline void blitz_analizer_warn_unexpected_tag (blitz_tpl *tpl, unsigned char tag_id, unsigned int pos) {
    char *human_tag_name[BLITZ_TAG_LIST_LEN] = {
        "TAG_OPEN", "TAG_OPEN_ALT", "TAG_CLOSE", "TAG_CLOSE_ALT",
        "TAG_OPEN_COMMENT", "TAG_CLOSE_COMMENT"
    };
    char *template_name = tpl->static_data.name;

    blitz_error(tpl, E_WARNING,
        "SYNTAX ERROR: unexpected %s (%s: line %lu, pos %lu)",
        human_tag_name[tag_id], template_name,
        get_line_number(&tpl->static_data.body, pos), get_line_pos(&tpl->static_data.body, pos)
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
static inline int blitz_analizer_create_parent(analizer_ctx *ctx, unsigned int get_next_node)
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
    i_node->lexem[i_node->lexem_len] = '\x0';

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
        blitz_error(NULL, E_WARNING,
            "INTERNAL ERROR: analizer stack length (%u) was exceeded when parsing template (%s: line %lu, pos %lu), recompile blitz with different BLITZ_ANALIZER_NODE_STACK_LEN or just don't use so complex templates", BLITZ_ANALIZER_NODE_STACK_LEN, ctx->tpl->static_data.name,
            get_line_number(&ctx->tpl->static_data.body, current_open), get_line_pos(&ctx->tpl->static_data.body, current_open)
        );
    }

    if (BLITZ_NODE_TYPE_LITERAL == i_node->type) {
        ctx->is_literal = 1;
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
static inline void blitz_analizer_finalize_parent(analizer_ctx *ctx, unsigned int attach)
{
    analizer_stack_elem *stack_head = NULL;
    blitz_node *i_node = ctx->node, *parent = NULL;
    blitz_tpl *tpl = NULL;
    unsigned long current_open = ctx->current_open;
    unsigned long current_close = ctx->current_close;
    unsigned long close_len = ctx->close_len;

    if (BLITZ_DEBUG)
        php_printf("*** FUNCTION *** blitz_analizer_finalize_parent: node = %s\n", i_node->lexem);

    /* end: remove node from stack, finalize node: set true close positions and new type */
    if (ctx->node_stack_len) {
        stack_head = ctx->node_stack + ctx->node_stack_len;

        /**************************************************************************/
        /* parent BEGIN/LITERAL/IF/ELSEIF/ELSE becomes:                           */
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

        if (BLITZ_NODE_TYPE_LITERAL == parent->type) {
            ctx->is_literal = 0;
        }

        --(ctx->node_stack_len);
        ++(ctx->n_nodes);

        if (BLITZ_DEBUG) blitz_dump_node_stack(ctx);

    } else {
        /* error: end with no begin */
        i_node->hidden = 1;
        tpl = ctx->tpl;
        blitz_error(tpl, E_WARNING,
            "SYNTAX ERROR: end with no begin (%s: line %lu, pos %lu)",
            tpl->static_data.name, get_line_number(&tpl->static_data.body, current_open), get_line_pos(&tpl->static_data.body, current_open)
        );
    }

    return;
}
/* }}} */

/* {{{ int blitz_analizer_add */
static inline int blitz_analizer_add(analizer_ctx *ctx) {
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
    unsigned char is_alt_tag = 0;
    unsigned char is_comment_tag = 0;
    smart_str *body = NULL;
    analizer_stack_elem *stack_head = NULL;

    tag = ctx->tag;
    tpl = ctx->tpl;
    body = &tpl->static_data.body;
    current_close = tag->pos;
    current_open = ctx->pos_open;

    if (tag->tag_id == BLITZ_TAG_ID_CLOSE_ALT) {
        is_alt_tag = 1;
    } else if (tag->tag_id == BLITZ_TAG_ID_COMMENT_CLOSE) {
        is_comment_tag = 1;
    }

    if (is_alt_tag) {
        open_len = tpl->static_data.tag_open_alt_len;
        close_len = tpl->static_data.tag_close_alt_len;
    } else if (is_comment_tag) {
        open_len = tpl->static_data.tag_comment_open_len;
        close_len = tpl->static_data.tag_comment_close_len;
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
        /* comments fix: CLOSE_ALT by default can be HTML-comment (dirty check), COMMENT_CLOSE - to enable large piece of code commented */
        if ((tag->tag_id != BLITZ_TAG_ID_CLOSE_ALT) && (tag->tag_id != BLITZ_TAG_ID_COMMENT_CLOSE)) { /* comments fix */
            blitz_error(tpl, E_WARNING,
                "SYNTAX ERROR: lexem is too long (%s: line %lu, pos %lu)",
                tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
            );
        }
        return 1;
    }

    if (lexem_len <= 0 && (tag->tag_id != BLITZ_TAG_ID_CLOSE_ALT) && (tag->tag_id != BLITZ_TAG_ID_COMMENT_CLOSE)) {
        blitz_error(tpl, E_WARNING,
            "SYNTAX ERROR: zero length lexem (%s: line %lu, pos %lu)",
            tpl->static_data.name,
            get_line_number(body, current_open), get_line_pos(body, current_open)
        );
        return 1;
    }

    i_node->args = NULL;
    i_node->n_args = 0;
    i_node->n_if_args = 0;
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
    plex = ZSTR_VAL(tpl->static_data.body.s) + shift;
    true_lexem_len = 0;
    is_alt_tag = (tag->tag_id == BLITZ_TAG_ID_CLOSE_ALT) ? 1 : 0;
    blitz_parse_call(plex, lexem_len, i_node, &true_lexem_len, BLITZ_G(var_prefix), &i_error);

    if (BLITZ_DEBUG)
        php_printf("parsed lexem: %s\n", i_node->lexem);

    // just do nothing inside literal blocks, only wait for {{ end }}
    if (ctx->is_literal) {
        if (i_node->type != BLITZ_NODE_TYPE_END) {
            blitz_free_node(i_node);
            return 1;
        }
    }

    if (i_error) {
        if (is_alt_tag) return 1; /* alternative tags can be just HTML comment tags */
        if (!ctx->is_literal) { /* suppress errors and don't hide anything inside literal blocks */
            i_node->hidden = 1;
            if (i_error == BLITZ_CALL_ERROR) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid method call (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_IF) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid <if> syntax, only 2 or 3 arguments allowed (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_INCLUDE) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid <include> syntax, first param must be filename and there can be optional scope arguments (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_SCOPE) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid scope syntax, to define scope use key1 = value1, key2 = value2, ... while key is a literal var without quotes (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_IF_CONTEXT) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid <if> syntax, probably a bracket mismatch but could be wrong operands too (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_IF_MISSING_BRACKETS) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid <if> syntax, bracket mismatch (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_IF_NOT_ENOUGH_OPERANDS) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid <if> syntax, wrong number of operands (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_IF_EMPTY_EXPRESSION) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid <if> syntax, empty expression between the brackets (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_IF_TOO_COMPLEX) {
                blitz_error(tpl, E_WARNING,
                    "SYNTAX ERROR: invalid <if> syntax, the expression is too complex, consider raising BLITZ_IF_STACK_MAX=%d (%s: line %lu, pos %lu)",
                    BLITZ_IF_STACK_MAX, tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            } else if (i_error == BLITZ_CALL_ERROR_IF_METHOD_CALL_TOO_COMPLEX) {
                blitz_error(tpl, E_WARNING,
                    "Method calls in IF statements are only supported in form '{{IF callback(...)}}'. "\
                    "You cannot use method call results in statements or use nested method calls (%s: line %lu, pos %lu)",
                    tpl->static_data.name, get_line_number(body, current_open), get_line_pos(body, current_open)
                );
            }
        }
    } else {
        i_node->hidden = 0;
    }

    ctx->node = i_node;
    ctx->current_open = current_open;
    ctx->current_close = current_close;
    ctx->close_len = close_len;
    ctx->true_lexem_len = true_lexem_len;

    if (i_node->type == BLITZ_NODE_TYPE_BEGIN || i_node->type == BLITZ_NODE_TYPE_LITERAL || i_node->type == BLITZ_NODE_TYPE_IF_NF || i_node->type == BLITZ_NODE_TYPE_UNLESS_NF) {
        if (!blitz_analizer_create_parent(ctx, 1)) {
            return 0;
        }
        ctx->n_needs_end++;
    } else if (i_node->type == BLITZ_NODE_TYPE_END) {
        blitz_analizer_finalize_parent(ctx, 1);
        ctx->n_actual_end++;
    } else if (i_node->type == BLITZ_NODE_TYPE_ELSEIF_NF || i_node->type == BLITZ_NODE_TYPE_ELSE_NF) {
        blitz_analizer_finalize_parent(ctx, 0);
        if (!blitz_analizer_create_parent(ctx, 0)) {
            return 0;
        }
    } else { /* just a var- or call-node */
        i_node->pos_begin = current_open;
        i_node->pos_end = current_close + close_len;
        i_node->lexem_len = true_lexem_len;
        i_node->lexem[true_lexem_len] = '\x0';

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
static inline void blitz_analizer_process_node (analizer_ctx *ctx, unsigned int is_last) {
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
            if (!ctx->is_literal) {
                blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->tag->tag_id, ctx->tag->pos);
            }
            break;

        case BLITZ_ANALISER_ACTION_ERROR_PREV:
            if (!ctx->is_literal) {
                blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->prev_tag->tag_id, ctx->prev_tag->pos);
            }
            break;

        case BLITZ_ANALISER_ACTION_ERROR_BOTH:
            if (!ctx->is_literal) {
                blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->tag->tag_id, ctx->tag->pos);
                blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->prev_tag->tag_id, ctx->prev_tag->pos);
            }
            break;

        case BLITZ_ANALISER_ACTION_ADD:
            if (!blitz_analizer_add(ctx)) return;
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

    if (is_last && state != BLITZ_ANALISER_STATE_NONE) {
        if (BLITZ_DEBUG) php_printf("is_last = %u, state = %u\n", is_last, state);
        if (!ctx->is_literal) {
            blitz_analizer_warn_unexpected_tag(ctx->tpl, ctx->tag->tag_id, ctx->tag->pos);
        }
    }

    if (ctx->tag)
        ctx->prev_tag = ctx->tag;

    return;
}
/* }}} */

static inline int blitz_analize (blitz_tpl *tpl) /* {{{ */
{
    unsigned int i = 0, is_last = 0;
    tag_pos *tags = NULL, *i_tag = NULL;
    blitz_list list_tag;
    blitz_string body_s;
    unsigned int tags_len = 0;
    unsigned int n_open = 0, n_close = 0;
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

    if (!ZSTR_LEN(tpl->static_data.body.s)) {
        return 0;
    }

    /* find all tag positions */
    list_tag.first = ecalloc(BLITZ_ALLOC_TAGS_STEP, sizeof(tag_pos));
    list_tag.size = 0;
    list_tag.item_size = sizeof(tag_pos);
    list_tag.allocated = BLITZ_ALLOC_TAGS_STEP;
    list_tag.end = list_tag.first;

    body_s.s = ZSTR_VAL(tpl->static_data.body.s);
    body_s.len = ZSTR_LEN(tpl->static_data.body.s);
    blitz_find_tag_positions(&body_s, &list_tag);

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
    ctx.n_needs_end = 0;
    ctx.n_actual_end = 0;
    ctx.is_literal = 0;

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
        blitz_analizer_process_node(&ctx, is_last);

        i_prev_state = ctx.state;
    }

    // Check if we had non-ended begin/if/unless.
    // Use ">" here instead of "!=" because end with no begin is checked in blitz_analizer_finalize_parent
    if (ctx.n_needs_end > ctx.n_actual_end) {
        blitz_error(tpl, E_WARNING, "SYNTAX ERROR: seems that you forgot to close some begin/if/unless nodes with end (%u opened vs %u end)", ctx.n_needs_end, ctx.n_actual_end);
    }

    tpl->static_data.n_nodes = ctx.n_nodes;
    if (BLITZ_G(remove_spaces_around_context_tags)) {
        blitz_remove_spaces_around_context_tags(tpl);
    }

    efree(list_tag.first);

    if (BLITZ_G(warn_context_duplicates)) {
        blitz_warn_context_duplicates(tpl);
    }

    return 1;
}
/* }}} */

/* {{{ void blitz_remove_spaces_around_context_tags(blitz_tpl *tpl) */
static inline void blitz_remove_spaces_around_context_tags(blitz_tpl *tpl) {
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
            && i_node->type != BLITZ_NODE_TYPE_LITERAL
            && i_node->type != BLITZ_NODE_TYPE_IF_CONTEXT
            && i_node->type != BLITZ_NODE_TYPE_UNLESS_CONTEXT
            && i_node->type != BLITZ_NODE_TYPE_ELSEIF_CONTEXT
            && i_node->type != BLITZ_NODE_TYPE_ELSE_CONTEXT)
        {
            continue;
        }

        if (BLITZ_DEBUG) {
            php_printf("lexem: %s, pos_begin = %lu, pos_begin_shift = %lu, pos_end = %lu, pos_end_shift = %lu\n",
                i_node->lexem, i_node->pos_begin, i_node->pos_begin_shift, i_node->pos_end, i_node->pos_end_shift);
        }

        // begin tag: scan back to the endline or any nonspace symbol
        shift = i_node->pos_begin;
        if (shift) shift--;
        pc = ZSTR_VAL(tpl->static_data.body.s) + shift;
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
            shift = ZSTR_LEN(tpl->static_data.body.s) - i_node->pos_begin_shift - 1;
            pc = ZSTR_VAL(tpl->static_data.body.s) + i_node->pos_begin_shift - 1;
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
                i_node->pos_begin_shift = ZSTR_LEN(tpl->static_data.body.s) - shift - 1;
                if (BLITZ_DEBUG) {
                    php_printf("new: pos_begin = %lu, pos_begin_shift = %lu\n", i_node->pos_begin, i_node->pos_begin_shift);
                }
            }
        }

        got_end_back = 0;
        got_end_fwd = 0;
        got_non_space = 0;

        // end tag: scan back to the endline or any nonspace symbol
        shift = i_node->pos_end_shift;
        pc = ZSTR_VAL(tpl->static_data.body.s) + shift;

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
            shift = ZSTR_LEN(tpl->static_data.body.s) - i_node->pos_end;
            pc = ZSTR_VAL(tpl->static_data.body.s) + i_node->pos_end - 1;
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
                i_node->pos_end = ZSTR_LEN(tpl->static_data.body.s) - shift;
                if (BLITZ_DEBUG) {
                    php_printf("new: pos_end = %lu, pos_end_shift = %lu\n", i_node->pos_end, i_node->pos_end_shift);
                }
            }
        }
    }
}
/* }}} */

/* {{{ int blitz_exec_wrapper() */
static inline int blitz_exec_wrapper(blitz_tpl *tpl, smart_str *result, unsigned long type, int args_num, char **args, unsigned int *args_len, char *tmp_buf)
{
    /* following wrappers are to be added: escape, date, gettext, something else?... */
    if (type == BLITZ_NODE_TYPE_WRAPPER_ESCAPE) {
        char *quote_str = args[1];
        long quote_style = ENT_QUOTES;
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
            blitz_error(tpl, E_WARNING,
                "escape format error (\"%s\"), available formats are ENT_QUOTES, ENT_COMPAT, ENT_NOQUOTES", quote_str
            );
            return 0;
        }

        result->s = php_escape_html_entities_ex((unsigned char *)args[0], args_len[0], all, quote_style, SG(default_charset), 1, 0);
        result->a = ZSTR_LEN(result->s);

    } else if (type == BLITZ_NODE_TYPE_WRAPPER_DATE) {
/* FIXME: check how it works under Windows */
        char *format = NULL;
        time_t timestamp = 0;
        struct tm *ta = NULL, tmbuf;
        int gm = 1; /* use localtime */
        char *p = NULL;
        int i = 0;
        size_t tmp_buf_len;

        /* check format */
        if (args_num>0) {
            format = args[0];
        }

        if (!format) {
            blitz_error(tpl, E_WARNING,
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
        tmp_buf_len = strftime(tmp_buf, BLITZ_TMP_BUF_MAX_LEN, format, ta);
        smart_str_appendl(result, tmp_buf, tmp_buf_len);
    }

    return 1;
}
/* }}} */

static inline int blitz_scope_stack_find_ind(blitz_tpl *tpl, char *key, unsigned long key_len, zval **zparam) /* {{{ */
{
    unsigned long i = 1;
    unsigned long lookup_limit = BLITZ_G(scope_lookup_limit);
    zval *data;

    if (tpl->scope_stack_pos <= lookup_limit)
        lookup_limit = tpl->scope_stack_pos - 1;

    while (i <= lookup_limit) {
        data = tpl->scope_stack[tpl->scope_stack_pos - i - 1];
        *zparam = blitz_hash_str_find_ind(HASH_OF(data), key, key_len);
        if (*zparam) {
            return i;
        }
        i++;
    }

    return 0;
}
/* }}} */

static inline unsigned int blitz_fetch_var_by_path(zval **zparam, const char *lexem, int lexem_len, zval *params, blitz_tpl *tpl) /* {{{ */
{
    register int i = 0, j = 0, last_pos = 0, key_len = 0, is_last = 0;
    char key[BLITZ_MAX_LEXEM_LEN];
    char root_found = 0;
    char use_scope = 0, found_in_scope = 0;
    int magic_offset = 0;
    unsigned long loop_index = 0, stack_level = 0, magic_stack = 0;

    *zparam = NULL;

    last_pos = 0;
    j = lexem_len - 1;
    magic_stack = tpl->scope_stack_pos;
    /* walk through the path */
    for (i = 0; i < lexem_len; i++, j--) {
        is_last = (j == 0);
        if (('.' == lexem[i]) || is_last) {
            key_len = i - last_pos + is_last;
            memcpy(key, lexem + last_pos, key_len);
            key[key_len] = '\x0';

            /* try to get data by the key */
            if (BLITZ_IS_PREDEFINED_TOP(key, key_len)) {
                if (BLITZ_DEBUG) php_printf("predefined path _top: resetting stack key '%s' in scope::%lu -> 0\n", key, magic_stack);
                magic_stack = 0;
                *zparam = tpl->scope_stack[magic_stack];
            } else if (BLITZ_IS_PREDEFINED_PARENT(key, key_len)) {
                magic_offset = (tpl->scope_stack_pos == magic_stack ? 2 : 1); /* if we're at the initial stack level, subtract 2 to get to the parent, since we just created a new stack level */
                if (BLITZ_DEBUG) php_printf("predefined path _parent: resetting stack key '%s' in scope::%lu -> %lu\n", key, magic_stack, (magic_stack > magic_offset ? magic_stack - magic_offset : 0));
                magic_stack = (magic_stack > magic_offset ? magic_stack - magic_offset : 0); /* keep track of the current magic scope to enable things like _parent._parent */
                *zparam = tpl->scope_stack[magic_stack];
            } else if (0 == root_found) { /* globals or params? */
                root_found = (params && (*zparam = blitz_hash_str_find_ind(HASH_OF(params), key, key_len)));
                if (!root_found) {
                    root_found = (tpl->hash_globals && (*zparam = blitz_hash_str_find_ind(tpl->hash_globals, key, key_len)));
                    if (!root_found) {
                        use_scope = BLITZ_G(scope_lookup_limit) && tpl->scope_stack_pos;
                        if (!use_scope) {
                            return 0;
                        }

                        stack_level = blitz_scope_stack_find_ind(tpl, key, key_len, zparam);
                        if (stack_level == 0) {
                            return 0;
                        }
                        found_in_scope = 1;
                    }
                }
            } else if (*zparam) { /* just propagate through elem */
                int predefined = -1;

                BLITZ_GET_PREDEFINED_VAR_EX(tpl, key, key_len, predefined, magic_stack);
                if (predefined >= 0) {
                    /* found a predefined var */
                    zval *item, tmp;

                    /* create zval to hold the predefined var value and store it in the stack to be destroyed later */
                    item = blitz_hash_str_find_ind(HASH_OF(*zparam), key, key_len);
                    if (!item) {
                        ZVAL_LONG(&tmp, predefined);
                        zend_hash_str_add(HASH_OF(*zparam), key, key_len, &tmp);
                        *zparam = blitz_hash_str_find_ind(HASH_OF(*zparam), key, key_len);
                    } else {
                        zval_ptr_dtor(item);
                        ZVAL_LONG(item, predefined);
                        *zparam = item;
                    }
                } else if (Z_TYPE_P(*zparam) == IS_ARRAY) {
                    zval *tmp;
                    tmp = blitz_hash_str_find_ind(HASH_OF(*zparam), key, key_len);
                    if (!tmp) {
                        // when using scope 'list.item' can be 1) list->item and 2) list->[loop_index]->item
                        // thus when list->item is not found whe have to test list->[loop_index]->item
                        if (found_in_scope == 0) {
                            return 0;
                        } else {
                            loop_index = tpl->loop_stack[tpl->loop_stack_level - stack_level + 1].current;
                            // move to list->[loop_index]
                            *zparam = blitz_hash_index_find(HASH_OF(*zparam), loop_index);
                            if (!*zparam)
                                return 0;

                            // try to find the key again
                            *zparam = blitz_hash_str_find_ind(HASH_OF(*zparam), key, key_len);
                            if (!*zparam)
                                return 0;
                            // when we found everything - decrease stack_level (we may have long loop path)
                            stack_level--;
                        }
                    } else {
                        *zparam = tmp;
                    }
                } else if (Z_TYPE_P(*zparam) == IS_OBJECT) {
                    zval *tmp;
                    tmp = blitz_hash_str_find_ind(HASH_OF(*zparam), key, key_len);
                    if (!tmp) {
                        return 0;
                    }
                    *zparam = tmp;
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
static inline int blitz_exec_predefined_method(blitz_tpl *tpl, blitz_node *node, zval *iteration_params, zval *parent_params, zval *id,
    smart_str *result, unsigned long *result_alloc_len, char *tmp_buf)
{
    zval *z;
    char is_var = 0, is_var_path = 0, is_found = 0;
    int res = 1;

    if (node->type == BLITZ_NODE_TYPE_IF || node->type == BLITZ_NODE_TYPE_UNLESS) {
        int is_true = 0, predefined = -1;
        unsigned int arg_offset = 1;
        call_arg *arg = NULL;

        // special check is required for if syntax with a single method call like '{{IF some_method()}}'
        if (node->n_if_args > 1 || (node->n_if_args == 1 && node->args[0].type == BLITZ_EXPR_OPERATOR_METHOD)) {
            blitz_check_expr(tpl, node, id, node->n_if_args, iteration_params, &is_true);
            /* complex if structures have as result there's a variable number of node args (say if you have if(a||b, "true", "false")),
             * node_args will be [ var b, var a, operator ||, literal "true", literal "false" ].
             * we need to know the position where the real arguments start.
            */
            arg_offset = node->n_if_args;
        } else {
            blitz_check_arg(tpl, node, iteration_params, &is_true);
        }

        if (node->type == BLITZ_NODE_TYPE_UNLESS) {
            if (is_true) {
                ++arg_offset;
                if (arg_offset >= node->n_args) {
                    return 1;  /* unless($a, 1) and $a is not empty */
                }
            }
        } else {
            if (!is_true) {
                ++arg_offset;
                if (arg_offset >= node->n_args) {
                    return 1; /* if($a, 1) and $a is empty */
                }
            }
        }

        arg = node->args + arg_offset;
        BLITZ_GET_PREDEFINED_VAR(tpl, arg->name, arg->len, predefined);
        if (predefined >= 0) {
            smart_str_append_unsigned(result, predefined);
            smart_str_0(result);
        } else {
            is_var = (arg->type == BLITZ_ARG_TYPE_VAR);
            is_var_path = (arg->type == BLITZ_ARG_TYPE_VAR_PATH);
            is_found = 0;
            if (is_var || is_var_path) { /* argument is variable  */
                if (is_var &&
                    (
                        (iteration_params && (z = blitz_hash_str_find_ind(HASH_OF(iteration_params), arg->name, arg->len)) != NULL)
                        || ((z = blitz_hash_str_find_ind(tpl->hash_globals, arg->name, arg->len)) != NULL)
                    ))
                {
                    is_found = 1;
                }
                else if (is_var_path
                    && blitz_fetch_var_by_path(&z, arg->name, arg->len, iteration_params, tpl))
                {
                    is_found = 1;
                }

                if (is_found) {
// FIXME: escape !!!
                    zend_string *str = zval_get_string(z);
                    smart_str_append_ex(result, str, 0);
                    smart_str_0(result);
                    zend_string_release(str);
                }
            } else { /* argument is string or number */
// FIXME: escape !!!
                smart_str_appendl(result, arg->name, arg->len);
                smart_str_0(result);
            }
        }
    } else if (node->type == BLITZ_NODE_TYPE_INCLUDE) {
        call_arg *arg = node->args, *tmp_arg = NULL;
        zval tmp_z;
        zval scope_iteration;
        char *filename = arg->name;
        unsigned int arg_type = arg->type;
        int filename_len = arg->len;
        smart_str inner_result = {NULL, 0};
        blitz_tpl *itpl = NULL;
        unsigned char i;
        int res = 0, found = 0, have_scope_iteration = 0;
        zend_string *zstr = NULL;

        if (!BLITZ_G(enable_include)) {
            blitz_error(tpl, E_WARNING,
                "includes are disabled by blitz.enable_include, line %lu, pos %lu",
                get_line_number(&tpl->static_data.body, node->pos_begin),
                get_line_pos(&tpl->static_data.body, node->pos_begin)
            );
            return 0;
        }

        if (arg_type != BLITZ_ARG_TYPE_STR) {
            if (arg_type == BLITZ_ARG_TYPE_VAR) {
                if ((iteration_params && (z = blitz_hash_str_find_ind(HASH_OF(iteration_params), arg->name, arg->len)) != NULL)
                    || (z = blitz_hash_str_find_ind(tpl->hash_globals, arg->name, arg->len)) != NULL)
                {
                    found = 1;
                }
            } else if (arg_type == BLITZ_ARG_TYPE_VAR_PATH) {
                if (blitz_fetch_var_by_path(&z, arg->name, arg->len, iteration_params, tpl)) {
                    found = 1;
                }
            }

            if (found) {
                zstr = zval_get_string(z);
                filename = zstr->val;
                filename_len = zstr->len;
            }
        }

        // Check if we have scope provided
        if (node->n_args > 1) {
            have_scope_iteration = 1;
            array_init(&scope_iteration);

            for (i = 1; (i + 1) < node->n_args; i += 2) {
                tmp_arg = node->args + i + 1;

                if (blitz_arg_to_zval(tpl, node, parent_params, tmp_arg, &tmp_z) == 0) {
                    continue;
                }

                tmp_arg = node->args + i;

                add_assoc_zval(&scope_iteration, tmp_arg->name, &tmp_z);
            }

            zend_hash_merge(HASH_OF(&scope_iteration), HASH_OF(iteration_params), zval_add_ref, 0);

            if (BLITZ_DEBUG) {
                smart_str buf = {0};
                php_var_export_ex(&scope_iteration, 1, &buf);
                smart_str_0 (&buf);
                php_printf("--> scope_iteration:%s value:%s\n", zend_zval_type_name(&scope_iteration), buf.a);
                smart_str_free(&buf);
            }

        }

        if (!blitz_include_tpl_cached(tpl, filename, filename_len, (have_scope_iteration ? &scope_iteration : iteration_params), &itpl)) {
            if (zstr) {
                zend_string_release(zstr);
            }
            return 0;
        }

        if (zstr) {
            zend_string_release(zstr);
        }

        /* parse */
        if ((res = blitz_exec_template(itpl, id, &inner_result))) {
            smart_str_append_smart_str_ex(result, &inner_result, 0);
            smart_str_0(result);
            smart_str_free(&inner_result);
        }

        if (have_scope_iteration) {
            zval_ptr_dtor(&scope_iteration);
        }
    } else if (node->type >= BLITZ_NODE_TYPE_WRAPPER_ESCAPE && node->type < BLITZ_NODE_TYPE_IF_NF) {
        char *wrapper_args[BLITZ_WRAPPER_MAX_ARGS];
        unsigned int wrapper_args_len[BLITZ_WRAPPER_MAX_ARGS];
        smart_str str = {NULL, 0};
        call_arg *p_arg = NULL;
        int i = 0;
        int wrapper_args_num = 0;

        for(i=0; i < BLITZ_WRAPPER_MAX_ARGS; i++) {
            wrapper_args[i] = NULL;
            wrapper_args_len[i] = 0;

            if (i<node->n_args) {
                zend_string *zstr;

                p_arg = node->args + i;
                wrapper_args_num++;
                if (p_arg->type == BLITZ_ARG_TYPE_VAR) {
                    if ((iteration_params && (z = blitz_hash_str_find_ind(HASH_OF(iteration_params), p_arg->name, p_arg->len)) != NULL)
                        || (z = blitz_hash_str_find_ind(tpl->hash_globals, p_arg->name, p_arg->len)) != NULL)
                    {
                        if (Z_TYPE_P(z) == IS_REFERENCE) {
                            z = Z_INDIRECT_P(z);
                        }
                        zstr = zval_get_string(z);
                        wrapper_args[i] = estrndup(ZSTR_VAL(zstr), ZSTR_LEN(zstr));
                        wrapper_args_len[i] = ZSTR_LEN(zstr);
                        zend_string_release(zstr);
                    }
                } else if (p_arg->type == BLITZ_ARG_TYPE_VAR_PATH) {
                    if (blitz_fetch_var_by_path(&z, p_arg->name, p_arg->len, iteration_params, tpl)) {
                        if (Z_TYPE_P(z) == IS_INDIRECT) {
                            z = Z_INDIRECT_P(z);
                        }
                        zstr = zval_get_string(z);
                        wrapper_args[i] = estrndup(ZSTR_VAL(zstr), ZSTR_LEN(zstr));
                        wrapper_args_len[i] = ZSTR_LEN(zstr);
                        zend_string_release(zstr);
                    }
                } else {
                    wrapper_args[i] = estrndup(p_arg->name, p_arg->len);
                    wrapper_args_len[i] = p_arg->len;
                }
            }
        }

        if (blitz_exec_wrapper(tpl, &str, node->type, wrapper_args_num, wrapper_args, wrapper_args_len, tmp_buf)) {
            smart_str_appendl(result, ZSTR_VAL(str.s), ZSTR_LEN(str.s));
            smart_str_0(result);
            smart_str_free(&str);
        } else {
            res = 0;
        }

        for (i = 0; i < wrapper_args_num; i++) {
            efree(wrapper_args[i]);
        }
    }

    return res;
}
/* }}} */

/* {{{ int blitz_exec_user_method() */
static inline int blitz_exec_user_method(blitz_tpl *tpl, blitz_node *node, zval *iteration_params, zval *obj, smart_str *result, unsigned long *result_alloc_len)
{
    zval retval, zmethod, *ztmp;
    int method_res = 0;
    zval **args = NULL;
    zval *pargs = NULL;
    unsigned int i = 0;
    call_arg *i_arg = NULL;
    char i_arg_type = 0;
    long predefined = -1;
    int has_iterations = 0, found = 0;
    zval *old_caller_iteration = NULL;
    blitz_tpl *tpl_caller = NULL;
    HashTable *function_table = NULL;

    if (BLITZ_DEBUG) php_printf("*** FUNCTION *** blitz_exec_user_method: %s\n", node->lexem);

    ZVAL_STRING(&zmethod, node->lexem);
    ZVAL_UNDEF(&retval);

    has_iterations = (iteration_params != NULL); // fetch has NULL *iteration_params

    /* prepare arguments if needed */
    /* 2DO: probably there's much more easy way to prepare arguments without two emalloc */
    if (node->n_args > 0 && node->args) {
        /* dirty trick to pass arguments */
        /* pargs are zval ** with parameter data */
        /* args just point to pargs */
        args = ecalloc(node->n_args, sizeof(*args));
        pargs = emalloc(node->n_args * sizeof(*pargs));
        for (i = 0; i < node->n_args; i++) {
            zval arg;

            args[i] = NULL;
            ZVAL_NULL(&pargs[i]);
            ZVAL_NULL(&arg);
            i_arg  = node->args + i;
            i_arg_type = i_arg->type;

            if (BLITZ_DEBUG) php_printf("setting argument (name, type): (%s, %d) \n", i_arg->name, i_arg_type);

            if (i_arg_type == BLITZ_ARG_TYPE_VAR) {
                predefined = -1;
                found = blitz_extract_var(tpl, i_arg->name, i_arg->len, 0, iteration_params, &predefined, &ztmp);
                if (predefined >= 0) {
                    ZVAL_LONG(&arg, (long)predefined);
                } else if (found) {
                    args[i] = ztmp;
                }
            } else if (i_arg_type == BLITZ_ARG_TYPE_VAR_PATH) {
                if (has_iterations && blitz_fetch_var_by_path(&ztmp, i_arg->name, i_arg->len, iteration_params, tpl)) {
                    args[i] = ztmp;
                }
            } else if (i_arg_type == BLITZ_ARG_TYPE_NUM) {
                ZVAL_LONG(&arg, atol(i_arg->name));
            } else if (i_arg_type == BLITZ_ARG_TYPE_FALSE) {
                ZVAL_FALSE(&arg);
            } else if (i_arg_type == BLITZ_ARG_TYPE_TRUE) {
                ZVAL_TRUE(&arg);
            } else {
                ZVAL_STRING(&arg, i_arg->name);
            }

            if (args[i]) {
                zval_ptr_dtor(&arg);
                pargs[i] = *args[i];
            } else {
                pargs[i] = arg;
            }
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

    /*
        CALLBACK SETTINGS:
            * enable_callbacks - all callbacks, PHP and object methods, checked externally
            * enable_php_callbacks - just PHP callbacks
            * php_callbacks_first - PHP callbacks have high priority in cases when it's unclear
              if it's PHP callback or template object callback
        CALLBACK PRIORITIES:
            * Call object method if it's a this::call().
            * Call PHP method if it's any other namespace::call() and PHP calls are enabled.
            * If PHP-calls are enabled and have highest priority - call PHP method and then object method.
              Otherwise, call object method and then PHP if PHP cals are enabled.
    */

    function_table = &Z_OBJCE_P(obj)->function_table;
    if (node->namespace_code == BLITZ_NODE_THIS_NAMESPACE) {
        method_res = call_user_function(function_table, obj, &zmethod, &retval, node->n_args, pargs);
    } else if (node->namespace_code) {
        if (BLITZ_G(enable_php_callbacks)) {
            method_res = call_user_function(NULL, NULL, &zmethod, &retval, node->n_args, pargs);
        } else {
            blitz_error(tpl, E_WARNING,
                "PHP callbacks are disabled by blitz.enable_php_callbacks, %s call was ignored, line %lu, pos %lu",
                node->lexem,
                get_line_number(&tpl->static_data.body, node->pos_begin),
                get_line_pos(&tpl->static_data.body, node->pos_begin)
            );
        }
    } else {
        if (BLITZ_G(php_callbacks_first) && BLITZ_G(enable_php_callbacks)) {
            method_res = call_user_function(NULL, NULL, &zmethod, &retval, node->n_args, pargs);
            if (method_res == FAILURE) {
                method_res = call_user_function(function_table, obj, &zmethod, &retval, node->n_args, pargs);
            }
        } else {
            method_res = call_user_function(function_table, obj, &zmethod, &retval, node->n_args, pargs);
            if (method_res == FAILURE && BLITZ_G(enable_php_callbacks)) {
                method_res = call_user_function(NULL, NULL, &zmethod, &retval, node->n_args, pargs);
            }
        }
    }

    tpl_caller->caller_iteration = old_caller_iteration;

    tpl->flags &= ~BLITZ_FLAG_CALLED_USER_METHOD;

    if (method_res == FAILURE) { /* failure: */
        blitz_error(tpl, E_WARNING,
            "INTERNAL ERROR: calling function \"%s\" failed, check if this function exists or parameters are valid", node->lexem);
    } else if (Z_TYPE(retval) != IS_UNDEF) { /* retval can be empty even in success: method throws exception */
        zend_string *str = zval_get_string(&retval);
        smart_str_append_ex(result, str, 0);
        smart_str_0(result);
        zend_string_release(str);
    }

    zval_ptr_dtor(&zmethod);
    zval_ptr_dtor(&retval);

    if (pargs) {
         for(i=0; i<node->n_args; i++) {
             zval *arg = &pargs[i];
             if (args[i] == NULL) {
                 zval_ptr_dtor(arg);
             }
         }
         efree(args);
         efree(pargs);
    }

    return 1;
}
/* }}} */

/* {{{ */
int blitz_nl2br(char *in, long unsigned int in_len, char **out, long unsigned int *len)
{
    char *tmp, *str, *result;
    long unsigned int new_length = 0;
    char *end, *target;
    long unsigned int repl_cnt = 0;
    zend_bool is_xhtml = 1;

    str = in;
    tmp = str;
    end = str + in_len;

    /* it is really faster to scan twice and allocate mem once instead of scanning once and constantly reallocing */
    while (tmp < end) {
        if (*tmp == '\r') {
            if (*(tmp + 1) == '\n') {
                tmp++;
            }
            repl_cnt++;
        } else if (*tmp == '\n') {
            if (*(tmp+1) == '\r') {
                tmp++;
            }
            repl_cnt++;
        }

        tmp++;
    }

    if (repl_cnt == 0)
        return 0;

    if (is_xhtml) {
        new_length = in_len + repl_cnt * (sizeof("<br />") - 1);
    } else {
        new_length = in_len + repl_cnt * (sizeof("<br>") - 1);
    }

    result = target = emalloc(new_length + 1);

    while (str < end) {
        switch (*str) {
            case '\r':
            case '\n':
                *target++ = '<';
                *target++ = 'b';
                *target++ = 'r';

                if (is_xhtml) {
                    *target++ = ' ';
                    *target++ = '/';
                }

                *target++ = '>';

                if ((*str == '\r' && *(str + 1) == '\n') || (*str == '\n' && *(str + 1) == '\r')) {
                    *target++ = *str++;
                }
                /* lack of a break; is intentional */
            default:
                *target++ = *str;
        }

        str++;
    }

    *target = '\0';
    *out = result;
    *len = new_length;

    return 1;
}
/* }}} */

/* {{{ void blitz_exec_var() */
static inline void blitz_exec_var(
    blitz_tpl *tpl,
    char *name,
    unsigned long len,
    unsigned char is_path,
    unsigned char escape_mode,
    unsigned long pos,
    zval *params,
    smart_str *result,
    unsigned long *result_alloc_len)
{
    zval *zparam = NULL;
    long predefined = -1;
    zend_string *escaped = NULL;
    unsigned int found = 0;

    found = blitz_extract_var(tpl, name, len, is_path, params, &predefined, &zparam);

    if (predefined >= 0) {
        smart_str_append_unsigned(result, predefined);
        smart_str_0(result);
        return;
    }

    if (!found || !zparam) { // variable wasn't found
        return;
    }

    if (Z_TYPE_P(zparam) != IS_STRING) {
        zend_string *str;

        if (Z_TYPE_P(zparam) == IS_ARRAY) {
            blitz_error(tpl, E_WARNING, "Array to string conversion for variable \"%s\" (%s: line %u, position %u)",
                name, tpl->static_data.name, get_line_number(&tpl->static_data.body, pos), get_line_pos(&tpl->static_data.body, pos));
            return;
        }

        if (Z_TYPE_P(zparam) == IS_INDIRECT) {
            zparam = Z_INDIRECT_P(zparam);
        }

        str = zval_get_string(zparam);
        smart_str_append_ex(result, str, 0);
        zend_string_release(str);
    } else {
        if (escape_mode == BLITZ_ESCAPE_YES || escape_mode == BLITZ_ESCAPE_NL2BR || ((escape_mode == BLITZ_ESCAPE_DEFAULT) && BLITZ_G(auto_escape))) {
#if defined(ENT_SUBSTITUTE) && defined(ENT_DISALLOWED) && defined(ENT_HTML5)
            long quote_style = ENT_QUOTES | ENT_SUBSTITUTE | ENT_DISALLOWED | ENT_HTML5;
#else
            long quote_style = ENT_QUOTES;
#endif
            escaped = php_escape_html_entities_ex((unsigned char *) Z_STRVAL_P(zparam), Z_STRLEN_P(zparam), 0, quote_style, SG(default_charset), 1, 0);

            if (escape_mode == BLITZ_ESCAPE_NL2BR) {
                char *escaped_str;
                unsigned long var_len;

                if (blitz_nl2br(ZSTR_VAL(escaped), ZSTR_LEN(escaped), &escaped_str, &var_len)) {
                    smart_str_appendl(result, escaped_str, var_len);
                    efree(escaped_str);
                } else {
                    smart_str_append_ex(result, escaped, 0);
                }
            } else {
                smart_str_append_ex(result, escaped, 0);
            }
            zend_string_release(escaped);
        } else {
            smart_str_append_ex(result, Z_STR_P(zparam), 0);
        }

    }

    smart_str_0(result);
}
/* }}} */

/* {{{ int blitz_exec_literal() */
static void blitz_exec_literal(blitz_tpl *tpl, blitz_node *node, zval *parent_params, zval *id,
    smart_str *result, unsigned long *result_alloc_len)
{
    unsigned long literal_len = 0;

    if (BLITZ_DEBUG) php_printf("*** FUNCTION *** blitz_exec_literal\n");
    if (BLITZ_DEBUG) {
        php_printf("node: pos_begin = %lu, pos_begin_shift = %lu, pos_end = %lu, pos_end_shift = %lu\n",
        node->pos_begin, node->pos_begin_shift, node->pos_end, node->pos_end_shift);
    }

    if (0 == node->pos_end_shift) { // {{ literal }} with no {{ end }}
        return;
    }

    literal_len = node->pos_end_shift - node->pos_begin_shift;

    smart_str_appendl(result, ZSTR_VAL(tpl->static_data.body.s) + node->pos_begin_shift, literal_len);
    smart_str_0(result);
}

/* {{{ int blitz_exec_context() */
static void blitz_exec_context(blitz_tpl *tpl, blitz_node *node, zval *parent_params, zval *id,
    smart_str *result, unsigned long *result_alloc_len)
{
    zend_string *key = NULL;
    zend_ulong key_index = 0;
    int check_key = 0, not_empty = 0, first_type = -1;
    long predefined = -1;
    zval *ctx_iterations = NULL;
    zval *ctx_data = NULL;
    call_arg *arg = node->args;
    HashPosition pos;

    if (BLITZ_DEBUG) php_printf("*** FUNCTION *** blitz_exec_context: %s\n",node->args->name);

    if (!parent_params) {
        if (BLITZ_DEBUG) php_printf("blitz_exec_context was called with empty parent_params: nothing to do, will return\n");
        return;
    } else {
        if (BLITZ_DEBUG) {
            php_printf("parent_params:\n");
            php_var_dump(parent_params, 0);
        }
    }

    if (BLITZ_DEBUG) {
        php_printf("checking key %s in parent_params...\n", arg->name);
    }

    if (blitz_extract_var(tpl, arg->name, arg->len, (arg->type == BLITZ_ARG_TYPE_VAR_PATH), parent_params, &predefined, &ctx_iterations) == 0) {
        if (BLITZ_DEBUG) {
            php_printf("failed to find key %s in parent params\n", arg->name);
            php_printf("as we failed to find key %s in parent params and node->type is BLITZ_NODE_TYPE_CONTEXT - will return\n", arg->name);
        }
        return;
    } else {
        if (BLITZ_DEBUG) {
            php_printf("key %s was found in parent params\n", arg->name);
        }

        BLITZ_ZVAL_NOT_EMPTY(ctx_iterations, not_empty);
    }

    if (BLITZ_DEBUG) {
        php_printf("will exec context (type=%u) with iterations:\n", node->type);
        php_var_dump(ctx_iterations, 0);
        php_printf("not_empty = %u\n", not_empty);
    }

    if (BLITZ_DEBUG) php_printf("data found in parent params for key %s\n",arg->name);
    if ((Z_TYPE_P(ctx_iterations) == IS_ARRAY || Z_TYPE_P(ctx_iterations) == IS_OBJECT) && zend_hash_num_elements(HASH_OF(ctx_iterations))) {
        if (BLITZ_DEBUG) php_printf("will walk through iterations\n");

        zend_hash_internal_pointer_reset_ex(HASH_OF(ctx_iterations), &pos);
        check_key = zend_hash_get_current_key_ex(HASH_OF(ctx_iterations), &key, &key_index, &pos);
        if (BLITZ_DEBUG) php_printf("KEY_TYPE: %d\n", check_key);

        if (HASH_KEY_IS_STRING == check_key) {
            if (BLITZ_DEBUG) php_printf("KEY_CHECK: %s <string>\n", key->val);
            blitz_exec_nodes(tpl, node->first_child, id,
                result, result_alloc_len,
                node->pos_begin_shift,
                node->pos_end_shift,
                ctx_iterations, parent_params
            );
        } else if (HASH_KEY_IS_LONG == check_key) {
            if (BLITZ_DEBUG) php_printf("KEY_CHECK: %llu <zend_ulong>\n", key_index);
            BLITZ_LOOP_INIT(tpl, zend_hash_num_elements(HASH_OF(ctx_iterations)));
            first_type = -1;
            while ((ctx_data = blitz_hash_get_current_data_ex(HASH_OF(ctx_iterations), &pos)) != NULL) {
                INDIRECT_CONTINUE_FORWARD(HASH_OF(ctx_iterations), ctx_data);

                if (first_type == -1) first_type = Z_TYPE_P(ctx_data);
                if (BLITZ_DEBUG) {
                    php_printf("GO subnode, params:\n");
                    php_var_dump(ctx_data,0);
                }
                /* mix of num/str errors: array(0=>array(), 'key' => 'val') */
                if (first_type != Z_TYPE_P(ctx_data)) {
                    blitz_error(tpl, E_WARNING,
                        "ERROR: You have a mix of numerical and non-numerical keys in the iteration set "
                        "(context: %s, line %lu, pos %lu), key was ignored",
                        node->args[0].name,
                        get_line_number(&tpl->static_data.body, node->pos_begin),
                        get_line_pos(&tpl->static_data.body, node->pos_begin)
                    );
                    zend_hash_move_forward_ex(HASH_OF(ctx_iterations), &pos);
                    continue;
                }

                blitz_exec_nodes(tpl, node->first_child, id,
                    result, result_alloc_len,
                    node->pos_begin_shift,
                    node->pos_end_shift,
                    ctx_data, parent_params
                );
                BLITZ_LOOP_INCREMENT(tpl);
                zend_hash_move_forward_ex(HASH_OF(ctx_iterations), &pos);
            }
        } else {
            blitz_error(tpl, E_WARNING, "INTERNAL ERROR: non existant hash key");
        }
    }
}
/* }}} */

/* {{{ int blitz_extract_var() */
static inline unsigned int blitz_extract_var (
    blitz_tpl *tpl,
    char *name,
    unsigned long len,
    unsigned char is_path,
    zval *params,
    long *l,
    zval **z)
{
    *l = -1;
    BLITZ_GET_PREDEFINED_VAR(tpl, name, len, *l);

    if (*l >= 0) {
        return 1;
    }

    if (BLITZ_IS_PREDEFINED_TOP(name, len)) {
        if (BLITZ_DEBUG) php_printf("predefined var _top\n");
        *z = tpl->scope_stack[0];
        return 1;
    } else if (BLITZ_IS_PREDEFINED_PARENT(name, len)) {
        int magic_stack;
        if (BLITZ_DEBUG) php_printf("predefined path _parent\n");
        magic_stack = (tpl->scope_stack_pos > 2 ? tpl->scope_stack_pos - 2 : 0); /* keep track of the current magic scope to enable things like _parent._parent */
        *z = tpl->scope_stack[magic_stack];
        return 1;
    } else if (len == 1 && name[0] == '_') {
        *z = tpl->scope_stack[tpl->scope_stack_pos-1];
        return 1;
    }

    if (is_path) {
        return blitz_fetch_var_by_path(z, name, len, params, tpl);
    } else {
        if (params && (*z = blitz_hash_str_find_ind(HASH_OF(params), name, len)) != NULL) {
            return 1;
        }

        if (tpl->hash_globals && (*z = blitz_hash_str_find_ind(tpl->hash_globals, name, len)) != NULL) {
            return 1;
        }

        if (tpl->scope_stack_pos && BLITZ_G(scope_lookup_limit) && blitz_scope_stack_find_ind(tpl, name, len, z)) {
            return 1;
        }

        return 0;
    }

    return 1;
}
/* }}} */

/* {{{ int blitz_arg_to_zval(blitz_tpl *tpl, blitz_node *node, zval *parent_params, call_arg **arg, zval **result) */
static inline int blitz_arg_to_zval(
    blitz_tpl *tpl,
    blitz_node *node,
    zval *parent_params,
    call_arg *arg,
    zval *return_value)
{
    long predefined = -1;
    zval *ztmp = NULL;

    if (BLITZ_DEBUG) php_printf("*** FUNCTION *** blitz_arg_to_zval: %s[%lu] (type=%d: %s)\n", arg->name, arg->len, arg->type, BLITZ_ARG_TO_STRING(arg->type));

    if (arg->type == BLITZ_ARG_TYPE_VAR || arg->type == BLITZ_ARG_TYPE_VAR_PATH) {
        if (blitz_extract_var(tpl, arg->name, arg->len, (arg->type == BLITZ_ARG_TYPE_VAR_PATH), parent_params, &predefined, &ztmp) != 0) {
            if (predefined >= 0) {
                ZVAL_DOUBLE(return_value, (double)predefined);
            } else {
                ZVAL_COPY_VALUE(return_value, ztmp);
                zval_copy_ctor(return_value);
            }
        } else {
            // UNDEFINED
            if (BLITZ_DEBUG) php_printf("--> not found\n");
            ZVAL_NULL(return_value);
            return 0;
        }
    } else if (arg->type == BLITZ_ARG_TYPE_STR) {
        ZVAL_STRINGL(return_value, arg->name, arg->len);
    } else if (arg->type == BLITZ_ARG_TYPE_FALSE) {
        ZVAL_FALSE(return_value);
    } else if (arg->type == BLITZ_ARG_TYPE_TRUE) {
        ZVAL_TRUE(return_value);
    } else {
        ZVAL_DOUBLE(return_value, atof(arg->name));
    }

    if (BLITZ_DEBUG) {
        smart_str buf = {0};
        php_var_export_ex(return_value, 1, &buf);
        smart_str_0 (&buf);
        php_printf("--> type:%s value:%s\n", zend_zval_type_name(return_value), buf.a);
        smart_str_free(&buf);
    }

    return 1;
}
/* }}} */

/* {{{ int blitz_check_arg(blitz_tpl *tpl, blitz_node *node, zval *parent_params, int *not_empty) */
static inline void blitz_check_arg (
    blitz_tpl *tpl,
    blitz_node *node,
    zval *parent_params,
    int *not_empty)
{
    long predefined = -1;
    call_arg *arg = NULL;
    zval *z = NULL;

    arg = node->args;
    if (arg->type == BLITZ_ARG_TYPE_VAR || arg->type == BLITZ_ARG_TYPE_VAR_PATH) {
        if (blitz_extract_var(tpl, arg->name, arg->len, (arg->type == BLITZ_ARG_TYPE_VAR_PATH), parent_params, &predefined, &z) != 0) {
            if (predefined >= 0) {
                *not_empty = (predefined == 0 ? 0 : 1);
            } else {
                BLITZ_ZVAL_NOT_EMPTY(z, *not_empty)
            }
        } else {
            *not_empty = 0; // "unknown" is equal to "empty"
        }
    } else if (BLITZ_IS_ARG_EXPR(arg->type)) {
        blitz_error(NULL, E_WARNING, "EXPRESSION ERROR: Argument is an expression %s(%u) without operands (%s: line %lu, pos %lu)", BLITZ_OPERATOR_TO_STRING(arg->type), arg->type, tpl->static_data.name, get_line_number(&tpl->static_data.body, node->pos_begin), get_line_pos(&tpl->static_data.body, node->pos_begin));
        *not_empty = 0;
    } else {
        BLITZ_ARG_NOT_EMPTY(*arg, NULL, *not_empty);
    }
}
/* }}} */

/* {{{ int blitz_check_expr(blitz_tpl *tpl, blitz_node *node, unsigned int node_count, zval *parent_params, int *is_true) */
static inline void blitz_check_expr (
    blitz_tpl *tpl,
    blitz_node *node,
    zval *id,
    unsigned int node_count,
    zval *parent_params,
    int *is_true)
{
    unsigned long i = 0, j = 0;
    call_arg *arg = NULL;
    int num_a = -1, operands_needed = 0, found = 0, b, arguments_are_undefined = 0;
    int method_res;
    zval z_stack[BLITZ_IF_STACK_MAX];
    zval resval;
    zval *z_stack_ptr[BLITZ_IF_STACK_MAX] = {0};
    zend_native_function native_func;
    zval retval, zmethod, *args;
    char *tmp_str;

    memset(z_stack, 0, sizeof(zval) * BLITZ_IF_STACK_MAX);

    if (BLITZ_DEBUG)
        php_printf("*** FUNCTION *** blitz_check_expr argcnt=%d\n", node_count);

    for (i = 0; i < node_count; i++) {
        arg = &node->args[i];
        if (!BLITZ_IS_ARG_EXPR(arg->type)) {
            // No operator (so operand), just store the operand on the stack (and don't care about strings, we're just pointing, no need to copy the mem)
            if (++num_a < BLITZ_IF_STACK_MAX) {
                if (z_stack_ptr[num_a]) {
                    zval_ptr_dtor(z_stack_ptr[num_a]);
                }
                z_stack_ptr[num_a] = &z_stack[num_a];
                ZVAL_UNDEF(z_stack_ptr[num_a]);
                found = blitz_arg_to_zval(tpl, node, parent_params, arg, z_stack_ptr[num_a]);
                if (found == 0) {
                    arguments_are_undefined = 1;
                }

                if (BLITZ_DEBUG) {
                    php_printf("operands[%s]: name:%s (type:%s) %p\n", (found == 1  ? "FOUND" : "UNKNOWN"), arg->name, zend_zval_type_name(z_stack_ptr[num_a]), &z_stack_ptr[num_a]);
                }
            } else {
                blitz_error(NULL, E_WARNING, "Too complex conditional, operator stack depth is too high and broken, operators will be resolved improperly. To fix this rebuild blitz extension with increased BLITZ_IF_STACK_MAX constant in php_blitz.h (%s: line %lu, pos %lu)", get_line_number(&tpl->static_data.body, node->pos_begin), get_line_pos(&tpl->static_data.body, node->pos_begin));
                *is_true = -1;
                return;
            }
        } else {
            // Check if we have enough operands
            if (arg->type == BLITZ_EXPR_OPERATOR_METHOD) {
                // this hack only works in simple cases when there is a single call like '{{IF callback(1,2+4,3)}}
                operands_needed = num_a + 1;
                if (BLITZ_DEBUG) php_printf("Operands for method: %d\n", operands_needed);
            } else {
                operands_needed = BLITZ_OPERATOR_GET_NUM_OPERANDS(arg->type);
                if (num_a + 1 < operands_needed) {
                    blitz_error(NULL, E_WARNING, "EXPRESSION ERROR: Condition %s(%u) requires %d operands, but we only have %d (%s: line %lu, pos %lu)", BLITZ_OPERATOR_TO_STRING(arg->type), arg->type, operands_needed, (num_a + 1), tpl->static_data.name, get_line_number(&tpl->static_data.body, node->pos_begin), get_line_pos(&tpl->static_data.body, node->pos_begin));
                    *is_true = 0;
                    return;
                }
            }

            // Prepare the operands
            if (BLITZ_DEBUG) {
                tmp_str = estrndup(arg->name, arg->len);
                php_printf("executing operator:%s\n", tmp_str);
                efree(tmp_str);
            }

            // Execute the operator
            if (arg->type == BLITZ_EXPR_OPERATOR_METHOD) {
                ZVAL_STRINGL(&zmethod, arg->name, arg->len);
                args = emalloc(operands_needed * sizeof(*args));
                for (j = 0; j < operands_needed; j++) {
                    args[j] = *z_stack_ptr[j];
                }

                ZVAL_UNDEF(&retval);

                if (BLITZ_G(enable_php_callbacks)) {
                    method_res = call_user_function(&Z_OBJCE_P(id)->function_table, id, &zmethod, &retval, operands_needed, args);
                } else {
                    method_res = FAILURE;
                    blitz_error(tpl, E_WARNING,
                        "PHP callbacks are disabled by blitz.enable_php_callbacks, %s call was ignored, line %lu, pos %lu",
                        node->lexem,
                        get_line_number(&tpl->static_data.body, node->pos_begin),
                        get_line_pos(&tpl->static_data.body, node->pos_begin)
                    );
                }

                if (method_res == FAILURE) {
                    tmp_str = estrndup(arg->name, arg->len);
                    blitz_error(tpl, E_WARNING,
                        "INTERNAL ERROR: calling method \"%s\" failed, check if this function exists or parameters are valid", tmp_str);
                    efree(tmp_str);
                    b = 0;
                } else if (Z_TYPE_P(&retval) != IS_UNDEF) {
                    b = zval_get_long(&retval) ? 1 : 0;
                    zval_ptr_dtor(&retval);
                }

                num_a = 0;
                zval_dtor(&z_stack[num_a]);
                ZVAL_BOOL(&z_stack[num_a], b);
                efree(args);
                zval_ptr_dtor(&zmethod);
            } else if (arg->type == BLITZ_EXPR_OPERATOR_N) {
                // If our operand is undefined, return true
                if (arguments_are_undefined == 1) {
                    zval_dtor(&z_stack[num_a]);
                    ZVAL_BOOL(&z_stack[num_a], 1);
                    if (BLITZ_DEBUG) php_printf("operand is undefined, result is true\n");
                } else {
                    b = (zend_is_true(z_stack_ptr[num_a]) == 1 ? 0 : 1);
                    zval_dtor(&z_stack[num_a]);
                    ZVAL_BOOL(&z_stack[num_a], b);
                }
            } else if (arg->type == BLITZ_EXPR_OPERATOR_LA) {
                b = (zend_is_true(z_stack_ptr[num_a]) == 1 && zend_is_true(z_stack_ptr[num_a-1]) == 1 ? 1 : 0);
                zval_dtor(&z_stack[--num_a]);
                ZVAL_BOOL(&z_stack[num_a], b);
            } else if (arg->type == BLITZ_EXPR_OPERATOR_LO) {
                b = (zend_is_true(z_stack_ptr[num_a]) == 1 || zend_is_true(z_stack_ptr[num_a-1]) == 1 ? 1 : 0);
                zval_dtor(&z_stack[--num_a]);
                ZVAL_BOOL(&z_stack[num_a], b);
            } else if (arg->type == BLITZ_EXPR_OPERATOR_ADD ||
                    arg->type == BLITZ_EXPR_OPERATOR_SUB ||
                    arg->type == BLITZ_EXPR_OPERATOR_MUL ||
                    arg->type == BLITZ_EXPR_OPERATOR_DIV ||
                    arg->type == BLITZ_EXPR_OPERATOR_MOD) {
                native_func = BLITZ_OPERATOR_TO_ZEND_NATIVE_FUNCTION(arg->type);
                if (arguments_are_undefined == 1) {
                    if (BLITZ_DEBUG) php_printf("one of the operands is undefined, result is false\n");
                    zval_dtor(&z_stack[--num_a]);
                    ZVAL_BOOL(&z_stack[num_a], 0);
                } else if (native_func(&resval, z_stack_ptr[num_a-1], z_stack_ptr[num_a]) == FAILURE) {
                    if (BLITZ_DEBUG) php_printf("one of the operands is of incorrect type, or failed to run mod_function, result is error\n");
                    *is_true = -1;
                    return;
                } else if (Z_TYPE(resval) == IS_DOUBLE) {
                    zval_dtor(&z_stack[--num_a]);
                    ZVAL_DOUBLE(&z_stack[num_a], Z_DVAL(resval));
                } else {
                    zval_dtor(&z_stack[--num_a]);
                    ZVAL_LONG(&z_stack[num_a], Z_LVAL(resval));
                }
            } else {
                if (arguments_are_undefined == 1) {
                    if (BLITZ_DEBUG) php_printf("one of the operands is undefined, result is false\n");
                    zval_dtor(&z_stack[--num_a]);
                    ZVAL_BOOL(&z_stack[num_a], 0);
                } else if (compare_function(&resval, z_stack_ptr[num_a-1], z_stack_ptr[num_a]) == FAILURE) {
                    if (BLITZ_DEBUG) php_printf("one of the operands is of incorrect type, or failed to run compare_function, result is error\n");
                    *is_true = -1;
                    return;
                } else {
                    if (BLITZ_DEBUG) php_printf("compare function resulted in %ld\n", Z_LVAL(resval));
                    zval_dtor(&z_stack[--num_a]);
                    switch (arg->type) {
                        case BLITZ_EXPR_OPERATOR_E:  ZVAL_BOOL(&z_stack[num_a], (Z_LVAL(resval) == 0) ? 1 : 0); break;
                        case BLITZ_EXPR_OPERATOR_NE: ZVAL_BOOL(&z_stack[num_a], (Z_LVAL(resval) != 0) ? 1 : 0); break;
                        case BLITZ_EXPR_OPERATOR_G:  ZVAL_BOOL(&z_stack[num_a], (Z_LVAL(resval) > 0) ? 1 : 0); break;
                        case BLITZ_EXPR_OPERATOR_GE: ZVAL_BOOL(&z_stack[num_a], (Z_LVAL(resval) >= 0) ? 1 : 0); break;
                        case BLITZ_EXPR_OPERATOR_L:  ZVAL_BOOL(&z_stack[num_a], (Z_LVAL(resval) < 0) ? 1 : 0); break;
                        case BLITZ_EXPR_OPERATOR_LE: ZVAL_BOOL(&z_stack[num_a], (Z_LVAL(resval) <= 0) ? 1 : 0); break;
                        default: ZVAL_BOOL(&z_stack[num_a], 0); break;
                    }
                }
            }
            if (z_stack_ptr[num_a]) {
                zval_ptr_dtor(z_stack_ptr[num_a]);
            }
            z_stack_ptr[num_a] = &z_stack[num_a];

            if (BLITZ_DEBUG) {
                smart_str buf = {0};
                php_var_export_ex(z_stack_ptr[num_a], 1, &buf);
                smart_str_0 (&buf);
                php_printf("intermediate results --> type:%s value:%s\n", zend_zval_type_name(&z_stack[num_a]), buf.a);
                smart_str_free(&buf);
            }

            arguments_are_undefined = 0;
        }
    }

    // If all goes well we only have 1 argument on the stack
    if (num_a != 0) {
        blitz_error(NULL, E_WARNING, "EXPRESSION ERROR: Condition stack contains still %d items, only one is expected (%s: line %lu, pos %lu)", (num_a + 1), tpl->static_data.name, get_line_number(&tpl->static_data.body, node->pos_begin), get_line_pos(&tpl->static_data.body, node->pos_begin));
        *is_true = -1;
        return;
    }

    *is_true = zend_is_true(z_stack_ptr[0]);
    for (i = 0; i < BLITZ_IF_STACK_MAX; i++) {
        if (z_stack_ptr[i]) {
            zval_ptr_dtor(z_stack_ptr[i]);
        }
    }
}
/* }}} */

/* {{{ int blitz_exec_if_context() */
static void blitz_exec_if_context(
    blitz_tpl *tpl,
    unsigned long node_id,
    zval *parent_params,
    zval *id,
    smart_str *result,
    unsigned long *result_alloc_len,
    unsigned long *jump)
{
    int condition = 0, is_true = 0;
    blitz_node *node = NULL;
    unsigned int i = 0, i_jump = 0, pos_end = 0;

    node = tpl->static_data.nodes + node_id;
    i = node_id;

    // find node to execute
    node = tpl->static_data.nodes + i;

    if (BLITZ_DEBUG)
        php_printf("*** FUNCTION *** blitz_exec_if_context\n");

    while (node) {

        if (node->type == BLITZ_NODE_TYPE_ELSE_CONTEXT) {
            condition = 1;
        } else {
            // special check is required for if syntax with a single method call like '{{IF some_method()}}'
            if (node->n_args > 1 || (node->n_args == 1 && node->args[0].type == BLITZ_EXPR_OPERATOR_METHOD)) {
                blitz_check_expr(tpl, node, id, node->n_if_args, parent_params, &is_true);
            } else {
                blitz_check_arg(tpl, node, parent_params, &is_true);
            }

            if (node->type == BLITZ_NODE_TYPE_UNLESS_CONTEXT) {
                condition = is_true == 1 ? 0 : 1;
            } else {
                condition = is_true == 1 ? 1 : 0;
            }
        }

        if (BLITZ_DEBUG) {
            php_printf("checking context %s (type=%u, args=%u)\n", node->lexem, node->type, node->n_args);
            php_printf("condition = %u, is_true = %d\n", condition, is_true);
        }

        if (!condition) { // if condition is false - move to the next node in this chain
            pos_end = node->pos_end;
            i_jump = 0;
            while ((node = node->next)) {
                if (node->pos_begin >= pos_end || node->pos_begin_shift >= pos_end) { // vars have pos_begin_shift = 0
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
            // but don't push the stack, since IF isn't another scope level
            blitz_exec_nodes_ex(tpl, node->first_child, id,
                result, result_alloc_len,
                node->pos_begin_shift, node->pos_end_shift,
                parent_params, parent_params, 0
            );

            // find the end of this chain
            pos_end = node->pos_end;
            while ((node = node->next)) {
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
    zval *id, smart_str *result, unsigned long *result_alloc_len,
    unsigned long parent_begin, unsigned long parent_end, zval *ctx_data, zval *parent_params)
{
    return blitz_exec_nodes_ex(tpl, first_child, id, result, result_alloc_len, parent_begin, parent_end, ctx_data, parent_params, 1);
}
/* }}} */

/* {{{ int blitz_exec_nodes_ex() */
static int blitz_exec_nodes_ex(blitz_tpl *tpl, blitz_node *first_child,
    zval *id, smart_str *result, unsigned long *result_alloc_len,
    unsigned long parent_begin, unsigned long parent_end, zval *ctx_data, zval *parent_params, int push_stack)
{
    unsigned long buf_len = 0;
    unsigned long last_close = 0, current_open = 0, i = 0, n_jump = 0;
    zval *ctx = NULL;
    blitz_node *node = NULL;

    /* check parent data (once in the beginning) - user could put non-array here.  */
    /* if we use hash_find on non-array - we get segfaults. */
    if (ctx_data && (Z_TYPE_P(ctx_data) == IS_ARRAY || Z_TYPE_P(ctx_data) == IS_OBJECT)) {
        ctx = ctx_data;
    }

    last_close = parent_begin;

    if (BLITZ_DEBUG) {
        php_printf("*** FUNCTION *** blitz_exec_nodes\n");
        if (ctx_data) {
            php_printf("ctx_data (%p):\n", ctx_data);
            php_var_dump(ctx_data, 0);
        }
        if (parent_params) {
            php_printf("parent_params (%p):\n", parent_params);
            php_var_dump(parent_params, 0);
        }
    }

    if (push_stack) {
        BLITZ_SCOPE_STACK_PUSH(tpl, ctx);
    }

    node = first_child;
    while (node) {

        if (BLITZ_DEBUG)
            php_printf("[EXEC] node:%s, pos = %ld, lc = %ld, next = %p\n", node->lexem, node->pos_begin, last_close, node->next);

        current_open = node->pos_begin;
        n_jump = 0;

        /* between nodes */
        if (current_open > last_close) {
            if (BLITZ_DEBUG) php_printf("copy part between nodes [%lu,%lu]...\n", last_close, current_open);
            buf_len = current_open - last_close;
            smart_str_appendl(result, ZSTR_VAL(tpl->static_data.body.s) + last_close, buf_len);
            smart_str_0(result);
            if (BLITZ_DEBUG) php_printf("...done\n");
        }

        if (node->lexem && !node->hidden) {
            if ((node->type == BLITZ_NODE_TYPE_VAR) || (node->type == BLITZ_NODE_TYPE_VAR_PATH)) {
                blitz_exec_var(tpl, node->lexem, node->lexem_len, node->type == BLITZ_NODE_TYPE_VAR_PATH, node->escape_mode, node->pos_begin,
                    ctx, result, result_alloc_len);
            } else if (BLITZ_IS_METHOD(node->type)) {
                if (node->type == BLITZ_NODE_TYPE_CONTEXT) {
                    BLITZ_LOOP_MOVE_FORWARD(tpl);
                    blitz_exec_context(tpl, node, ctx, id, result, result_alloc_len);
                    BLITZ_LOOP_MOVE_BACK(tpl);
                } else if (node->type == BLITZ_NODE_TYPE_LITERAL) {
                    blitz_exec_literal(tpl, node, ctx, id, result, result_alloc_len);
                } else if (node->type == BLITZ_NODE_TYPE_IF_CONTEXT || node->type == BLITZ_NODE_TYPE_UNLESS_CONTEXT) {
                    blitz_exec_if_context(tpl, node->pos_in_list, ctx, id,
                        result, result_alloc_len, &n_jump);
                } else {
                    zval *iteration_params = ctx ? ctx : NULL;
                    if (BLITZ_IS_PREDEF_METHOD(node->type)) {
                        blitz_exec_predefined_method(
                            tpl, node, iteration_params, parent_params, id,
                            result, result_alloc_len, tpl->tmp_buf
                        );
                    } else {
                        if (BLITZ_G(enable_callbacks)) {
                            blitz_exec_user_method(
                                tpl, node, iteration_params, id,
                                result, result_alloc_len
                            );
                        } else {
                            blitz_error(tpl, E_WARNING,
                                "callbacks are disabled by blitz.enable_callbacks, %s call was ignored, line %lu, pos %lu",
                                node->lexem,
                                get_line_number(&tpl->static_data.body, node->pos_begin),
                                get_line_pos(&tpl->static_data.body, node->pos_begin)
                            );
                        }
                    }
                }
            }
        }

        last_close = node->pos_end;
        if (n_jump) {
            if (BLITZ_DEBUG) php_printf("JUMP FROM: %s, n_jump = %lu\n", node->lexem, n_jump);
            i = 0;
            while(i++ < n_jump) {
                node = node->next;
            }
        } else {
            node = node->next;
        }
    }

    if (push_stack) {
        BLITZ_SCOPE_STACK_SHIFT(tpl);
    }

    if (BLITZ_DEBUG)
        php_printf("== D:b3  %ld,%ld,%ld\n",last_close,parent_begin,parent_end);

    if (parent_end>last_close) {
        buf_len = parent_end - last_close;
        smart_str_appendl(result, ZSTR_VAL(tpl->static_data.body.s) + last_close, buf_len);
        smart_str_0(result);
    }

    if (BLITZ_DEBUG)
        php_printf("== END NODES\n");

    return 1;
}
/* }}} */

static inline int blitz_populate_root (blitz_tpl *tpl) /* {{{ */
{
    zval empty_array;

    if (BLITZ_DEBUG) php_printf("will populate the root iteration\n");

    array_init(&empty_array);
    add_next_index_zval(&tpl->iterations, &empty_array);

    return 1;
}
/* }}} */

static int blitz_exec_template(blitz_tpl *tpl, zval *id, smart_str *result) /* {{{ */
{
    unsigned long result_alloc_len = 0;

    /* quick return if there was no nodes */
    if (0 == tpl->static_data.n_nodes) {
        *result = tpl->static_data.body;
        zend_string_addref(result->s);
        return 1; /* will not call efree on result */
    }

    /* build result, initial alloc of twice bigger than body */
    smart_str_alloc(result, ZSTR_LEN(tpl->static_data.body.s), 0);
    smart_str_0(result);

    if (0 == zend_hash_num_elements(HASH_OF(&tpl->iterations))) {
        blitz_populate_root(tpl);
    }

    {
        zval *iteration_data = NULL;
        zend_string *key;
        zend_ulong key_index;

        /* if it's an array of numbers - treat this as single iteration and pass as a parameter */
        /* otherwise walk and iterate all the array elements */
        zend_hash_internal_pointer_reset(HASH_OF(&tpl->iterations));
        if (HASH_KEY_IS_LONG != zend_hash_get_current_key(HASH_OF(&tpl->iterations), &key, &key_index)) {
            blitz_exec_nodes(tpl, tpl->static_data.nodes, id, result, &result_alloc_len, 0,
                ZSTR_LEN(tpl->static_data.body.s), &tpl->iterations, &tpl->iterations);
        } else {
            BLITZ_LOOP_INIT(tpl, zend_hash_num_elements(HASH_OF(&tpl->iterations)));
            while ((iteration_data = blitz_hash_get_current_data(HASH_OF(&tpl->iterations))) != NULL) {
                if (
                    HASH_KEY_IS_LONG != zend_hash_get_current_key(HASH_OF(&tpl->iterations), &key, &key_index)
                    || IS_ARRAY != Z_TYPE_P(iteration_data))
                {
                   zend_hash_move_forward(HASH_OF(&tpl->iterations));
                   continue;
                }
                INDIRECT_CONTINUE_FORWARD(HASH_OF(&tpl->iterations), iteration_data);

                blitz_exec_nodes(tpl, tpl->static_data.nodes, id,
                    result, &result_alloc_len, 0, ZSTR_LEN(tpl->static_data.body.s), iteration_data, &tpl->iterations);
                BLITZ_LOOP_INCREMENT(tpl);
                zend_hash_move_forward(HASH_OF(&tpl->iterations));
            }
        }
    }

    return 1;
}
/* }}} */

static inline int blitz_normalize_path(blitz_tpl *tpl, char **dest, const char *local, int local_len, char *global, int global_len) /* {{{ */
{
    int buf_len = 0;
    char *buf = *dest;
    register char  *p = NULL, *q = NULL;

    if (local_len && local[0] == '/') {
        if (local_len + 1 > BLITZ_CONTEXT_PATH_MAX_LEN) {
            blitz_error(tpl, E_WARNING, "context path %s is too long (limit %d)", local, BLITZ_CONTEXT_PATH_MAX_LEN);
            return 0;
        }
        memcpy(buf, local, local_len + 1);
        buf_len = local_len;
    } else {
        if (local_len + global_len + 2 > BLITZ_CONTEXT_PATH_MAX_LEN) {
            blitz_error(tpl, E_WARNING, "context path %s is too long (limit %d)", local, BLITZ_CONTEXT_PATH_MAX_LEN);
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

static inline int blitz_iterate_by_path(blitz_tpl *tpl, const char *path, int path_len, int is_current_iteration, int create_new) /* {{{ */
{
    zval *tmp;
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
        if (tpl->current_iteration) php_var_dump(tpl->current_iteration,1);
    }

    /* check root */
    if (*p == '/' && pmax == 1) {
        is_root = 1;
    }

    if ((0 == zend_hash_num_elements(HASH_OF(tmp)) || (is_root && create_new))) {
        blitz_populate_root(tpl);
    }

    /* iterate root  */
    if (is_root) {
        zval *tmp_iteration;
        zend_hash_internal_pointer_end(HASH_OF(tmp));
        if ((tmp_iteration = blitz_hash_get_current_data(HASH_OF(tmp))) != NULL) {
            INDIRECT_RETURN(tmp_iteration, 0);
            tpl->last_iteration = tmp_iteration;
            if (is_current_iteration) {
                /*blitz_hash_get_current_data(HASH_OF(*tmp), (void **) &tpl->current_iteration); */
                tpl->current_iteration = tpl->last_iteration;
                tpl->current_iteration_parent = & tpl->iterations;
            }
            if (BLITZ_DEBUG) {
                php_printf("last iteration becomes:\n");
                if (tpl->last_iteration) {
                    php_var_dump(tpl->last_iteration,1);
                } else {
                    php_printf("empty\n");
                }
            }
        } else {
            return 0;
        }
        return 1;
    }

    p++;
    while (i < pmax) {
        zval *tmp2;
        if (*p == '/' || i == k) {
            j = i - ilast;
            key_len = j + (i == k ? 1 : 0);
            memcpy(key, p-j, key_len);
            key[key_len] = '\x0';

            if (BLITZ_DEBUG) php_printf("[debug] going move to:%s\n",key);

            zend_hash_internal_pointer_end(HASH_OF(tmp));
            if ((tmp2 = blitz_hash_get_current_data(HASH_OF(tmp))) == NULL) {
                zval empty_array;
                if (BLITZ_DEBUG) php_printf("[debug] current_data not found, will populate the list \n");
                array_init(&empty_array);
                add_next_index_zval(tmp, &empty_array);
                if ((tmp2 = blitz_hash_get_current_data(HASH_OF(tmp))) == NULL) {
                    return 0;
                }
                INDIRECT_RETURN(tmp2, 0);
                if (BLITZ_DEBUG) {
                    php_printf("[debug] tmp becomes:\n");
                    php_var_dump(tmp,0);
                }
            } else {
                if (BLITZ_DEBUG) {
                    php_printf("[debug] tmp dump (node):\n");
                    php_var_dump(tmp,0);
                }
            }
            tmp = tmp2;

            if (Z_TYPE_P(tmp) != IS_ARRAY) {
                blitz_error(tpl, E_WARNING,
                    "OPERATION ERROR: unable to iterate context \"%s\" in \"%s\" "
                    "because parent iteration was not set as array of arrays before. "
                    "Correct your previous iteration sets.", key, path);
                return 0;
            }

            tmp2 = blitz_hash_str_find_ind(HASH_OF(tmp), key, key_len);
            if (!tmp2) {
                zval empty_array;
                zval init_array;

                if (BLITZ_DEBUG) php_printf("[debug] key \"%s\" was not found, will populate the list \n",key);
                found = 0;

                /* create */
                array_init(&empty_array);

                array_init(&init_array);
                /* [] = array(0 => array()) */
                if (BLITZ_DEBUG) {
                    php_printf("D-1: %p %p\n", tpl->current_iteration, tpl->last_iteration);
                    if (tpl->current_iteration) php_var_dump(tpl->current_iteration,1);
                }

                add_assoc_zval_ex(tmp, key, key_len, &init_array);

                if (BLITZ_DEBUG) {
                    php_printf("D-2: %p %p\n", tpl->current_iteration, tpl->last_iteration);
                   if (tpl->current_iteration) php_var_dump(tpl->current_iteration,1);
                }

                add_next_index_zval(&init_array, &empty_array);
                zend_hash_internal_pointer_end(HASH_OF(tmp));

                if (BLITZ_DEBUG) {
                    php_var_dump(tmp, 0);
                }

                /* 2DO: getting tmp and current_iteration_parent can be done by 1 call of blitz_hash_get_current_data */
                if (is_current_iteration) {
                    tpl->current_iteration_parent = blitz_hash_get_current_data(HASH_OF(tmp));
                    if (!tpl->current_iteration_parent) {
                        return 0;
                    }
                }

                tmp2 = blitz_hash_get_current_data(HASH_OF(tmp));
                if (!tmp2) {
                    return 0;
                }
            }

            tmp = tmp2;

            if (Z_TYPE_P(tmp2) != IS_ARRAY) {
                blitz_error(tpl, E_WARNING,
                    "OPERATION ERROR: unable to iterate context \"%s\" in \"%s\" "
                    "because it was set as \"scalar\" value before. "
                    "Correct your previous iteration sets.", key, path);
                return 0;
            }

            ilast = i + 1;
            if (BLITZ_DEBUG) {
                php_printf("[debug] tmp dump (item \"%s\"):\n",key);
                php_var_dump(tmp, 0);
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

    if (found && (create_new || 0 == zend_hash_num_elements(HASH_OF(tmp)))) {
        zval empty_array;
        array_init(&empty_array);

        add_next_index_zval(tmp, &empty_array);
        zend_hash_internal_pointer_end(HASH_OF(tmp));

        if (BLITZ_DEBUG) {
            php_printf("[debug] found, will add new iteration\n");
            php_var_dump(tmp, 0);
        }
    }

    tpl->last_iteration = blitz_hash_get_current_data(HASH_OF(tmp));
    if (!tpl->last_iteration) {
        blitz_error(tpl, E_WARNING,
            "INTERNAL ERROR: unable fetch last_iteration in blitz_iterate_by_path");
    }

    if (is_current_iteration) {
        tpl->current_iteration = tpl->last_iteration; /* was: fetch from tmp */
    }

    if (BLITZ_DEBUG) {
        php_printf("Iteration pointers: %p %p %p\n", tpl->current_iteration, tpl->current_iteration_parent, tpl->last_iteration);
        tpl->current_iteration ? php_var_dump(tpl->current_iteration,1) : php_printf("current_iteration is empty\n");
        tpl->current_iteration_parent ? php_var_dump(tpl->current_iteration_parent,1) : php_printf("current_iteration_parent is empty\n");
        tpl->last_iteration ? php_var_dump(tpl->last_iteration,1) : php_printf("last_iteration is empty\n");
    }

    return 1;
}
/* }}} */

static int blitz_find_iteration_by_path(blitz_tpl *tpl, const char *path, int path_len,
    zval **iteration, zval **iteration_parent) /* {{{ */
{
    zval *tmp, *entry;
    register int i = 1;
    int ilast = 1, j = 0, k = 0, key_len = 0;
    register const char *p = path;
    register int pmax = path_len;
    char buffer[BLITZ_CONTEXT_PATH_MAX_LEN];
    char *key = NULL;
    zend_string *tmp_key;
    zval *dummy;
    zend_ulong index = 0;

    k = pmax - 1;
    tmp = &tpl->iterations;
    key = buffer;

    /* check root */
    if (BLITZ_DEBUG) php_printf("D:-0 %s(%d)\n", path, path_len);

    if (*p == '/' && pmax == 1) {
        if (0 == zend_hash_num_elements(HASH_OF(tmp))) {
            blitz_populate_root(tpl);
        }
        *iteration = NULL;
        zend_hash_internal_pointer_end(HASH_OF(tmp));
        entry = blitz_hash_get_current_data(HASH_OF(tmp));
        if (!entry) {
            return 0;
        }
        INDIRECT_RETURN(entry, 0);
        *iteration = entry;
        *iteration_parent = &tpl->iterations;
        return 1;
    }

    if (i >= pmax) {
        return 0;
    }

    p++;
    if (BLITZ_DEBUG) php_printf("%d/%d\n", i, pmax);
    while (i < pmax) {
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
            zend_hash_internal_pointer_end(HASH_OF(tmp));
            if (zend_hash_get_current_key(HASH_OF(tmp), &tmp_key, &index) == HASH_KEY_IS_LONG) {
                entry = blitz_hash_get_current_data(HASH_OF(tmp));
                if (!entry) {
                    return 0;
                }
                INDIRECT_RETURN(entry, 0);
                if (BLITZ_DEBUG) {
                    php_printf("moving to the last array element:\n");
                    php_var_dump(entry, 0);
                }

                tmp = blitz_hash_str_find_ind(HASH_OF(entry), key, key_len);
                if (!tmp) {
                    return 0;
                }
            } else {
                tmp = blitz_hash_str_find_ind(HASH_OF(tmp), key, key_len);
                if (!tmp) {
                    return 0;
                }
            }

            if (BLITZ_DEBUG) {
                php_printf("key %s: we are here:\n", key);
                php_var_dump(tmp, 0);
            }

            ilast = i + 1;
        }
        ++p;
        ++i;
    }

    /* can be not an array (tried to iterate scalar) */
    if (IS_ARRAY != Z_TYPE_P(tmp)) {
        blitz_error(tpl, E_WARNING, "ERROR: unable to find context '%s', "
            "it was set as \"scalar\" value - check your iteration params", path);
        return 0;
    }

    zend_hash_internal_pointer_end(HASH_OF(tmp));

    /* if not array - stop searching, otherwise get the latest data */
    if (zend_hash_get_current_key(HASH_OF(tmp), &tmp_key, &index) == HASH_KEY_IS_STRING) {
        *iteration = tmp;
        *iteration_parent = tmp;
        return 1;
    }

    dummy = blitz_hash_get_current_data(HASH_OF(tmp));
    if (dummy) {
        INDIRECT_RETURN(dummy, 0);
        if (BLITZ_DEBUG) php_printf("%p %p\n", dummy, iteration);
        *iteration = dummy;
        *iteration_parent = tmp;
    } else {
        return 0;
    }

    if (BLITZ_DEBUG) {
        php_printf("%p %p\n", dummy, iteration );
        php_printf("parent:\n");
        php_var_dump(*iteration_parent,0);
        php_printf("found:\n");
        php_var_dump(*iteration,0);
    }

    return 1;
}
/* }}} */

static void blitz_build_fetch_index_node(blitz_tpl *tpl, blitz_node *node, const char *path, unsigned int path_len) /* {{{ */
{
    unsigned int current_path_len = 0;
    char current_path[BLITZ_MAX_FETCH_INDEX_KEY_SIZE] = "";
    char *lexem = NULL;
    unsigned int lexem_len = 0;
    zval temp;
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

        ZVAL_LONG(&temp, node->pos_in_list);
        zend_hash_str_update(tpl->static_data.fetch_index, current_path, current_path_len, &temp);
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

static int blitz_build_fetch_index(blitz_tpl *tpl) /* {{{ */
{
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
        zval *elem;
        zend_string *key;
        zend_ulong index;
        HashTable *ht = tpl->static_data.fetch_index;

        zend_hash_internal_pointer_reset(ht);
        php_printf("fetch_index dump:\n");

        while ((elem = blitz_hash_get_current_data(ht)) != NULL) {
            if (zend_hash_get_current_key(ht, &key, &index) != HASH_KEY_IS_STRING) {
                zend_hash_move_forward(ht);
                continue;
            }
            INDIRECT_CONTINUE_FORWARD(ht, elem);
            php_printf("key: %s(%u)\n", key->val, key->len);
            php_var_dump(elem, 0);
            zend_hash_move_forward(ht);
        }
    }

    return 1;
}
/* }}} */

static inline int blitz_touch_fetch_index(blitz_tpl *tpl) /* {{{ */
{
    if (!(tpl->flags & BLITZ_FLAG_FETCH_INDEX_BUILT)) {
        if (blitz_build_fetch_index(tpl)) {
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
    zval *input_params, smart_str *result)
{
    blitz_node *i_node = NULL;
    unsigned long result_alloc_len = 0;
    zval *z;

    if ((path[0] == '/') && (path_len == 1)) {
        return blitz_exec_template(tpl, id, result);
    }

    if (!blitz_touch_fetch_index(tpl)) {
        return 0;
    }

    /* find node by path   */
    z = blitz_hash_str_find_ind(tpl->static_data.fetch_index, (char *)path, path_len);
    if (z) {
        i_node = tpl->static_data.nodes + Z_LVAL_P(z);
    } else {
        blitz_error(tpl, E_WARNING, "cannot find context %s in template %s", path, tpl->static_data.name);
        return 0;
    }

    /* fetch result */
    if (i_node->first_child) {
        result_alloc_len = i_node->pos_end_shift - i_node->pos_begin_shift;
        smart_str_alloc(result, result_alloc_len, 0);
        smart_str_0(result);

        return blitz_exec_nodes(tpl, i_node->first_child, id,
            result, &result_alloc_len,
            i_node->pos_begin_shift, i_node->pos_end_shift,
            input_params, input_params
        );
    } else {
        unsigned long rlen = i_node->pos_end_shift - i_node->pos_begin_shift;
        smart_str_appendl(result, ZSTR_VAL(tpl->static_data.body.s) + i_node->pos_begin_shift, rlen);
    }

    return 1;
}
/* }}} */

static inline int blitz_iterate_current(blitz_tpl *tpl) /* {{{ */
{

    if (BLITZ_DEBUG) php_printf("[debug] BLITZ_FUNCTION: blitz_iterate_current, path=%s\n", tpl->current_path);

    blitz_iterate_by_path(tpl, tpl->current_path, strlen(tpl->current_path), 1, 1);
    tpl->last_iteration = tpl->current_iteration;

    return 1;
}
/* }}} */

static inline int blitz_prepare_iteration(blitz_tpl *tpl, const char *path, int path_len, int iterate_nonexistant) /* {{{ */
{
    int res = 0;

    if (BLITZ_DEBUG) php_printf("[debug] BLITZ_FUNCTION: blitz_prepare_iteration, path=%s\n", path);

    if (path_len == 0) {
        return blitz_iterate_current(tpl);
    } else {
        int current_len = strlen(tpl->current_path);
        int norm_len = 0;
        res = blitz_normalize_path(tpl, &tpl->tmp_buf, path, path_len, tpl->current_path, current_len);

        if (!res) return 0;
        norm_len = strlen(tpl->tmp_buf);

        /* check if path exists */
        if (norm_len>1) {
            zval *dummy = NULL;
            if (!blitz_touch_fetch_index(tpl)) {
                return 0;
            }

            if ((0 == iterate_nonexistant)
                && (dummy = blitz_hash_str_find_ind(tpl->static_data.fetch_index, tpl->tmp_buf, norm_len)) == NULL)
            {
                return -1;
            }
        }

        if (tpl->current_iteration_parent
            && (current_len == norm_len)
            && (0 == strncmp(tpl->tmp_buf,tpl->current_path,norm_len)))
        {
            return blitz_iterate_current(tpl);
        } else {
            return blitz_iterate_by_path(tpl, tpl->tmp_buf, norm_len, 0, 1);
        }
    }

    return 0;
}
/* }}} */

static inline int blitz_merge_iterations_by_str_keys(zval *target, zval *input) /* {{{ */
{
    zval *elem;
    HashTable *input_ht = NULL;
    zend_string *key = NULL;
    zend_ulong index = 0;

    if (!input || (IS_ARRAY != Z_TYPE_P(input)) || (IS_ARRAY != Z_TYPE_P(target))) {
        return 0;
    }

    if (0 == zend_hash_num_elements(HASH_OF(input))) {
        return 1;
    }

    input_ht = HASH_OF(input);
    while ((elem = blitz_hash_get_current_data(input_ht)) != NULL) {
        if (zend_hash_get_current_key(input_ht, &key, &index) != HASH_KEY_IS_STRING) {
            zend_hash_move_forward(input_ht);
            continue;
        }
        INDIRECT_CONTINUE_FORWARD(input_ht, elem);

        if (key && key->len) {
            zval_add_ref(elem);
            zend_hash_str_update(HASH_OF(target), key->val, key->len, elem);
        }
        zend_hash_move_forward(input_ht);
    }

    return 1;
}
/* }}} */

static inline int blitz_merge_iterations_by_num_keys(zval *target, zval *input) /* {{{ */
{
    zval *elem;
    HashTable *input_ht = NULL;
    zend_string *key = NULL;
    zend_ulong index = 0;

    if (!input || (IS_ARRAY != Z_TYPE_P(input))) {
        return 0;
    }

    if (0 == zend_hash_num_elements(HASH_OF(input))) {
        return 1;
    }

    input_ht = HASH_OF(input);
    while ((elem = blitz_hash_get_current_data(input_ht)) != NULL) {
        if (zend_hash_get_current_key(input_ht, &key, &index) != HASH_KEY_IS_LONG) {
            zend_hash_move_forward(input_ht);
            continue;
        }
        INDIRECT_CONTINUE_FORWARD(input_ht, elem);

        zval_add_ref(elem);
        zend_hash_index_update(HASH_OF(target), index, elem);
        zend_hash_move_forward(input_ht);
    }

    return 1;
}
/* }}} */

static inline int blitz_merge_iterations_set(blitz_tpl *tpl, zval *input_arr) /* {{{ */
{
    HashTable *input_ht = NULL;
    zend_string *key = NULL;
    zend_ulong index = 0;
    int is_current_iteration = 0, first_key_type = 0;
    zval *target_iteration;

    if (0 == zend_hash_num_elements(HASH_OF(input_arr))) {
        return 0;
    }

    /* *** DATA STRUCTURE FORMAT *** */
    /* set works differently for numerical keys and string keys: */
    /*     (1) STRING: set(array('a' => 'a_val')) will update current_iteration keys */
    /*     (2) LONG: set(array(0=>array('a'=>'a_val'))) will reset current_iteration_parent */
    input_ht = HASH_OF(input_arr);
    zend_hash_internal_pointer_reset(input_ht);
    first_key_type = zend_hash_get_current_key(input_ht, &key, &index);

    /* *** FIXME *** */
    /* blitz_iterate_by_path here should have is_current_iteration = 1 ALWAYS. */
    /* BUT for some reasons this broke tests. Until the bug is found for the backward compatibility */
    /* is_current_iteration is 0 for string key set and 1 for numerical (otherwise current_iteration_parrent */
    /* is not set properly) in blitz_iterate_by_path */

    /*    is_current_iteration = 1; */
    if (HASH_KEY_IS_LONG == first_key_type)
        is_current_iteration = 1;

    if (!tpl->current_iteration) {
        blitz_iterate_by_path(tpl, tpl->current_path, strlen(tpl->current_path), is_current_iteration, 0);
    } else {
        /* fix last_iteration: if we did iterate('/some') before and now set in '/', */
        /* then current_iteration is not empty but last_iteration points to '/some' */
        tpl->last_iteration = tpl->current_iteration;
    }

    if (IS_ARRAY == Z_TYPE_P(tpl->last_iteration))
        zend_hash_internal_pointer_reset(HASH_OF(tpl->last_iteration));

    if (HASH_KEY_IS_STRING == first_key_type) {
        target_iteration = tpl->last_iteration;
        blitz_merge_iterations_by_str_keys(target_iteration, input_arr);
    } else {
        if (!tpl->current_iteration_parent) {
            blitz_error(tpl, E_WARNING, "INTERNAL ERROR: unable to set into current_iteration_parent, is NULL");
            return 0;
        }
        target_iteration = tpl->current_iteration_parent;
        zend_hash_clean(HASH_OF(target_iteration));
        tpl->current_iteration = NULL;  /* parent was cleaned */
        blitz_merge_iterations_by_num_keys(target_iteration, input_arr);
    }

    return 1;
}
/* }}} */

static void blitz_register_constants(INIT_FUNC_ARGS) /* {{{ */
{
    //basic types
    REGISTER_LONG_CONSTANT("BLITZ_TYPE_VAR", BLITZ_TYPE_VAR, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_TYPE_METHOD", BLITZ_TYPE_METHOD, BLITZ_CONSTANT_FLAGS);

    //arg type constants
    REGISTER_LONG_CONSTANT("BLITZ_ARG_TYPE_VAR", BLITZ_ARG_TYPE_VAR, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_ARG_TYPE_VAR_PATH", BLITZ_ARG_TYPE_VAR_PATH, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_ARG_TYPE_STR", BLITZ_ARG_TYPE_STR, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_ARG_TYPE_NUM", BLITZ_ARG_TYPE_NUM, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_ARG_TYPE_FALSE", BLITZ_ARG_TYPE_FALSE, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_ARG_TYPE_TRUE", BLITZ_ARG_TYPE_TRUE, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_ARG_TYPE_FLOAT", BLITZ_ARG_TYPE_FLOAT, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_ARG_TYPE_EXPR_SHIFT", BLITZ_ARG_TYPE_EXPR_SHIFT, BLITZ_CONSTANT_FLAGS);

    //expr type constants
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_GE", BLITZ_EXPR_OPERATOR_GE, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_G", BLITZ_EXPR_OPERATOR_G, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_LE", BLITZ_EXPR_OPERATOR_LE, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_L", BLITZ_EXPR_OPERATOR_L, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_NE", BLITZ_EXPR_OPERATOR_NE, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_E", BLITZ_EXPR_OPERATOR_E, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_LA", BLITZ_EXPR_OPERATOR_LA, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_LO", BLITZ_EXPR_OPERATOR_LO, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_N", BLITZ_EXPR_OPERATOR_N, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_LP", BLITZ_EXPR_OPERATOR_LP, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_EXPR_OPERATOR_RP", BLITZ_EXPR_OPERATOR_RP, BLITZ_CONSTANT_FLAGS);

    //node type constants
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_COMMENT", BLITZ_NODE_TYPE_COMMENT, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_IF", BLITZ_NODE_TYPE_IF, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_UNLESS", BLITZ_NODE_TYPE_UNLESS, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_INCLUDE", BLITZ_NODE_TYPE_INCLUDE, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_END", BLITZ_NODE_TYPE_END, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_CONTEXT", BLITZ_NODE_TYPE_CONTEXT, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_LITERAL", BLITZ_NODE_TYPE_LITERAL, BLITZ_CONSTANT_FLAGS);

    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_WRAPPER_ESCAPE", BLITZ_NODE_TYPE_WRAPPER_ESCAPE, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_WRAPPER_DATE", BLITZ_NODE_TYPE_WRAPPER_DATE, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_WRAPPER_UPPER", BLITZ_NODE_TYPE_WRAPPER_UPPER, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_WRAPPER_LOWER", BLITZ_NODE_TYPE_WRAPPER_LOWER, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_WRAPPER_TRIM", BLITZ_NODE_TYPE_WRAPPER_TRIM, BLITZ_CONSTANT_FLAGS);

    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_IF_CONTEXT", BLITZ_NODE_TYPE_IF_CONTEXT, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_UNLESS_CONTEXT", BLITZ_NODE_TYPE_UNLESS_CONTEXT, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_ELSEIF_CONTEXT", BLITZ_NODE_TYPE_ELSEIF_CONTEXT, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_ELSE_CONTEXT", BLITZ_NODE_TYPE_ELSE_CONTEXT, BLITZ_CONSTANT_FLAGS);

    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_VAR", BLITZ_NODE_TYPE_VAR, BLITZ_CONSTANT_FLAGS);
    REGISTER_LONG_CONSTANT("BLITZ_NODE_TYPE_VAR_PATH", BLITZ_NODE_TYPE_VAR_PATH, BLITZ_CONSTANT_FLAGS);
}
/* }}} */

/**********************************************************************************************************************
* Blitz CLASS methods
**********************************************************************************************************************/

/* {{{ proto new Blitz([string filename]) */
static PHP_FUNCTION(blitz_init)
{
    blitz_tpl *tpl = NULL;
    size_t filename_len = 0;
    char *filename = NULL;
    zend_resource *rsrc;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(),"|s", &filename, &filename_len)) {
        return;
    }

    if (filename_len >= MAXPATHLEN) {
        blitz_error(NULL, E_WARNING, "Filename exceeds the maximum allowed length of %d characters", MAXPATHLEN);
        RETURN_FALSE;
    }

    if (getThis() && zend_hash_str_exists(Z_OBJPROP_P(getThis()), "tpl", sizeof("tpl")-1)) {
        blitz_error(tpl, E_WARNING, "ERROR: the object has already been initialized");
        RETURN_FALSE;
    }

    /* initialize template  */
    if (!(tpl = blitz_init_tpl(filename, filename_len, NULL, NULL, NULL))) {
        RETURN_FALSE;
    }

    if (tpl->static_data.body.s && ZSTR_LEN(tpl->static_data.body.s)) {
        /* analize template  */
        if (!blitz_analize(tpl)) {
            blitz_free_tpl(tpl);
            RETURN_FALSE;
        }
    }

    rsrc = zend_register_resource(tpl, le_blitz);
    add_property_resource(getThis(), "tpl", rsrc);
}
/* }}} */

/* {{{ proto bool Blitz->load(string body) */
static PHP_FUNCTION(blitz_load)
{
    blitz_tpl *tpl;
    zend_string *body;
    zval *id, *desc;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (BLITZ_CALLED_USER_METHOD(tpl)) RETURN_FALSE;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "S", &body)) {
        return;
    }

    if (ZSTR_LEN(tpl->static_data.body.s)) {
        blitz_error(tpl, E_WARNING, "INTERNAL ERROR: template is already loaded");
        RETURN_FALSE;
    }

    /* load body */
    if (!blitz_load_body(tpl, body)) {
        RETURN_FALSE;
    }

    /* analize template */
    if (!blitz_analize(tpl)) {
        RETURN_FALSE;
    }

    if (tpl->error == NULL) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }

}
/* }}} */

/* {{{ proto bool Blitz->dumpStruct(void) */
static PHP_FUNCTION(blitz_dump_struct)
{
    zval *id, *desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    php_blitz_dump_struct(tpl);

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto array Blitz->getTokens(void) */
static PHP_FUNCTION(blitz_get_tokens)
{
    zval *id, *desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    array_init(return_value);

    php_blitz_get_tokens(tpl, return_value);
}
/* }}} */

/* {{{ proto void Blitz->getStruct(void) */
static PHP_FUNCTION(blitz_get_struct)
{
    zval *id, *desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    array_init(return_value);

    blitz_get_path_list(tpl, return_value, 0, 0);
}
/* }}} */

/* {{{ proto array Blitz->getIterations(void) */
static PHP_FUNCTION(blitz_get_iterations)
{
    zval *id, *desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    ZVAL_COPY_VALUE(return_value, &tpl->iterations);
    zval_copy_ctor(return_value);
}
/* }}} */

/* {{{ proto bool Blitz->setGlobals(array values) */
static PHP_FUNCTION(blitz_set_global)
{
    zval *id, *desc, *input_arr, *elem;
    blitz_tpl *tpl;
    HashTable *input_ht;
    zend_string *key;
    zend_ulong index;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(),"a", &input_arr)) {
        return;
    }

    input_ht = HASH_OF(input_arr);
    if (0 == zend_hash_num_elements(input_ht)) {
        return;
    }

    zend_hash_internal_pointer_reset(tpl->hash_globals);
    zend_hash_internal_pointer_reset(input_ht);

    while ((elem = blitz_hash_get_current_data(input_ht)) != NULL) {
        if (zend_hash_get_current_key(input_ht, &key, &index) != HASH_KEY_IS_STRING) {
            zend_hash_move_forward(input_ht);
            continue;
        }

        INDIRECT_CONTINUE_FORWARD(input_ht, elem);

        /* disallow empty keys */
        if (!key || !key->len) {
            zend_hash_move_forward(input_ht);
            continue;
        }

        zval_add_ref(elem);
        zend_hash_str_update(tpl->hash_globals, key->val, key->len, elem);
        zend_hash_move_forward(input_ht);
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto array Blitz->getGlobals(void) */
static PHP_FUNCTION(blitz_get_globals)
{
    zval *id, *desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    array_init(return_value);
    zend_hash_copy(HASH_OF(return_value), tpl->hash_globals, (copy_ctor_func_t) zval_add_ref);
}
/* }}} */

/* {{{ proto array Blitz->cleanGlobals(void) */
static PHP_FUNCTION(blitz_clean_globals)
{
    zval *id, *desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    zend_hash_clean(tpl->hash_globals);

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Blitz->hasContext(string context) */
static PHP_FUNCTION(blitz_has_context)
{
    zval *id, *desc;
    blitz_tpl *tpl;
    char *path;
    size_t path_len, norm_len = 0, current_len = 0;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "s", &path, &path_len)) {
        return;
    }

    if (path_len == 1 && path[0] == '/') {
        RETURN_TRUE;
    }

    current_len = strlen(tpl->current_path);
    if (!blitz_normalize_path(tpl, &tpl->tmp_buf, path, path_len, tpl->current_path, current_len)) {
        RETURN_FALSE;
    }
    norm_len = strlen(tpl->tmp_buf);

    if (!blitz_touch_fetch_index(tpl)) {
        RETURN_FALSE;
    }

    /* find node by path */
    if (zend_hash_str_exists(tpl->static_data.fetch_index, tpl->tmp_buf, norm_len)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
/* }}} */

/* {{{ proto string Blitz->parse([array iterations]) */
static PHP_FUNCTION(blitz_parse)
{
    zval *id, *desc, *input_arr = NULL;
    blitz_tpl *tpl;
    smart_str result = {NULL, 0};
    int res;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);
    if (BLITZ_CALLED_USER_METHOD(tpl)) RETURN_FALSE;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "|a", &input_arr)) {
        return;
    }

    if (!ZSTR_LEN(tpl->static_data.body.s)) { /* body was not loaded */
        RETURN_EMPTY_STRING();
    }

    if (input_arr && 0 < zend_hash_num_elements(HASH_OF(input_arr))) {
        if (!blitz_merge_iterations_set(tpl, input_arr)) {
            RETURN_FALSE;
        }
    }

    res = blitz_exec_template(tpl, id, &result);

    if (res) {
        RETVAL_STR(result.s);
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto string Blitz->display([array iterations]) */
static PHP_FUNCTION(blitz_display)
{
    zval *id, *desc, *input_arr = NULL;
    blitz_tpl *tpl;
    smart_str result = {NULL, 0};
    int res;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);
    if (BLITZ_CALLED_USER_METHOD(tpl)) RETURN_FALSE;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "|a", &input_arr)) {
        return;
    }

    if (!ZSTR_LEN(tpl->static_data.body.s)) { /* body was not loaded */
        RETURN_EMPTY_STRING();
    }

    if (input_arr && 0 < zend_hash_num_elements(HASH_OF(input_arr))) {
        if (!blitz_merge_iterations_set(tpl, input_arr)) {
            RETURN_FALSE;
        }
    }

    res = blitz_exec_template(tpl, id, &result);

    if (res) {
        PHPWRITE(ZSTR_VAL(result.s), ZSTR_LEN(result.s));
        smart_str_free(&result);
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto string Blitz->context(string path) */
static PHP_FUNCTION(blitz_context)
{
    zval *id, *desc;
    blitz_tpl *tpl;
    char *path;
    size_t current_len, norm_len = 0, path_len, res;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "s", &path, &path_len)) {
        return;
    }

    current_len = strlen(tpl->current_path);
    RETVAL_STRINGL(tpl->current_path, current_len);

    if (path && path_len == current_len && 0 == strncmp(path, tpl->current_path, path_len)) {
        return;
    }

    res = blitz_normalize_path(tpl, &tpl->tmp_buf, path, path_len, tpl->current_path, current_len);
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
    zval *id, *desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);
    RETVAL_STRING(tpl->current_path);
}
/* }}} */

/* {{{ proto bool Blitz->iterate([string path [, mixed nonexistent]]) */
static PHP_FUNCTION(blitz_iterate)
{
    zval *id, *desc, *p_iterate_nonexistant = NULL;
    blitz_tpl *tpl;
    char *path = NULL;
    size_t path_len = 0;
    int iterate_nonexistant = 0;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(),"|sz", &path, &path_len, &p_iterate_nonexistant)) {
        return;
    }

    if (p_iterate_nonexistant) {
         BLITZ_ZVAL_NOT_EMPTY(p_iterate_nonexistant, iterate_nonexistant);
    }

    if (blitz_prepare_iteration(tpl, path, path_len, iterate_nonexistant)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool Blitz->set(array input) */
static PHP_FUNCTION(blitz_set)
{
    zval *id, *desc, *input_arr;
    blitz_tpl *tpl;
    int res;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "a", &input_arr)) {
        return;
    }

    res = blitz_merge_iterations_set(tpl, input_arr);

    if (res) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool Blitz->block(mixed p1 [, mixed p2[, mixed nonexistent]]) */
static PHP_FUNCTION(blitz_block) {
    zval *id, *desc;
    blitz_tpl *tpl;
    zval *p1, *p2 = NULL, *p_iterate_nonexistant = NULL, *input_arr = NULL;
    char *path = NULL;
    size_t path_len = 0;
    int iterate_nonexistant = 0;
    int res = 0;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(),"z|zz", &p1, &p2, &p_iterate_nonexistant)) {
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
        blitz_error(tpl, E_WARNING, "first pararmeter can be only NULL, string or array");
        RETURN_FALSE;
    }

    if (p_iterate_nonexistant) {
        BLITZ_ZVAL_NOT_EMPTY(p_iterate_nonexistant, iterate_nonexistant)
    }

    res = blitz_prepare_iteration(tpl, path, path_len, iterate_nonexistant);
    if (res == -1) { /* context doesn't exist */
        RETURN_TRUE;
    } else if (res == 0) { /* error */
        RETURN_FALSE;
    }

    /* copy params array to current iteration */
    if (input_arr && zend_hash_num_elements(HASH_OF(input_arr)) > 0) {
        if (tpl->last_iteration) {
            zval_dtor(tpl->last_iteration);
            ZVAL_COPY_VALUE(tpl->last_iteration, input_arr);
            zval_copy_ctor(tpl->last_iteration);
        } else {
            blitz_error(tpl, E_WARNING,
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
    size_t filename_len;
    zval *desc, *id;
    smart_str result = {NULL, 0};
    zval *input_arr = NULL;
    zval *iteration_params = NULL;
    int res = 0;
    int do_merge = 0;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "s|a", &filename, &filename_len, &input_arr)) {
        return;
    }

    if (!filename) RETURN_FALSE;

    if (input_arr && (IS_ARRAY == Z_TYPE_P(input_arr)) && (0 < zend_hash_num_elements(HASH_OF(input_arr)))) {
        do_merge = 1;
    } else {
        // we cannot mix this with merging (blitz_merge_iterations_set),
        // because merging is writing to the parent iteration set
        if (tpl->caller_iteration)
            iteration_params = tpl->caller_iteration;
    }

    if (!blitz_include_tpl_cached(tpl, filename, filename_len, iteration_params, &itpl))
        RETURN_FALSE;

    if (do_merge) {
        if (!blitz_merge_iterations_set(itpl, input_arr)) {
            RETURN_FALSE;
        }
    }

    res = blitz_exec_template(itpl, id, &result);

    if (res) {
        RETVAL_STR(result.s);
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto string Blitz->fetch(string path, array params) */
static PHP_FUNCTION(blitz_fetch)
{
    zval *id, *desc;
    blitz_tpl *tpl;
    smart_str result = {NULL, 0};
    char exec_status = 0;
    char *path;
    size_t path_len, norm_len, res, current_len = 0;
    zval *input_arr = NULL, *elem;
    HashTable *input_ht = NULL;
    zend_string *key = NULL;
    zend_ulong index = 0;
    zval *path_iteration = NULL;
    zval *path_iteration_parent = NULL;
    zval *final_params = NULL;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(),"s|a", &path, &path_len, &input_arr)) {
        return;
    }

    /* find corresponding iteration data */
    res = blitz_normalize_path(tpl, &tpl->tmp_buf, path, path_len, tpl->current_path, current_len);
    current_len = strlen(tpl->current_path);
    norm_len = strlen(tpl->tmp_buf);

    /* 2DO: using something like current_iteration and current_iteration_parent can speed up next step,  */
    /* but for other speed-up purposes it's not guaranteed that current_iteration and current_iteration_parent  */
    /* point to really current values. that's why we cannot just check tpl->current_path == tpl->tmp_buf and use them */
    /* we always find iteration by path instead */
    res = blitz_find_iteration_by_path(tpl, tpl->tmp_buf, norm_len, &path_iteration, &path_iteration_parent);
    if (!res) {
        if (BLITZ_DEBUG) php_printf("blitz_find_iteration_by_path failed!\n");
        final_params = input_arr;
    } else {
        if (BLITZ_DEBUG) php_printf("blitz_find_iteration_by_path: pi=%p ti=%p &ti=%p\n", path_iteration, tpl->iterations, &tpl->iterations);
        final_params = path_iteration;
    }

    if (BLITZ_DEBUG) {
        php_printf("tpl->iterations:\n");
        php_var_dump(&tpl->iterations, 1);
    }

    /* merge data with input params */
    if (input_arr && path_iteration) {
        if (BLITZ_DEBUG) php_printf("merging current iteration and params in fetch\n");
        input_ht = HASH_OF(input_arr);

        ZEND_HASH_FOREACH_STR_KEY_VAL_IND(input_ht, key, elem) {
            INDIRECT_CONTINUE_FORWARD(input_ht, elem);

            if (key && key->len) {
                zval_add_ref(elem);
                zend_hash_str_update(HASH_OF(path_iteration), key->val, key->len, elem);
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (BLITZ_DEBUG) {
        php_printf("tpl->iterations:\n");
        php_var_dump(&tpl->iterations, 1);
        php_printf("final_params:\n");
        if (final_params) php_var_dump(final_params,1);
    }

    exec_status = blitz_fetch_node_by_path(tpl, id, tpl->tmp_buf, norm_len, final_params, &result);

    if (exec_status) {
        RETVAL_STR(result.s);

        /* clean-up parent path iteration after the fetch */
        if (path_iteration_parent) {
            zend_hash_internal_pointer_end(HASH_OF(path_iteration_parent));
            if (HASH_KEY_IS_LONG == zend_hash_get_current_key(HASH_OF(path_iteration_parent), &key, &index)) {
                /*tpl->last_iteration = NULL; */
                zend_hash_index_del(HASH_OF(path_iteration_parent),index);
            } else {
                zend_hash_clean(HASH_OF(path_iteration_parent));
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
    zval *id, *desc;
    blitz_tpl *tpl;
    char *path = NULL;
    size_t path_len = 0, norm_len = 0, res = 0, current_len = 0;
    zval *path_iteration = NULL, *path_iteration_parent = NULL, *warn_param = NULL;
    int warn_not_found = 1;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "|sz", &path, &path_len, &warn_param)) {
        return;
    }

    /* warn_param = FALSE: throw no warning when iteration is not found. All other cases - do please. */
    if (warn_param && (IS_FALSE == Z_TYPE_P(warn_param))) {
        warn_not_found = 0;
    }

    /* find corresponding iteration data */
    res = blitz_normalize_path(tpl, &tpl->tmp_buf, path, path_len, tpl->current_path, current_len);

    current_len = strlen(tpl->current_path);
    norm_len = strlen(tpl->tmp_buf);

    res = blitz_find_iteration_by_path(tpl, tpl->tmp_buf, norm_len, &path_iteration, &path_iteration_parent);
    if (!res) {
        if (warn_not_found) {
            php_error(E_WARNING, "unable to find iteration by path %s", tpl->tmp_buf);
            RETURN_FALSE;
        } else {
            RETURN_TRUE;
        }
    }

    /* clean-up parent iteration */
    zend_hash_clean(HASH_OF(path_iteration_parent));

    /* reset current iteration pointer if it's current iteration  */
    if ((current_len == norm_len) && (0 == strncmp(tpl->tmp_buf, tpl->current_path, norm_len))) {
        tpl->current_iteration = NULL;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ string Blitz->getError() */
static PHP_FUNCTION(blitz_get_error)
{
    zval *id, *desc;
    blitz_tpl *tpl;

    BLITZ_FETCH_TPL_RESOURCE(id, tpl, desc);
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "")) {
        return;
    }

    if (tpl) {
        if (tpl->error) {
            RETVAL_STRING(tpl->error);
        } else {
            RETURN_FALSE;
        }
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ arginfo */

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_init, 0, 0, 0)
    ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_load, 0, 0, 1)
    ZEND_ARG_INFO(0, body)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_dump_struct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_get_tokens, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_get_struct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_get_iterations, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_set_global, 0, 0, 1)
    ZEND_ARG_INFO(0, values)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_get_globals, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_clean_globals, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_has_context, 0, 0, 1)
    ZEND_ARG_INFO(0, context)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_parse, 0, 0, 0)
    ZEND_ARG_INFO(0, iterations)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_display, 0, 0, 0)
    ZEND_ARG_INFO(0, iterations)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_context, 0, 0, 1)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_get_context, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_iterate, 0, 0, 0)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, nonexistent)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_set, 0, 0, 1)
    ZEND_ARG_INFO(0, input)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_block, 0, 0, 1)
    ZEND_ARG_INFO(0, p1)
    ZEND_ARG_INFO(0, p2)
    ZEND_ARG_INFO(0, nonexistent)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_include, 0, 0, 1)
    ZEND_ARG_INFO(0, input)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_fetch, 0, 0, 1)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_clean, 0, 0, 0)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, warn_not_found)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blitz_get_error, 0, 0, 0)
ZEND_END_ARG_INFO()

/* }}} */

#define BLITZ_ALIAS(method, func) PHP_FALIAS(method, func, arginfo_ ## func)

/* {{{ blitz_functions[] : Blitz class */
static const zend_function_entry blitz_functions[] = {
    BLITZ_ALIAS(__construct,         blitz_init)
    BLITZ_ALIAS(blitz,               blitz_init)
    BLITZ_ALIAS(load,                blitz_load)
    BLITZ_ALIAS(dump_struct,         blitz_dump_struct)
    BLITZ_ALIAS(get_struct,          blitz_get_struct)
    BLITZ_ALIAS(get_iterations,      blitz_get_iterations)
    BLITZ_ALIAS(get_context,         blitz_get_context)
    BLITZ_ALIAS(has_context,         blitz_has_context)
    BLITZ_ALIAS(set_global,          blitz_set_global)
    BLITZ_ALIAS(set_globals,         blitz_set_global)
    BLITZ_ALIAS(get_globals,         blitz_get_globals)
    BLITZ_ALIAS(clean_globals,       blitz_clean_globals)
    BLITZ_ALIAS(set,                 blitz_set)
    BLITZ_ALIAS(assign,              blitz_set)
    BLITZ_ALIAS(parse,               blitz_parse)
    BLITZ_ALIAS(display,             blitz_display)
    BLITZ_ALIAS(include,             blitz_include)
    BLITZ_ALIAS(iterate,             blitz_iterate)
    BLITZ_ALIAS(context,             blitz_context)
    BLITZ_ALIAS(block,               blitz_block)
    BLITZ_ALIAS(fetch,               blitz_fetch)
    BLITZ_ALIAS(clean,               blitz_clean)
    BLITZ_ALIAS(dumpstruct,          blitz_dump_struct)
    BLITZ_ALIAS(gettokens,           blitz_get_tokens)
    BLITZ_ALIAS(getstruct,           blitz_get_struct)
    BLITZ_ALIAS(getiterations,       blitz_get_iterations)
    BLITZ_ALIAS(hascontext,          blitz_has_context)
    BLITZ_ALIAS(getcontext,          blitz_get_context)
    BLITZ_ALIAS(setglobal,           blitz_set_global)
    BLITZ_ALIAS(setglobals,          blitz_set_global)
    BLITZ_ALIAS(getglobals,          blitz_get_globals)
    BLITZ_ALIAS(cleanglobals,        blitz_clean_globals)
    BLITZ_ALIAS(geterror,            blitz_get_error)
    BLITZ_ALIAS(get_error,           blitz_get_error)
    {NULL, NULL, NULL}
};
/* }}} */

PHP_MINIT_FUNCTION(blitz) /* {{{ */
{
    ZEND_INIT_MODULE_GLOBALS(blitz, php_blitz_init_globals, NULL);
    REGISTER_INI_ENTRIES();

    le_blitz = zend_register_list_destructors_ex(blitz_resource_dtor, NULL, "Blitz template", module_number);

    INIT_CLASS_ENTRY(blitz_class_entry, "blitz", blitz_functions);
    zend_register_internal_class(&blitz_class_entry);
    blitz_register_constants(INIT_FUNC_ARGS_PASSTHRU);

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
    php_info_print_table_row(2, "Blitz version", BLITZ_VERSION_STRING);
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
