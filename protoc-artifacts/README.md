# Build scripts that publish pre-compiled protoc artifacts
``protoc`` is the compiler for ``.proto`` files. It generates language bindings
for the messages and/or RPC services from ``.proto`` files.

Because ``protoc`` is a native executable, we build and publish a ``protoc``
executable (a.k.a. artifact) to Maven repositories. The artifact can be used by
build automation tools so that users would not need to compile and install
``protoc`` for their systems.

The [pom.xml](pom.xml) file specifies configuration details used by Maven to
publish the protoc binaries. This is only used internally for releases.

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
