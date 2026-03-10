# Publish pre-compiled protoc artifacts
``protoc`` is the compiler for ``.proto`` files. It generates language bindings
for the messages and/or RPC services from ``.proto`` files.

Because ``protoc`` is a native executable, the scripts under this directory
publish a ``protoc`` executable (a.k.a. artifact) to Maven repositories. The
artifact can be used by build automation tools so that users would not need to
compile and install ``protoc`` for their systems.

If you would like us to publish protoc artifact for a new platform, please
open an issue to request it.

## Maven Location
The published protoc artifacts are available on Maven here:

    https://repo.maven.apache.org/maven2/com/google/protobuf/protoc/

## Versioning
The version of the ``protoc`` artifact must be the same as the version of the
Protobuf project.

## Artifact name
The name of a published ``protoc`` artifact is in the following format:
``protoc-<version>-<os>-<arch>.exe``, e.g., ``protoc-3.6.1-linux-x86_64.exe``.

Note that artifacts for linux/macos also have the `.exe` suffix but they are
not windows binaries.

## System requirement
Install [Apache Maven](http://maven.apache.org/) if you don't have it.

The scripts only work under Unix-like environments, e.g., Linux, MacOSX, and
Cygwin or MinGW for Windows. Please see ``README.md`` of the Protobuf project
for how to set up the build environment.

## Tested build environments
We have successfully built artifacts on the following environments:
- Linux x86_32 and x86_64:
  - Centos 6.9 (within Docker 1.6.1)
  - Ubuntu 14.04.5 64-bit
- Linux aarch_64: Cross compiled with `g++-aarch64-linux-gnu` on Ubuntu 14.04.5 64-bit
- Mac OS X x86_32 and x86_64: Mac OS X 10.9.5
