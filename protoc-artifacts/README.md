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

## To install artifacts locally
The following command will install the ``protoc`` artifact to your local Maven repository.
```
$ ./gradlew install
```

## To push artifacts to Maven Central
Before you can upload artifacts to Maven Central repository, you must have [set
up your account with OSSRH](http://central.sonatype.org/pages/ossrh-guide.html),
and have [generated a PGP key](http://gradle.org/docs/current/userguide/signing_plugin.html)
for siging.  You need to put your account information and PGP key information
in ``$HOME/.gradle/gradle.properties``, e.g.:
```
signing.keyId=24875D73
signing.password=secret
signing.secretKeyRingFile=/Users/me/.gnupg/secring.gpg

ossrhUsername=your-jira-id
ossrhPassword=your-jira-password
```

Use the following command to upload artifacts:
```
$ ./gradlew uploadArchives
```

