AC_INIT([xmlargs], [1.0])

AM_INIT_AUTOMAKE([foreign])

PKG_CHECK_MODULES([XML], [libxml-2.0])
# PKG_CHECK_MODULES([GRAPH], [libgraph])

AC_PROG_CXX()
AC_PROG_INSTALL()
AC_PROG_RANLIB()
AC_PROG_LN_S()

AC_CONFIG_FILES( [ \
  Makefile \
  docs/Makefile \
  src/Makefile \
  src/data/Makefile \
] )

AC_OUTPUT()
