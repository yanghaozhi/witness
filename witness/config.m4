dnl $Id$
dnl config.m4 for extension witness

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(witness, for witness support,
dnl Make sure that the comment is aligned:
dnl [  --with-witness             Include witness support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(witness, whether to enable witness support,
Make sure that the comment is aligned:
[  --enable-witness           Enable witness support])

PHP_ARG_ENABLE(debug, whether to enable debugging support in witness,
[  --enable-debug              Enable debugging support in witness], no, no)

PHP_ARG_ENABLE(gcov, whether to enable gcov support in witness,
[  --enable-gcov               Enable gcov support in witness], no, no)

if test "$PHP_WITNESS" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-witness -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/witness.h"  # you most likely want to change this
  dnl if test -r $PHP_WITNESS/$SEARCH_FOR; then # path given as parameter
  dnl   WITNESS_DIR=$PHP_WITNESS
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for witness files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       WITNESS_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$WITNESS_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the witness distribution])
  dnl fi

  dnl # --with-witness -> add include path
  dnl PHP_ADD_INCLUDE($WITNESS_DIR/include)

  dnl # --with-witness -> check for lib and symbol presence
  dnl LIBNAME=witness # you may want to change this
  dnl LIBSYMBOL=witness # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $WITNESS_DIR/lib, WITNESS_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_WITNESSLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong witness lib version or lib not found])
  dnl ],[
  dnl   -L$WITNESS_DIR/lib -lm
  dnl ])
  dnl
  PHP_SUBST(WITNESS_SHARED_LIBADD)

  if test "$PHP_DEBUG" != "no"; then
    CFLAGS+=" -O0 -DDEBUG "
  fi

  if test "$PHP_GCOV" != "no"; then
    CFLAGS+=" -fprofile-arcs -ftest-coverage -g3 "
    LDFLAGS+=" -lgcov "
  fi

  DIRS="./include ../include ../../include"
  for dir in $DIRS ; do 
    if test -f "$dir/php_common.h" ; then 
      CFLAGS+=" -I$dir " 
      break
    fi
  done

  CFLAGS+=" -std=gnu99 "
  LDFLAGS+=" -lsqlite3 "

  PHP_NEW_EXTENSION(witness, php_witness.c, $ext_shared)
fi
