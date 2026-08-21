# Protocol Buffers - Google's data interchange format

Copyright 2008 Google Inc.

https://developers.google.com/protocol-buffers/

## Use Java Protocol Buffers

To use protobuf in Java, first obtain the protocol compiler (a.k.a., protoc,
see instructions in the toplevel [README.md](../README.md)) and use it to
generate Java code for your .proto files:

    $ protoc --java_out=${OUTPUT_DIR} path/to/your/proto/file

Include the generated Java files in your project and add a dependency on the
protobuf Java runtime.

### Maven

If you are using Maven, use the following:

```xml
<dependency>
  <groupId>com.google.protobuf</groupId>
  <artifactId>protobuf-java</artifactId>
  <version><!--version--></version>
</dependency>
```

And **replace `<!--version-->` with a version from the
[Maven Protocol Buffers Repository](https://mvnrepository.com/artifact/com.google.protobuf/protobuf-java).**
For example, `4.28.2`.

Make sure the version number of the runtime matches (or is newer than) the
version number of the protoc.

If you want to use features like protobuf JsonFormat, add a dependency on the
protobuf-java-util package:

```xml
<dependency>
  <groupId>com.google.protobuf</groupId>
  <artifactId>protobuf-java-util</artifactId>
  <version><!--version--></version>
</dependency>
```

### Use Java Protocol Buffers on Android

For Android users, it's recommended to use protobuf Java Lite runtime because
of its smaller code size. Java Lite runtime also works better with Proguard
because it doesn't rely on Java reflection and is optimized to allow as much
code stripping as possible. You can following these [instructions to use Java
Lite runtime](lite.md).

### Use Java Protocol Buffers with Bazel

Bazel has native build rules to work with protobuf. For Java, you can use the
`java_proto_library` rule for server and the `java_lite_proto_library` rule for
Android. Check out [our build files examples](../examples/BUILD.bazel) to learn
how to use them.

## Build from Source

Most users should follow the instructions above to use protobuf Java runtime.
If you are contributing code to protobuf or want to use a protobuf version
that hasn't been officially released yet, you can follow the instructions
below to build protobuf from source code.

### Build from Source

You may follow these instructions to build from source. This does not require
Maven to be installed. Note that these instructions skip running unit tests and
only describes how to install the core protobuf library (without the util
package).

1) Build the C++ code, or obtain a binary distribution of protoc (see
   the toplevel [README.md](../README.md)). If you install a binary
   distribution, make sure that it is the same version as this package.
   If in doubt, run:

     $ protoc --version

   If you built the C++ code without installing, the compiler binary
   should be located in ../src.

2) Invoke protoc to build DescriptorProtos.java:

     $ protoc --java_out=core/src/main/java -I../src \
         ../src/google/protobuf/descriptor.proto

3) Compile the code in core/src/main/java using whatever means you prefer.

4) Install the classes wherever you prefer.

## Kotlin Protocol Buffers

This directory also provides support for Kotlin protocol buffers, which are
built on top of Java protocol buffers. Kotlin protocol buffers require a
dependency on Java protocol buffers, and both Java and Kotlin protocol buffer
code must be generated for every proto file.

The main goal of Kotlin protobuf is to provide idiomatic ways to build and read
protocol buffers in Kotlin. Learn more about Kotlin protobufs in our
[documentation](https://protobuf.dev/getting-started/kotlintutorial/).

### Use Kotlin Protocol Buffers

To use protobuf in Kotlin, first install the protocol compiler (protoc --
instructions for installing are in the top-level [README.md](../README.md)) and
use it to generate Java and Kotlin code for your .proto files:

```
$ protoc --java_out=${OUTPUT_DIR} --kotlin_out=${OUTPUT_DIR} path/to/your/proto/file
```

Include the generated Java and Kotlin files in your project and add a dependency on the
protobuf Java and Kotlin runtime.

### Maven

If you are using Maven, use the following:

```xml
<dependency>
  <groupId>com.google.protobuf</groupId>
  <artifactId>protobuf-java</artifactId>
  <version><!--version--></version>
</dependency>

<dependency>
  <groupId>com.google.protobuf</groupId>
  <artifactId>protobuf-kotlin</artifactId>
  <version><!--version--></version>
</dependency>
```

**Replace `<!--version-->` with a version from the
[Maven Protocol Buffers Repository](https://mvnrepository.com/artifact/com.google.protobuf/protobuf-kotlin),**
such as `4.28.2`.

Make sure the version number of the runtimes match each other and match (or are 
newer than) the version number of the protoc.

### Use Kotlin Protocol Buffers on Android

For Android users, it's recommended to use the Java Lite runtime for its smaller
code size. We provide a `protobuf-kotlin-lite` package in Maven and Bazel to
pair with the Java Lite runtime. Use these if you want to use Kotlin on
Android or in another context where you want to use Java Lite. Similar to the
full runtime, `protobuf-kotlin-lite` requires a dependency on
`protobuf-java-lite`.

## Compatibility Notice

* Protobuf minor version releases are backwards-compatible. If your code
  can build/run against the old version, it's expected to build/run against
  the new version as well. Both binary compatibility and source compatibility
  are guaranteed for minor version releases if the user follows the guideline
  described in this section.

* Protobuf major version releases may also be backwards-compatible with the
  last release of the previous major version. See the release notice for more
  details.

* APIs marked with the @ExperimentalApi annotation are subject to change. They
  can be modified in any way, or even removed, at any time. Don't use them if
  compatibility is needed. If your code is a library itself (i.e. it is used on
  the CLASSPATH of users outside your own control), you should not use
  experimental APIs, unless you repackage them (e.g. using ProGuard).

* Deprecated non-experimental APIs will be removed two years after the release
  in which they are first deprecated. You must fix your references before this
  time. If you don't, any manner of breakage could result (you are not
  guaranteed a compilation error).

* Protobuf message interfaces/classes are designed to be subclassed by protobuf
  generated code only. Do not subclass these message interfaces/classes
  yourself. We may add new methods to the message interfaces/classes which will
  break your own subclasses.

* Don't use any method/class that is marked as "used by generated code only".
  Such methods/classes are subject to change.

* Protobuf LITE runtime APIs are not stable yet. They are subject to change even
  in minor version releases.

## Documentation

The complete documentation for Protocol Buffers is available via the
web at:

  https://developers.google.com/protocol-buffers/
