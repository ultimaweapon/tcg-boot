AC_INIT([tcg-boot], [0.1])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])

AC_PROG_CC

AC_SUBST([efibindir], ['${pkgdatadir}/efi'])
AC_SUBST([efiarch], [$(uname -m)])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/loader/Makefile
])

AC_OUTPUT
