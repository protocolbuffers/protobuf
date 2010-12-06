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
AC_INIT([Protocol Buffers],[2.3.1-pre],[protobuf@googlegroups.com],[protobuf])


AC_CONFIG_SRCDIR(src/google/protobuf/message.cc)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# autoconf's default CXXFLAGS are usually "-g -O2".  These aren't necessarily
# the best choice for libprotobuf.
AS_IF([test "x${ac_cv_env_CFLAGS_set}" = "x"],
      [CFLAGS=""])
AS_IF([test "x${ac_cv_env_CXXFLAGS_set}" = "x"],
      [CXXFLAGS=""])

AC_CANONICAL_TARGET

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
AM_CONDITIONAL(GCC, test "$GCC" = yes)   # let the Makefile know if we're gcc

# test_util.cc takes forever to compile with GCC and optimization turned on.
AC_MSG_CHECKING([C++ compiler flags...])
AS_IF([test "x${ac_cv_env_CXXFLAGS_set}" = "x"],[
  AS_IF([test "$GCC" = "yes"],[
    PROTOBUF_OPT_FLAG="-O2"
    CXXFLAGS="${CXXFLAGS} -g"
  ])

  # Protocol Buffers contains several checks that are intended to be used only
  # for debugging and which might hurt performance.  Most users are probably
  # end users who don't want these checks, so add -DNDEBUG by default.
  CXXFLAGS="$CXXFLAGS -DNDEBUG"

  AC_MSG_RESULT([use default: $PROTOBUF_OPT_FLAG $CXXFLAGS])
],[
  AC_MSG_RESULT([use user-supplied: $CXXFLAGS])
])

AC_SUBST(PROTOBUF_OPT_FLAG)

ACX_CHECK_SUNCC

# Have to do libtool after SUNCC, other wise it "helpfully" adds Crun Cstd
# to the link
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdlib.h unistd.h])

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_STRTOD
AC_CHECK_FUNCS([ftruncate memset mkdir strchr strerror strtol])

# Check for zlib.
HAVE_ZLIB=0
AS_IF([test "$with_zlib" != no], [
  AC_MSG_CHECKING([zlib version])

  # First check the zlib header version.
  AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM([[
        #include <zlib.h>
        #if !defined(ZLIB_VERNUM) || (ZLIB_VERNUM < 0x1204)
        # error zlib version too old
        #endif
        ]], []), [
    AC_MSG_RESULT([ok (1.2.0.4 or later)])

    # Also need to add -lz to the linker flags and make sure this succeeds.
    AC_SEARCH_LIBS([zlibVersion], [z], [
      AC_DEFINE([HAVE_ZLIB], [1], [Enable classes using zlib compression.])
      HAVE_ZLIB=1
    ], [
      AS_IF([test "$with_zlib" != check], [
        AC_MSG_FAILURE([--with-zlib was given, but no working zlib library was found])
      ])
    ])
  ], [
    AS_IF([test "$with_zlib" = check], [
      AC_MSG_RESULT([headers missing or too old (requires 1.2.0.4)])
    ], [
      AC_MSG_FAILURE([--with-zlib was given, but zlib headers were not present or were too old (requires 1.2.0.4)])
    ])
  ])
])
AM_CONDITIONAL([HAVE_ZLIB], [test $HAVE_ZLIB = 1])

AS_IF([test "$with_protoc" != "no"], [
  PROTOC=$with_protoc
  AS_IF([test "$with_protoc" = "yes"], [
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

# HACK:  Make gtest's configure script pick up our copy of CFLAGS and CXXFLAGS,
#   since the flags added by ACX_CHECK_SUNCC must be used when compiling gtest
#   too.
export CFLAGS
export CXXFLAGS
AC_CONFIG_SUBDIRS([gtest])

AC_CONFIG_FILES([Makefile src/Makefile protobuf.pc protobuf-lite.pc])
AC_OUTPUT
