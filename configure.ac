AC_PREREQ(2.61)
AC_INIT([libprocreact], m4_esyscmd([echo -n $(cat ./version)$VERSION_SUFFIX]))
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([1.9 tar-pax -Wall -Werror -Wno-extra-portability])

# Checks for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

# Output
AC_CONFIG_FILES([
Makefile
src/Makefile
src/libprocreact/Makefile
tests/Makefile
])
AC_OUTPUT
