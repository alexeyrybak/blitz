dnl $Id$

define(MAX_LEXEM_VALUE, 512)
AC_ARG_WITH(max-lexem,
  [AS_HELP_STRING([--with-max-lexem=MAX_LEXEM_VALUE],
                  [Define max lexem length.])],
  MAX_LEXEM_VALUE
)

if test "$with_max_lexem" != ""; then
  AC_DEFINE_UNQUOTED(BLITZ_MAX_LEXEM_LEN, $with_max_lexem, [Maximum length of lexem.])
else
  AC_DEFINE_UNQUOTED(BLITZ_MAX_LEXEM_LEN, MAX_LEXEM_VALUE, [Maximum length of lexem.]) 
fi

PHP_ARG_ENABLE(blitz, whether to enable blitz support,
[  --enable-blitz          Enable blitz support])

if test "$PHP_BLITZ" != "no"; then
  PHP_NEW_EXTENSION(blitz, blitz.c blitz_debug.c, $ext_shared)
fi
