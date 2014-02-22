AC_INIT([tgrey], [0.1], [florian@wagner-flo.net])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign subdir-objects
                       parallel-tests color-tests])

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_AR

AC_ARG_WITH(
    [db-path],
    AC_HELP_STRING([--with-db-path=PATH],
        [filename for the greylisting database [LOCALSTATEDIR/tgrey.tdb]]),
    [AC_SUBST([TGREY_DB], ["${withval}"])],
    [AC_SUBST([TGREY_DB], ["${localstatedir}/tgrey.tdb"])])

PKG_CHECK_MODULES([libtdb], [tdb >= 1.0.0])

AC_OUTPUT([Makefile])
