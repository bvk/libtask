dnl -*- autoconf -*-
dnl
dnl Libtask: A thread-safe coroutine library.
dnl
dnl Copyright (C) 2013  BVK Chaitanya
dnl
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl

AC_INIT([libtask],[0.1])
AC_PREREQ(2.60)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_RANLIB

AC_CHECK_PROGS(BUILDCC, [gcc], [:])
AC_CHECK_TOOLS(HOSTCC, [gcc], [:])
AC_CHECK_TARGET_TOOLS(TARGETCC, [gcc], [:])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([libtask/Makefile])

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
