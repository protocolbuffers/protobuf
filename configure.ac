## Process this file with autoconf to produce configure.
## In general, the safest way to proceed is to run ./autogen.sh

AC_PREREQ(2.59)

# Note:  If you change the version, you must also update it in:
# * java/pom.xml
# * python/setup.py
# * src/google/protobuf/stubs/common.h
# * src/Makefile.am (Update -version-info for LDFLAGS if needed)
AC_INIT([protobuf],[2.0.4-SNAPSHOT],[protobuf@googlegroups.com])

AC_CONFIG_SRCDIR(src/google/protobuf/message.cc)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
ACX_USE_SYSTEM_EXTENSIONS
AC_PROG_LIBTOOL
AM_CONDITIONAL(GCC, test "$GCC" = yes)   # let the Makefile know if we're gcc
AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])

AS_IF([test "$SUNCC" = "yes"],[
  CFLAGS="$CFLAGS -xO4 -xlibmil -xdepend -Xa -mt -xstrconst -D_FORTEC_"
  CXXFLAGS="$CXXFLAGS -xO4 -xlibmil -mt -D_FORTEC_ -xlang=c99 -compat=5 -library=stlport4 -template=no%extdef"
])


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdlib.h unistd.h])

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_STRTOD
AC_CHECK_FUNCS([ftruncate memset mkdir strchr strerror strtol])

ACX_PTHREAD
AC_CXX_STL_HASH

AC_CONFIG_FILES([Makefile src/Makefile ])
AC_OUTPUT
