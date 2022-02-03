# Build scripts that publish pre-compiled protoc artifacts
``protoc`` is the compiler for ``.proto`` files. It generates language bindings
for the messages and/or RPC services from ``.proto`` files.

Because ``protoc`` is a native executable, the scripts under this directory
build and publish a ``protoc`` executable (a.k.a. artifact) to Maven
repositories. The artifact can be used by build automation tools so that users
would not need to compile and install ``protoc`` for their systems.

If you would like us to publish protoc artifact for a new platform, please send
us a pull request to add support for the new platform. You would need to change
the following files:

* [build-protoc.sh](build-protoc.sh): script to cross-build the protoc for your
  platform.
* [pom.xml](pom.xml): script to upload artifacts to maven.
* [build-zip.sh](build-zip.sh): script to package published maven artifacts in
  our release page.

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

## Building from a freshly checked-out source

If you just checked out the Protobuf source from github, you need to
generate the configure script.

Under the protobuf project directory:


```
$ ./autogen.sh
```

### Build the artifact for each platform

Run the build-protoc.sh script under this protoc-artifacts directory to build the protoc
artifact for each platform.  For example:

```
$ cd protoc-artifacts
$ ./build-protoc.sh linux x86_64 protoc
```

The above command will produce a `target/linux/x86_64/protoc` binary under the
protoc-artifacts directory.

For a list of supported platforms, see the comments in the build-protoc.sh
script. We only use this script to build artifacts on Ubuntu and MacOS (both
with x86_64, and do cross-compilation for other platforms.

### Tips for building for Linux
We build on Centos 6.9 to provide a good compatibility for not very new
systems. We have provided a ``Dockerfile`` under this directory to build the
environment. It has been tested with Docker 1.6.1.

To build a image:

```
$ docker build -t protoc-artifacts .
```

To run the image:

```
$ docker run -it --rm=true protoc-artifacts bash
```

To checkout protobuf (run within the container):

```
$ # Replace v3.5.1 with the version you want
$ wget -O - https://github.com/protocolbuffers/protobuf/archive/v3.5.1.tar.gz | tar xvzp
```

### Windows build
We no longer use scripts in this directory to build windows artifacts. Instead,
we use Visual Studio 2015 to build our windows release artifacts. See our
[kokoro windows build scripts here](../kokoro/release/protoc/windows/build.bat).

To upload windows artifacts, copy the built binaries into this directory and
put it into the target/windows/(x86_64|x86_32) directory the same way as the
artifacts for other platforms. That will allow the maven script to find and
upload the artifacts to maven.

## To push artifacts to Maven Central
Before you can upload artifacts to Maven Central repository, make sure you have
read [this page](http://central.sonatype.org/pages/apache-maven.html) on how to
configure GPG and Sonatype account.

Before you do the deployment, make sure you have built the protoc artifacts for
every supported platform and put them under the target directory. Example
target directory layout:

    + pom.xml
    + target
      + linux
        + x86_64
          protoc.exe
        + x86_32
          protoc.exe
        + aarch_64
          protoc.exe
        + ppcle_64
          protoc.exe
        + s390_64
          protoc.exe
      + osx
        + x86_64
          protoc.exe
        + x86_32
          protoc.exe
      + windows
        + x86_64
          protoc.exe
        + x86_32
          protoc.exe

You will need to build the artifacts on multiple machines and gather them
together into one place.

Use the following command to deploy artifacts for the host platform to a
staging repository.

```
$ mvn deploy -P release
```

It creates a new staging repository. Go to
https://oss.sonatype.org/#stagingRepositories and find the repository, usually
in the name like ``comgoogle-123``. Verify that the staging repository has all
the binaries, close and release this repository.


## Tested build environments
We have successfully built artifacts on the following environments:
- Linux x86_32 and x86_64:
  - Centos 6.9 (within Docker 1.6.1)
  - Ubuntu 14.04.5 64-bit
- Linux aarch_64: Cross compiled with `g++-aarch64-linux-gnu` on Ubuntu 14.04.5 64-bit
- Mac OS X x86_32 and x86_64: Mac OS X 10.9.5
