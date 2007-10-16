PHP_ARG_ENABLE(blitz, whether to enable blitz support,
Make sure that the comment is aligned:
[  --enable-blitz           Enable blitz support])

if test "$PHP_BLITZ" != "no"; then
  LIBSYMBOL=blitz # you most likely want to change this 
  PHP_NEW_EXTENSION(blitz, blitz.c, $ext_shared)
fi
