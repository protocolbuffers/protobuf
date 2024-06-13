prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: UTF8 Range
Description: Google's UTF8 Library
Version: 1.0
Requires: absl_strings
Libs: -L${libdir} -lutf8_validity -lutf8_range @CMAKE_THREAD_LIBS_INIT@
Cflags: -I${includedir}
