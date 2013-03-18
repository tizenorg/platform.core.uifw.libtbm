#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AC_PREREQ(2.60)
AC_INIT(libtbm, 1.0.0)
AC_USE_SYSTEM_EXTENSIONS
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([dist-bzip2])

AM_CONFIG_HEADER([config.h])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC

AC_HEADER_STDC
AC_SYS_LARGEFILE
AC_FUNC_ALLOCA

# Enable quiet compiles on automake 1.11.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# set the dir for the tbm module
DEFAULT_BUFMGR_MODULE_PATH="/usr/lib/bufmgr"
AC_ARG_WITH(bufmgr-module-path, AS_HELP_STRING([--with-bufmgr-module-path=PATH], [tbm bufmgr module dir]),
				[ BUFMGR_MODULE_PATH="$withval" ],
				[ BUFMGR_MODULE_PATH="${DEFAULT_BUFMGR_MODULE_PATH}" ])
#AC_DEFINE(BUFMGR_MODULE_DIR, "${BUFMGR_MODULE_PATH}", [Directory for the modules of tbm_bufmgr])
AC_DEFINE_UNQUOTED(BUFMGR_MODULE_DIR, "${BUFMGR_MODULE_PATH}", [Directory for the modules of tbm_bufmgr])

AC_CHECK_FUNCS([clock_gettime], [CLOCK_LIB=],
               [AC_CHECK_LIB([rt], [clock_gettime], [CLOCK_LIB=-lrt],
                             [AC_MSG_ERROR([Couldn't find clock_gettime])])])
AC_SUBST([CLOCK_LIB])

PKG_CHECK_MODULES(PTHREADSTUBS, pthread-stubs)
PKG_CHECK_MODULES(LIBDRM, libdrm)

LIBTBM_CFLAGS="$PTHREADSTUBS_CFLAGS $LIBDRM_CFLAGS "
LIBTBM_LIBS="$PTHREADSTUBS_LIBS $LIBDRM_LIBS "
AC_SUBST(LIBTBM_CFLAGS)
AC_SUBST(LIBTBM_LIBS)

AC_OUTPUT([
    src/Makefile
    drm_slp/Makefile
	Makefile
	libtbm.pc])

echo ""
echo "CFLAGS            : $CFLAGS"
echo "LDFLAGS           : $LDFLAGS"
echo "LIBTBM_CFLAGS     : $LIBTBM_CFLAGS"
echo "LIBTBM_LIBS       : $LIBTBM_LIBS"
echo "BUFMGR_MODULE_DIR : $BUFMGR_MODULE_PATH"
echo ""

