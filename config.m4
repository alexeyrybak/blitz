dnl $Id$

PHP_ARG_ENABLE(blitz, whether to enable blitz support,
[  --enable-blitz           Enable blitz support])

if test "$PHP_BLITZ" != "no"; then
  PHP_NEW_EXTENSION(blitz, blitz.c, $ext_shared)
fi
