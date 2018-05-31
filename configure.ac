dnl Autoconf configure script for mt.

AC_PREREQ(2.69)dnl
AC_INIT(mt.c)

AC_CANONICAL_SYSTEM

AC_PROG_INSTALL

case "${target}" in
  *-cygwin)
  	;;
  *)
  	echo "The only supported target is cygwin."
	exit 1
	;;
esac

AC_CHECK_TOOL(CC, ${target_cpu}-pc-cygwin-gcc, ${target_cpu}-pc-cygwin-gcc)
if test -z "$CC"; then
  AC_CHECK_TOOL(CC, gcc, gcc)
  if test -z "$CC"; then
    AC_CHECK_TOOL(CC, cc, cc)
    test -z "$CC" && AC_MSG_ERROR([no acceptable c compiler found in \$PATH])
  fi
fi

test -z "$CFLAGS" && CFLAGS="-g -O2 -Wall -Werror"

VERSION=`sed -ne 's/^VERSION=\(".*"\)/\1/p' ${srcdir}/mt.cygport`

AC_SUBST(CFLAGS)
AC_SUBST(VERSION)

AC_OUTPUT(Makefile)
