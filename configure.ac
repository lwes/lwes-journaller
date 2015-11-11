dnl Process this file with autoconf to produce a configure script.

AC_INIT([lwes-journaller], [0.2.1], [lwes-devel@lists.sourceforge.net])
AM_INIT_AUTOMAKE

dnl Determine the host type for the host specific inclusion below
AC_CANONICAL_HOST

dnl --- Variables I use later on --

SHORT_DESC="The LWES (Light Weight Event System) is a system for sending structured self describing events over multicast.  This is a journaller implementation that receives events and saves them to disk."
MAJOR_VERSION=`echo "[$]PACKAGE_VERSION" |
               perl -ne 'm%^(\d+)\.% && print "[$]1"'`
MINOR_VERSION=`echo "[$]PACKAGE_VERSION" |
               perl -ne 'm%^\d+\.(\d+)% && print "[$]1"'`
RELEASE_NUMBER=`echo "[$]PACKAGE_VERSION" |
                perl -ne 'm%^\d+\.\d+\.(\d+)% && print "[$]1"'`
MAJOR_VERSION_UNDERLINE=`echo "[$]MAJOR_VERSION" | perl -pe 'chomp; s/\W/_/g;'`
PACKAGE_UNDERLINE=`echo "[$]PACKAGE_NAME" | perl -pe 'chomp; s/\W/_/g;'`
PACKAGEPACKED=`echo "[$]PACKAGE_NAME" | perl -pe 'chomp; s/\W//g;'`
VERSION_UNDERLINE=`echo "[$]PACKAGE_VERSION" | perl -pe 'chomp; s/\W/_/g;'`

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(RELEASE_NUMBER)
AC_SUBST(SHORT_DESC)

AC_SUBST(MAJOR_VERSION_UNDERLINE)
AC_SUBST(PACKAGE_UNDERLINE)
AC_SUBST(PACKAGEPACKED)
AC_SUBST(VERSION_UNDERLINE)

dnl -- set maintainer mode
AM_MAINTAINER_MODE
AC_SUBST(USE_MAINTAINER_MODE)

dnl -- we want a header to include in our source files with configure
dnl    info
AM_CONFIG_HEADER(src/config.h)

dnl -- make sure we have a C compiler and libtool
AC_PROG_CC
AM_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h sys/time.h unistd.h getopt.h)
AC_CHECK_HEADER(valgrind/valgrind.h,
                AC_DEFINE([HAVE_VALGRIND_HEADER],
                          [1],
                          [Define to 1 if the <valgrind/valgrind.h> header is on the system]))
AC_CHECK_HEADER(pthread.h,
  AC_CHECK_LIB(pthread, pthread_join,[
   THREAD_LIBS="-lpthread"
   AC_DEFINE([HAVE_LIBPTHREAD], [1], [Define if pthread library is there (-lpthread)])
   AC_DEFINE([HAVE_PTHREAD_H], [1], [Define if <pthread.h> is there])
   WITH_THREADS="1"]))
AC_SUBST(THREAD_LIBS)
AC_CHECK_HEADER(zlib.h,
  AC_CHECK_LIB(z, gzopen,[
   Z_LIBS="-lz"
   AC_DEFINE([HAVE_LIBZ], [1], [Define if zlib library is there (-lz)])
   AC_DEFINE([HAVE_LIBZ_H], [1], [Define if <zlib.h> is there])
   WITH_ZLIB="1"]))
AC_SUBST(Z_LIBS)

dnl Check for LWES installed
PKG_CHECK_MODULES([LWES], [lwes-0 >= 0.23.1])
AC_SUBST(LWES_CFLAGS)
AC_SUBST(LWES_LIBS)

dnl Check for mondemand installed, if requested
AC_ARG_WITH([mondemand], AS_HELP_STRING([--with-mondemand],
                          [Build with support for mondemand logging and statistics (default: disabled)]))
if test "x$with_mondemand" == "xyes"; then
  PKG_CHECK_MODULES([MONDEMAND], [mondemand-4.0 >= 4.0.2])
  AC_SUBST(MONDEMAND_CFLAGS)
  AC_SUBST(MONDEMAND_LIBS)
  AC_DEFINE(HAVE_MONDEMAND, 1, [System supports mondemand])
fi

dnl Check for messaging
AC_MSG_CHECKING(for sys/msg support)
AC_TRY_COMPILE([
                #include <sys/msg.h>
                ],[ ],g_have_sys_msg=yes,g_have_sys_msg=no)
AC_MSG_RESULT($g_have_sys_msg)
if test "x$g_have_sys_msg" = "xyes" ; then
  AC_DEFINE(HAVE_SYS_MSG_H, 1, [System supports sys/msg.h])
fi

dnl Check for mqueue
AC_MSG_CHECKING(for mqueue support)
AC_TRY_COMPILE([
                #include <mqueue.h>
                ],[ ],g_have_mqueue=yes,g_have_mqueue=no)
AC_MSG_RESULT($g_have_mqueue)
if test "x$g_have_mqueue" = "xyes" ; then
  AC_DEFINE(HAVE_MQUEUE_H, 1, [System supports mqueue.h])
fi
AC_CHECK_LIB(rt, main)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Check for SO_REUSEPORT
AC_CHECK_DECL([SO_REUSEPORT],
              AC_DEFINE(HAVE_SO_REUSEPORT,1,[Define if kernel supports SO_REUSEPORT]),
              ,
              [
               #include <sys/types.h>
               #include <sys/socket.h>
              ])

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gettimeofday socket strerror)

dnl These are mostly for solaris
AC_CHECK_LIB(socket,main)
AC_CHECK_LIB(nsl,main)
AC_CHECK_LIB(xnet,main)
AC_CHECK_LIB(resolv,main)

dnl syslog check
AC_CHECK_FUNCS(syslog)
AC_CHECK_LIB(syslog,main)

dnl allow for an external gettimeofday function, mostly useful for people have
dnl reimplemented gettimeofday because the system call is slow (FreeBSD 4.11)
AC_ARG_ENABLE(external-gettimeofday,
              [  --enable-external-gettimeofday=<header>  Provide an external definition of the gettimeofday function.  The header pointed to should contain a macro which defines GETTIMEOFDAY(t,tz).  The header will be included in the appropriate place, so that your macro is substituted],
              [AC_DEFINE([HAVE_EXTERNAL_GETTIMEOFDAY], [1], [Define to 1 if there is an external gettimeofday defined])
               AC_DEFINE_UNQUOTED([EXTERNAL_GETTIMEOFDAY_HEADER], "$enable_external_gettimeofday", [Header for external gettime of day])
               ],
              AC_MSG_WARN(using system gettimeofday))

dnl  -- determine the type of varargs this platform supports --
# check for flavours of varargs macros
AC_MSG_CHECKING(for ISO C99 varargs macros in C)
AC_TRY_COMPILE([],[
int a(int p1, int p2, int p3);
#define call_a(...) a(1,__VA_ARGS__)
call_a(2,3);
],g_have_iso_c_varargs=yes,g_have_iso_c_varargs=no)
AC_MSG_RESULT($g_have_iso_c_varargs)
if test "x$g_have_iso_c_varargs" = "xyes" ; then
  AC_DEFINE(HAVE_ISO_C_VARARGS, 1, [ISO C varargs])
fi

AC_MSG_CHECKING(for GNUC varargs macros)
AC_TRY_COMPILE([],[
int a(int p1, int p2, int p3);
#define call_a(params...) a(1,params)
call_a(2,3);
],g_have_gnuc_varargs=yes,g_have_gnuc_varargs=no)
AC_MSG_RESULT($g_have_gnuc_varargs)
if test "x$g_have_gnuc_varargs" = "xyes" ; then
  AC_DEFINE(HAVE_GNUC_C_VARARGS, 1, [GNUC varargs])
fi


# --- Coverage hooks ---

AC_ARG_ENABLE(coverage,
              [  --enable-coverage      turn on -fprofile-arcs -ftest-coverage],              [case "${enableval}" in
                yes) ENABLE_COVERAGE=1 ;;
                no) ENABLE_COVERAGE=0 ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --enable-cov) ;;
              esac],
              [ENABLE_COVERAGE=2])

if test "x[$]ENABLE_COVERAGE" = "x1"; then
  AC_MSG_WARN(enable coverage)
  CFLAGS="`echo \"[$]CFLAGS\" | perl -pe 's/-O\d+//g;'` -fprofile-arcs -ftest-coverage"
  CXXFLAGS="`echo \"[$]CXXFLAGS\" | perl -pe 's/-O\d+//g;'` -fprofile-arcs -ftest-coverage"
fi

AC_SUBST(ENABLE_COVERAGE)

# --- Compiler warnings ---
#
# for developer use, enable lots of compile warnings,
# but don't require this generally, because some system's
# header files (BSD) can't handle it
#
# NB: must add -Werror after AC_PROG_CC, etc., so do this last

AC_ARG_ENABLE(hardcore,
              [  --disable-hardcore      turn off -W -Wall -Werror],
              [case "${enableval}" in
                yes) ENABLE_HARDCORE=1 ;;
                no) ENABLE_HARDCORE=0 ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-hardcore) ;;              esac],
              [ENABLE_HARDCORE=1])

if test "x[$]ENABLE_HARDCORE" = "x1"; then
  AC_MSG_WARN(enable hardcore compile warnings)
  if test "x$CXX" = "x"; then
    dnl - only valid for C with newer gcc's
    CPPFLAGS="[$]CPPFLAGS -Wmissing-prototypes"
  fi
  CPPFLAGS="[$]CPPFLAGS -Werror -W -Wall -Wpointer-arith -Wcast-align -Wwrite-strings"
fi


AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile
                 doxygen.config])
AC_CONFIG_FILES([tests/test-wrapper.sh],
                [chmod +x tests/test-wrapper.sh])

AC_OUTPUT

