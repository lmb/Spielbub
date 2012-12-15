AC_INIT([spielbub], [0.1])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PREREQ([2.59])

AM_INIT_AUTOMAKE([no-define foreign subdir-objects])
AM_PATH_CHECK()

m4_ifdef(
    [AM_SILENT_RULES],
    [AM_SILENT_RULES([yes])]
)

PKG_CHECK_MODULES([SDL], [sdl])
PKG_CHECK_MODULES([ZeroMQ], [libzmq])
PKG_CHECK_MODULES([protobufc], [libprotobuf-c])
CFLAGS="$CFLAGS -std=gnu99 -O0 -Wall -Wextra -fno-inline"

AC_CHECK_HEADERS([SDL/SDL.h])
AC_CHECK_HEADERS([zmq.h])

AC_PATH_PROG(PROTOC, protoc-c, [no])
if test x"${PROTOC}" == x"no"; then
    AC_MSG_ERROR([The Protocol Buffers for C compiler protoc-c needs to be installed.])
fi

AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])

AC_OUTPUT
