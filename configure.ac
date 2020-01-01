AC_INIT([tcg-boot], [0.1])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])

AC_PROG_CC

AC_CHECK_TOOL([OBJCOPY], [objcopy], [:])

AS_IF([test x$OBJCOPY == x], [AC_MSG_ERROR([cannot find objcopy])])

AC_SUBST([efibindir], ['${pkgdatadir}/efi'])
AC_SUBST([efiarch], [$(uname -m)])

AM_CONDITIONAL([X86_64], [test x$efiarch = xx86_64])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/tcg-loader/Makefile
])

AC_OUTPUT
