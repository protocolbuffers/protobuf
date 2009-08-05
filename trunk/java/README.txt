Protocol Buffers - Google's data interchange format
Copyright 2008 Google Inc.

This directory contains the Java Protocol Buffers runtime library.

Installation - With Maven
=========================

The Protocol Buffers build is managed using Maven.  If you would
rather build without Maven, see below.

1) Install Apache Maven if you don't have it:

     http://maven.apache.org/

2) Build the C++ code, or obtain a binary distribution of protoc.  If
   you install a binary distribution, make sure that it is the same
   version as this package.  If in doubt, run:

     $ protoc --version

   You will need to place the protoc executable in ../src.  (If you
   built it yourself, it should already be there.)

3) Run the tests:

     $ mvn test

   If some tests fail, this library may not work correctly on your
   system.  Continue at your own risk.

4) Install the library into your Maven repository:

     $ mvn install

5) If you do not use Maven to manage your own build, you can build a
   .jar file to use:

     $ mvn package

   The .jar will be placed in the "target" directory.

Installation - 'Lite' Version - With Maven
==========================================

Building the 'lite' version of the Java Protocol Buffers library is
the same as building the full version, except that all commands are
run using the 'lite' profile.  (see
http://maven.apache.org/guides/introduction/introduction-to-profiles.html)

E.g. to install the lite version of the jar, you would run:

  $ mvn install -P lite

The resulting artifact has the 'lite' classifier.  To reference it
for dependency resolution, you would specify it as:

  <dependency>
    <groupId>com.google.protobuf</groupId>
    <artifactId>protobuf-java</artifactId>
    <version>${version}</version>
    <classifier>lite</classifier>
  </dependency>

Installation - Without Maven
============================

If you would rather not install Maven to build the library, you may
follow these instructions instead.  Note that these instructions skip
running unit tests.

1) Build the C++ code, or obtain a binary distribution of protoc.  If
   you install a binary distribution, make sure that it is the same
   version as this package.  If in doubt, run:

     $ protoc --version

   If you built the C++ code without installing, the compiler binary
   should be located in ../src.

2) Invoke protoc to build DescriptorProtos.java:

     $ protoc --java_out=src/main/java -I../src \
         ../src/google/protobuf/descriptor.proto

3) Compile the code in src/main/java using whatever means you prefer.

4) Install the classes wherever you prefer.

Usage
=====

The complete documentation for Protocol Buffers is available via the
web at:

  http://code.google.com/apis/protocolbuffers/
