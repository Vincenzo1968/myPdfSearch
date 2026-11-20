AC_PREREQ([2.69])

AC_INIT([mypdfsearch],[ 1.0.0],[vincenzolocicero68@gmail.com])
AC_CONFIG_SRCDIR([src/mypdf_api.c])
AC_CONFIG_FILES([Makefile src/Makefile mypdfsearch.pc
                 examples/Makefile
                 examples/c/console/Makefile
                 examples/c/gtk4/Makefile
                 examples/cpp/console/Makefile])

AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

dnl Shared and static library (Libtool). 'win32-dll' enables Windows DLLs.
LT_INIT([win32-dll])

AC_CHECK_HEADERS([zlib.h openssl/evp.h], [],
    [AC_MSG_ERROR([Required development header not found])])

AC_CHECK_LIB([z], [inflate], [],
    [AC_MSG_ERROR([zlib library not found])])

AC_CHECK_LIB([crypto], [EVP_sha256], [],
    [AC_MSG_ERROR([OpenSSL libcrypto library not found])])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [build with debugging symbols and without optimizations])],
    [enable_debug="$enableval"],
    [enable_debug=no])

AS_IF([test "x$enable_debug" != "xyes" &&
       test "x$enable_debug" != "xno"],
    [AC_MSG_ERROR([--enable-debug accepts only yes or no])])

AC_ARG_ENABLE([mytime],
    [AS_HELP_STRING([--enable-mytime],
        [enable the MYTIME execution-time instrumentation])],
    [enable_mytime="$enableval"],
    [enable_mytime=no])

AS_IF([test "x$enable_mytime" != "xyes" &&
       test "x$enable_mytime" != "xno"],
    [AC_MSG_ERROR([--enable-mytime accepts only yes or no])])

dnl Set optimization defaults only when the user did not supply CFLAGS.
AS_IF([test "x$ac_env_CFLAGS_set" != "xset"], [
    AS_IF([test "x$enable_debug" = "xyes"],
        [CFLAGS="-O0 -g"],
        [CFLAGS="-O3"])
])

AC_ARG_ENABLE([examples],
    [AS_HELP_STRING([--enable-examples],
        [also build the optional examples (GTK4 and C++); the console example is always built])],
    [enable_examples="$enableval"],
    [enable_examples=no])

AS_IF([test "x$enable_examples" != "xyes" &&
       test "x$enable_examples" != "xno"],
    [AC_MSG_ERROR([--enable-examples accepts only yes or no])])

have_gtk4=no
have_cxx=no
AS_IF([test "x$enable_examples" = "xyes"], [
    PKG_CHECK_MODULES([GTK4], [gtk4 >= 4.0], [have_gtk4=yes],
        [AC_MSG_WARN([GTK4 not found: the GTK4 example will not be built])])
    AS_IF([test -n "$CXX" && $CXX --version >/dev/null 2>&1], [have_cxx=yes],
        [AC_MSG_WARN([no C++ compiler found: the C++ example will not be built])])
])

AM_CONDITIONAL([BUILD_EXAMPLE_GTK4], [test "x$have_gtk4" = "xyes"])
AM_CONDITIONAL([BUILD_EXAMPLE_CPP], [test "x$have_cxx" = "xyes"])

AM_CONDITIONAL([DEBUG_BUILD], [test "x$enable_debug" = "xyes"])
AM_CONDITIONAL([SHOW_TIME], [test "x$enable_mytime" = "xyes"])

dnl On Windows a program that links the static library must define MYPDF_STATIC
dnl (see mypdfsearch.h): without a DLL there is nothing to import.
AM_CONDITIONAL([MYPDF_STATIC_ONLY], [test "x$enable_shared" = "xno"])

AC_OUTPUT
