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

## Building from a freshly checked-out source

If you just checked out the Protobuf source from github, you need to
generate the configure script.

Under the protobuf project directory:

```
$ ./autogen.sh && ./configure && make
```

## To install artifacts locally
The following command will install the ``protoc`` artifact to your local Maven repository.
```
$ mvn install
```

## Cross-compilation
The Maven script will try to detect the OS and the architecture from Java
system properties. It's possible to build a protoc binary for an architecture
that is different from what Java has detected, as long as you have the proper
compilers installed.

You can override the Maven properties ``os.detected.name`` and
``os.detected.arch`` to force the script to generate binaries for a specific OS
and/or architecture. Valid values are defined as the return values of
``normalizeOs()`` and ``normalizeArch()`` of ``Detector`` from
[os-maven-plugin](https://github.com/trustin/os-maven-plugin/blob/master/src/main/java/kr/motd/maven/os/Detector.java).
Frequently used values are:
- ``os.detected.name``: ``linux``, ``osx``, ``windows``.
- ``os.detected.arch``: ``x86_32``, ``x86_64``

For example, MinGW32 only ships with 32-bit compilers, but you can still build
32-bit protoc under 64-bit Windows, with the following command:
```
$ mvn install -Dos.detected.arch=x86_32
```

## To push artifacts to Maven Central
Before you can upload artifacts to Maven Central repository, make sure you have
read [this page](http://central.sonatype.org/pages/apache-maven.html) on how to
configure GPG and Sonatype account.

You need to perform the deployment for every platform that you want to
support. DO NOT close the staging repository until you have done the
deployment for all platforms. Currently the following platforms are supported:
- Linux (x86_32 and x86_64)
- Windows (x86_32 and x86_64) with
 - Cygwin64 with MinGW compilers (x86_64)
 - MSYS with MinGW32 (x86_32)
- MacOSX (x86_32 and x86_64)

As for MSYS2/MinGW64 for Windows: protoc will build, but it insists on
adding a dependency of `libwinpthread-1.dll`, which isn't shipped with
Windows.

Use the following command to deploy artifacts for the host platform to a
staging repository.
```
$ mvn clean deploy -P release
```
It creates a new staging repository. Go to
https://oss.sonatype.org/#stagingRepositories and find the repository, usually
in the name like ``comgoogle-123``.

You will want to run this command on a different platform. Remember, in
subsequent deployments you will need to provide the repository name that you
have found in the first deployment so that all artifacts go to the same
repository:
```
$ mvn clean deploy -P release -Dstaging.repository=comgoogle-123
```

A 32-bit artifact can be deployed from a 64-bit host with
``-Dos.detected.arch=x86_32``

When you have done deployment for all platforms, go to
https://oss.sonatype.org/#stagingRepositories, verify that the staging
repository has all the binaries, close and release this repository.

### Tips for deploying on Linux
We build on Centos 6.6 to provide a good compatibility for not very new
systems. We have provided a ``Dockerfile`` under this directory to build the
environment. It has been tested with Docker 1.6.1.

To build a image:
```
$ docker build -t protoc-artifacts .
```

To run the image:
```
$ docker run -it --rm=true protoc-artifacts
```

The Protobuf repository has been cloned into ``/protobuf``.

### Tips for deploying on Windows
Under Windows the following error may occur: ``gpg: cannot open tty `no tty':
No such file or directory``. This can be fixed by configuring gpg through an
active profile in ``.m2\settings.xml`` where also the Sonatype password is
stored:
```xml
<settings>
  <servers>
    <server>
      <id>sonatype-nexus-staging</id>
      <username>[username]</username>
      <password>[password]</password>
    </server>
  </servers>
  <profiles>
    <profile>
      <id>gpg</id>
      <properties>
        <gpg.executable>gpg</gpg.executable>
        <gpg.passphrase>[password]</gpg.passphrase>
      </properties>
    </profile>
  </profiles>
  <activeProfiles>
    <activeProfile>gpg</activeProfile>
  </activeProfiles>
</settings>
```

### Tested build environments
We have successfully built artifacts on the following environments:
- Linux x86_32 and x86_64:
 - Centos 6.6 (within Docker 1.6.1)
 - Ubuntu 14.04.2 64-bit
- Windows x86_32: MSYS with ``mingw32-gcc-g++ 4.8.1-4`` on Windows 7 64-bit
- Windows x86_64: Cygwin64 with ``mingw64-x86_64-gcc-g++ 4.8.3-1`` on Windows 7 64-bit
- Mac OS X x86_32 and x86_64: Mac OS X 10.9.5
