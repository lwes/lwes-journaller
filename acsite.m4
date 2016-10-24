# need to find the correct pthread_setname_np version
# found some macros here
# https://github.com/Kronuz/Xapiand/blob/master/m4/ax_pthread_set_name.m4
# with license
#
# Copyright (C) 2015,2016 deipi.com LLC and contributors. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

# AX_PTHREAD_SET_NAME();
# Check which variant (if any) of pthread_set_name_np we have.
# -----------------------------------------------------------------------------
AC_DEFUN([AX_PTHREAD_SET_NAME],
[
  old_CFLAGS=$CFLAGS
  old_LIBS=$LIBS
  CFLAGS="$CFLAGS -Werror"
  LIBS="$LIBS $THREAD_LIBS"
  # pthread setname (4 non-portable variants...)
  AC_CHECK_HEADERS([pthread_np.h])
  define(pthread_np_preamble,[
    #include <pthread.h>
    #if HAVE_PTHREAD_NP_H
    #  include <pthread_np.h>
    #endif
  ])
  # 2-arg setname (e.g. Linux/glibc, QNX, IBM)
  AC_MSG_CHECKING([for 2-arg pthread_setname_np])
  AC_LINK_IFELSE([AC_LANG_PROGRAM(pthread_np_preamble, [
      pthread_setname_np(pthread_self(), "foo")
  ])], [
    AC_DEFINE(HAVE_PTHREAD_SETNAME_NP_2, 1, [2-arg pthread_setname_np])
    AC_MSG_RESULT([yes])
  ], [
    AC_MSG_RESULT([no])
    # 2-arg set_name (e.g. FreeBSD, OpenBSD)
    AC_MSG_CHECKING([for 2-arg pthread_set_name_np])
    AC_LINK_IFELSE([AC_LANG_PROGRAM(pthread_np_preamble, [
        return pthread_set_name_np(pthread_self(), "foo");
    ])], [
      AC_DEFINE(HAVE_PTHREAD_SET_NAME_NP_2, 1, [2-arg pthread_set_name_np])
      AC_MSG_RESULT([yes])
    ], [
      AC_MSG_RESULT([no])
      # 1-arg setname (e.g. Darwin)
      AC_MSG_CHECKING([for 1-arg pthread_setname_np])
      AC_LINK_IFELSE([AC_LANG_PROGRAM(pthread_np_preamble, [
          return pthread_setname_np("foo");
      ])], [
        AC_DEFINE(HAVE_PTHREAD_SETNAME_NP_1, 1, [1-arg pthread_setname_np])
        AC_MSG_RESULT([yes])
      ], [
        AC_MSG_RESULT([no])
        # 3-arg setname (e.g. NetBSD)
        AC_MSG_CHECKING([for 3-arg pthread_setname_np])
        AC_LINK_IFELSE([AC_LANG_PROGRAM(pthread_np_preamble, [
            return pthread_setname_np(pthread_self(), "foo", NULL);
        ])], [
          AC_DEFINE(HAVE_PTHREAD_SETNAME_NP_3, 1, [3-arg pthread_setname_np])
          AC_MSG_RESULT([yes])
        ], [
          AC_MSG_RESULT([no])
        ])
      ])
    ])
  ])

  CFLAGS=$old_CFLAGS
  LIBS=$old_LIBS
])

AC_DEFUN([AX_PTHREAD_GET_NAME],
[
  # pthread getname (non-portable variants...)
  AC_CHECK_HEADERS([pthread_np.h])
  define(pthread_np_preamble,[
    #include <pthread.h>
    #if HAVE_PTHREAD_NP_H
    #  include <pthread_np.h>
    #endif
  ])
  # 3-arg getname (e.g. most platforms)
  AC_MSG_CHECKING([for 3-arg pthread_getname_np])
  AC_LINK_IFELSE([AC_LANG_PROGRAM(pthread_np_preamble, [
      char name[[16]];
      pthread_getname_np(pthread_self(), name, sizeof(name))
  ])], [
    AC_DEFINE(HAVE_PTHREAD_GETNAME_NP_3, 1, [3-arg pthread_getname_np])
    AC_MSG_RESULT([yes])
  ], [
    AC_MSG_RESULT([no])
    # 3-arg get_name (e.g. ??)
    AC_MSG_CHECKING([for 3-arg pthread_get_name_np])
    AC_LINK_IFELSE([AC_LANG_PROGRAM(pthread_np_preamble, [
        char name[[16]];
        pthread_get_name_np(pthread_self(), name, sizeof(name))
    ])], [
      AC_DEFINE(HAVE_PTHREAD_GET_NAME_NP_3, 1, [3-arg pthread_get_name_np])
      AC_MSG_RESULT([yes])
    ], [
      AC_MSG_RESULT([no])
      # 1-arg get_name (e.g. ??)
      AC_MSG_CHECKING([for 1-arg pthread_get_name_np])
      AC_LINK_IFELSE([AC_LANG_PROGRAM(pthread_np_preamble, [
          char* buffer = pthread_get_name_np(pthread_self())
      ])], [
        AC_DEFINE(HAVE_PTHREAD_GET_NAME_NP_1, 1, [1-arg pthread_get_name_np])
        AC_MSG_RESULT([yes])
      ], [
        AC_MSG_RESULT([no])
      ])
    ])
  ])
])
