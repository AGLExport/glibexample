# autoconf sample

AC_PREREQ([2.69])

AC_INIT([exsample], [1.0])

AM_INIT_AUTOMAKE([foreign subdir-objects])

#AC_CONFIG_SRCDIR([src/exsample.cpp])

AC_CONFIG_HEADERS([include/config.h])

AC_CONFIG_MACRO_DIR([m4])


AC_ARG_ENABLE([address-sanitizer],
  [AS_HELP_STRING([--enable-address-sanitizer], [Enable address sanitizer in build time (for debug, default is no)])],
  [:],
  [enable_address_sanitizer=no])
AM_CONDITIONAL([ENABLE_ADDRESS_SANITIZER], [test "$enable_address_sanitizer" = "yes"])

AC_ARG_ENABLE([gcov],
  [AS_HELP_STRING([--enable-gcov], [Enable gcov in build time (for debug, default is no)])],
  [:],
  [enable_gcov=no])
AM_CONDITIONAL([ENABLE_GCOV], [test "$enable_gcov" = "yes"])

AC_ARG_ENABLE([test],
  [AS_HELP_STRING([--enable-test], [Enable unit test build (requir to gtest and gmock, default is no)])],
  [:],
  [enable_test=no])
AM_CONDITIONAL([ENABLE_TEST], [test "$enable_test" = "yes"])


# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_DISABLE_STATIC

# Checks for libraries.
LT_INIT
AM_PROG_LIBTOOL 

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([GLIB2], [glib-2.0])
PKG_CHECK_MODULES([GIO2], [gio-2.0])
PKG_CHECK_MODULES([GTEST_MAIN], [gtest_main], , enable_test=no)
PKG_CHECK_MODULES([GMOCK_MAIN], [gmock_main], , enable_test=no)



# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile 
				lib/Makefile
				src/Makefile
				test/Makefile ])
AC_OUTPUT

