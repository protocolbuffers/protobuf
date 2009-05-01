dnl Check for the presence of the Sun Studio compiler.
dnl If Sun Studio compiler is found, set appropriate flags.
dnl Additionally, Sun Studio doesn't default to 64-bit by itself,
dnl nor does it automatically look in standard Solaris places for
dnl 64-bit libs, so we must add those options and paths to the search
dnl paths.

dnl TODO(kenton):  This is pretty hacky.  It sets CXXFLAGS, which the autoconf
dnl docs say should never be overridden except by the user.  It also isn't
dnl cross-compile safe.  We should fix these problems, but since I don't have
dnl Sun CC at my disposal for testing, someone else will have to do it.

AC_DEFUN([ACX_CHECK_SUNCC],[

  AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])

  AS_IF([test "$SUNCC" = "yes"],[
    isainfo_k=`isainfo -k`
    AS_IF([test "$target_cpu" = "sparc"],[
      MEMALIGN_FLAGS="-xmemalign=8s"
      IS_64="-m64"
      LDFLAGS="${LDFLAGS} -L/usr/lib/${isainfo_k} -L/usr/local/lib/${isainfo_k}"
    ],[
      AS_IF([test "$isainfo_k" = "amd64"],[
        IS_64="-m64"
        LDFLAGS="${LDFLAGS} -L/usr/lib/${isainfo_k} -L/usr/local/lib/${isainfo_k}"
      ])
    ])

    CFLAGS="-g -xO4 -xlibmil -xdepend -Xa -mt -xstrconst ${IS_64} ${MEMALIGN_FLAGS} $CFLAGS"
    CXXFLAGS="-g -xO4 -xlibmil -mt ${IS_64} ${MEMALIGN_FLAGS} -xlang=c99 -compat=5 -library=stlport4 -template=no%extdef $CXXFLAGS"
  ])
])
