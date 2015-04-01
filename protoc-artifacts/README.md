# Build scripts that publish pre-compiled protoc artifacts
``protoc`` is the compiler for ``.proto`` files. It generates language bindings
for the messages and/or RPC services from ``.proto`` files.

Because ``protoc`` is a native executable, the scripts under this directory
build and publish a ``protoc`` executable (a.k.a. artifact) to Maven
repositories. The artifact can be used by build automation tools so that users
would not need to compile and install ``protoc`` for their systems.

## Versioning
The version of the ``protoc`` artifact must be the same as the version of the
Protobuf project.

## Artifact name
The name of a published ``protoc`` artifact is in the following format:
``protoc-<version>-<os>-<arch>.exe``, e.g., ``protoc-3.0.0-alpha-3-windows-x86_64.exe``.

## System requirement
Install [Apache Maven](http://maven.apache.org/) if you don't have it.

The scripts only work under Unix-like environments, e.g., Linux, MacOSX, and
Cygwin or MinGW for Windows. Please see ``README.md`` of the Protobuf project
for how to set up the build environment.

## To install artifacts locally
The following command will install the ``protoc`` artifact to your local Maven repository.
```
$ mvn install
```

## To push artifacts to Maven Central
Before you can upload artifacts to Maven Central repository, make sure you have
read [this page](http://central.sonatype.org/pages/apache-maven.html) on how to
configure GPG and Sonatype account

Use the following command to upload artifacts:
```
$ mvn clean deploy -P release
```

