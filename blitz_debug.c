#include "php.h"
#include "ext/standard/php_standard.h"

static int dump_array_values(void *pDest, int num_args, va_list args, zend_hash_key *hash_key);

void blitz_dump_ht(HashTable *ht, int depth) {
    php_printf("%*carray(%d) {\n", depth * 2, ' ', zend_hash_num_elements(ht));
    zend_hash_apply_with_arguments(ht, dump_array_values, 1, depth + 1);
    php_printf("%*c}\n", depth * 2, ' ');
}

static void dump_value(zval *zv, int depth) {
    if (Z_TYPE_P(zv) == IS_ARRAY) {
        php_printf("%*carray(%d) {\n", depth * 2, ' ', zend_hash_num_elements(Z_ARRVAL_P(zv)));
        zend_hash_apply_with_arguments(Z_ARRVAL_P(zv), dump_array_values, 1, depth + 1);
        php_printf("%*c}\n", depth * 2, ' ');
    } else {
        php_printf("%*c%Z\n", depth * 2, ' ', zv);
    }
}

static int dump_array_values(
    void *pDest, int num_args, va_list args, zend_hash_key *hash_key
) {
    zval *zv = (zval *) pDest;
    int depth = va_arg(args, int);

    if (hash_key->key == NULL) {
        php_printf("%*c[%ld]=>\n", depth * 2, ' ', hash_key->h);
    } else {
        php_printf("%*c[\"", depth * 2, ' ');
        PHPWRITE(ZSTR_VAL(hash_key->key), ZSTR_LEN(hash_key->key));
        php_printf("\"]=>\n");
    }

    php_var_dump(zv, depth);

    return ZEND_HASH_APPLY_KEEP;
}
