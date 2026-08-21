This directory contains CI-specific tooling.

# Clang wrappers

CMake allows for compiler wrappers to be injected such as ccache, which
intercepts compiler calls and short-circuits on cache-hits.  This can be done
by specifying `CMAKE_C_COMPILER_LAUNCHER` and `CMAKE_CXX_COMPILER_LAUNCHER`
during CMake's configure step. Unfortunately, X-Code doesn't provide anything
like this, so we use basic wrapper scripts to invoke ccache + clang.

# Bazelrc files

In order to allow platform-specific `.bazelrc` flags during testing, we keep
3 different versions here along with a shared `common.bazelrc` that they all
include.  Our GHA infrastructure will select the appropriate file for any test
and overwrite the default `.bazelrc` in our workspace, which is intended for
development only.
