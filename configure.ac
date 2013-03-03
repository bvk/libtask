# -*- autoconf -*-
#
# Copyright (C) 2012  BVK Chaitanya
#

AC_INIT([libtask],[0.1])
AC_PREREQ(2.60)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AC_CHECK_PROGS(BUILDCC, [gcc], [:])
AC_CHECK_TOOLS(HOSTCC, [gcc], [:])
AC_CHECK_TARGET_TOOLS(TARGETCC, [gcc], [:])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_OUTPUT
[
echo host: $host
echo host_cpu: $host_cpu
echo host_os: $host_os
echo host_alias: $host_alias
echo
echo target: $target
echo target_os: $target_os
echo target_cpu: $target_cpu
echo target_alias: $target_alias
echo
echo build: $build
echo build_os: $build_os
echo build_cpu: $build_cpu
echo build_alias: $build_alias
echo
echo cross: $cross_compiling
echo buildcc: $BUILDCC
echo hostcc: $HOSTCC
echo targetcc: $TARGETCC
]
