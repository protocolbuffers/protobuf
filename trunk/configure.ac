## Process this file with autoconf to produce configure.
## In general, the safest way to proceed is to run ./autogen.sh

AC_PREREQ(2.59)

# Note:  If you change the version, you must also update it in:
# * java/pom.xml
# * python/setup.py
# * src/google/protobuf/stubs/common.h
# * src/Makefile.am (Update -version-info for LDFLAGS if needed)
#
# In the SVN trunk, the version should always be the next anticipated release
# version with the "-pre" suffix.  (We used to use "-SNAPSHOT" but this pushed
# the size of one file name in the dist tarfile over the 99-char limit.)
AC_INIT([Protocol Buffers],[2.1.1-pre],[protobuf@googlegroups.com],[protobuf])

# Detect whether the user specified their own compilation flags.  If so then
# we want to respect their decision, otherwise we will twiddle them later.
AS_IF([test "$CXXFLAGS" = ""],[
  protobuf_default_cxxflags=yes
])

AC_CONFIG_SRCDIR(src/google/protobuf/message.cc)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AC_ARG_WITH([zlib],
  [AS_HELP_STRING([--with-zlib],
    [include classes for streaming compressed data in and out @<:@default=check@:>@])],
  [],[with_zlib=check])

AC_ARG_WITH([protoc],
  [AS_HELP_STRING([--with-protoc=COMMAND],
    [use the given protoc command instead of building a new one when building tests (useful for cross-compiling)])],
  [],[with_protoc=no])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
ACX_USE_SYSTEM_EXTENSIONS
AC_PROG_LIBTOOL
AM_CONDITIONAL(GCC, test "$GCC" = yes)   # let the Makefile know if we're gcc

# autoconf's default CXXFLAGS are usually "-g -O2".  These aren't necessarily
# the best choice for libprotobuf.
AC_MSG_CHECKING([C++ compiler flags...])
AS_IF([test "$protobuf_default_cxxflags" = "yes"],[

  # test_util.cc takes forever to compile with GCC and optimization turned on.
  # But we cannot override anything that is part of CXXFLAGS since it is the
  # last thing added to the command line.  The automake docs insist that you
  # should never want to override CXXFLAGS because they represent the intent of
  # the user, and the user knows best.  But if the user actually did not set
  # any CXXFLAGS, then AC_PROG_CXX sets them to a rather arbitrary default.
  # That's not user intent at all, but automake still treats it like it is.
  # Grr.  Anyway, getting back to the point, this hack here strips out the -O
  # flag from autoconf's defaults and puts it into another variable so that
  # we can override it.  BTW, m4 escaping sucks.
  PROTOBUF_OPT_FLAG=`echo "$CXXFLAGS" | grep -o '\-O@<:@0-9@:>@\?'`
  CXXFLAGS=`echo "$CXXFLAGS" | sed -e 's/ \?-O@<:@0-9@:>@\?//g'`

  # Protocol Buffers contains several checks that are intended to be used only
  # for debugging and which might hurt performance.  Most users are probably
  # end users who don't want these checks, so add -DNDEBUG by default.
  CXXFLAGS="$CXXFLAGS -DNDEBUG"

  AC_MSG_RESULT([use default: $PROTOBUF_OPT_FLAG $CXXFLAGS])
],[
  PROTOBUF_OPT_FLAG=
  AC_MSG_RESULT([use user-supplied: $CXXFLAGS])
])

AC_SUBST(PROTOBUF_OPT_FLAG)

ACX_CHECK_SUNCC

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdlib.h unistd.h])

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_STRTOD
AC_CHECK_FUNCS([ftruncate memset mkdir strchr strerror strtol])

HAVE_ZLIB=0
AS_IF([test "$with_zlib" != no],
  [AC_SEARCH_LIBS([zlibVersion], [z],
    [AC_DEFINE([HAVE_ZLIB], [1], [Enable classes using zlib compression.])
     HAVE_ZLIB=1],
    [if test "$with_zlib" != check; then
      AC_MSG_FAILURE([--with-zlib was given, but test for zlib failed])
     fi])])
AM_CONDITIONAL([HAVE_ZLIB], [test $HAVE_ZLIB = 1])

AS_IF([test "$with_protoc" != "no"], [
  PROTOC=$with_protoc
  AS_IF([test "$with_protoc" == "yes"], [
    # No argument given.  Use system protoc.
    PROTOC=protoc
  ])
  AS_IF([echo "$PROTOC" | grep -q '^@<:@^/@:>@.*/'], [
    # Does not start with a slash, but contains a slash.  So, it's a relative
    # path (as opposed to an absolute path or an executable in $PATH).
    # Since it will actually be executed from the src directory, prefix with
    # the current directory.  We also insert $ac_top_build_prefix in case this
    # is a nested package and --with-protoc was actually given on the outer
    # package's configure script.
    PROTOC=`pwd`/${ac_top_build_prefix}$PROTOC
  ])
  AC_SUBST([PROTOC])
])
AM_CONDITIONAL([USE_EXTERNAL_PROTOC], [test "$with_protoc" != "no"])

ACX_PTHREAD
AC_CXX_STL_HASH

AC_CONFIG_SUBDIRS([gtest])

AC_CONFIG_FILES([Makefile src/Makefile ])
AC_OUTPUT
