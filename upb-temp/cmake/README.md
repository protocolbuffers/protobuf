
# upb CMake build (EXPERIMENTAL)

upb's CMake support is experimental. The core library builds successfully
under CMake, and this is verified by the Bazel tests in this directory.
However there is no support for building the upb compiler or for generating
.upb.c/upb.h files. This means upb's CMake support is incomplete at best,
unless your application is intended to be purely reflective.

If you find this CMake setup useful in its current state, please consider
filing an issue so we know. If you have suggestions for how it could be
more useful (and particularly if you can contribute some code for it)
please feel free to file an issue for that too. Do keep in mind that upb
does not currently provide any ABI stability, so we want to avoid providing
a shared library.

The CMakeLists.txt is generated from the Bazel BUILD files using the Python
scripts in this directory. We want to avoid having two separate sources of
truth that both need to be updated when a file is added or removed.

This directory also contains some generated files that would be created
on the fly during a Bazel build. These are automaticaly kept in sync by
the Bazel test `//cmake:test_generated_files`.
