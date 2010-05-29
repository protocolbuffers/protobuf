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

Micro version
============================

The runtime and generated code for MICRO_RUNTIME is smaller
because it does not include support for the descriptor,
reflection or extensions. Also, not currently supported
are packed repeated elements nor testing of java_multiple_files.

To create a jar file for the runtime and run tests invoke
"mvn package -P micro" from the <protobuf-root>/java
directory. The generated jar file is
<protobuf-root>java/target/protobuf-java-2.2.0-micro.jar.

If you wish to compile the MICRO_RUTIME your self, place
the 7 files below, in <root>/com/google/protobuf and
create a jar file for use with your code and the generated
code:

ByteStringMicro.java
CodedInputStreamMicro.java
CodedOutputStreamMicro.java
InvalidProtocolBufferException.java
MessageMicro.java
StringUtf8Micro.java
WireFormatMicro.java

If you wish to change on the code generator it is located
in /src/google/protobuf/compiler/javamicro.

To generate code for the MICRO_RUNTIME invoke protoc with
--javamicro_out command line parameter. javamciro_out takes
a series of optional sub-parameters separated by comma's
and a final parameter, with a colon separator, which defines
the source directory. Sub-paraemeters begin with a name
followed by an equal and if that sub-parameter has multiple
parameters they are seperated by "|". The command line options
are:

opt                  -> speed or space
java_use_vector      -> true or false
java_package         -> <file-name>|<package-name>
java_outer_classname -> <file-name>|<package-name>

opt:
  This change the code generation to optimize for speed,
  opt=speed, or space, opt=space. When opt=speed this
  changes the code generation for strings to use
  StringUtf8Micro which eliminates multiple conversions
  of the string to utf8. The default value is opt=space.

java_use_vector:
  Is a boolean flag either java_use_vector=true or
  java_use_vector=false. When java_use_vector=true the
  code generated for repeated elements uses
  java.util.Vector and when java_use_vector=false the
  java.util.ArrayList<> is used. When java.util.Vector
  is used the code must be compiled with Java 1.3 and
  when ArrayList is used Java 1.5 or above must be used.
  The using javac the source parameter maybe used to
  control the version of the srouce: "javac -source 1.3".
  You can also change the <source> xml element for the
  maven-compiler-plugin. Below is for 1.5 sources:

      <plugin>
        <artifactId>maven-compiler-plugin</artifactId>
        <configuration>
          <source>1.5</source>
          <target>1.5</target>
        </configuration>
      </plugin>

  When compiling for 1.5 java_use_vector=false or not
  present where the default value is false.

  And below would be for 1.3 sources note when changing
  to 1.3 you must also set java_use_vector=true:

      <plugin>
        <artifactId>maven-compiler-plugin</artifactId>
        <configuration>
          <source>1.3</source>
          <target>1.5</target>
        </configuration>
      </plugin>

java_package:
  The allows setting/overriding the java_package option
  and associates allows a package name for a file to
  be specified on the command line. Overriding any
  "option java_package xxxx" in the file. The default
  if not present is to use the value from the package
  statment or "option java_package xxxx" in the file.

java_outer_classname:
  This allows the setting/overriding of the outer
  class name option and associates a class name
  to a file. An outer class name is required and
  must be specified if there are multiple messages
  in a single proto file either in the proto source
  file or on the command line. If not present the
  no outer class name will be used.

Below are a series of examples for clarification of the
various javamicro_out parameters using
src/test/proto/simple-data.proto:

package testprotobuf;

message SimpleData {
  optional fixed64 id = 1;
  optional string description = 2;
  optional bool ok = 3 [default = false];
};


Assuming you've only compiled and not installed protoc and
your current working directory java/, then a simple
command line to compile simple-data would be:

../src/protoc --javamicro_out=. src/test/proto/simple-data.proto

This will create testprotobuf/SimpleData.java

The directory testprotobuf is created because on line 1
of simple-data.proto is "package testprotobuf;". If you
wanted a different package name you could use the
java_package option command line sub-parameter:

../src/protoc '--javamicro_out=java_package=src/test/proto/simple-data.proto|my_package:.' src/test/proto/simple-data.proto

Here you see the new java_package sub-parameter which
itself needs two parameters the file name and the
package name, these are separated by "|". Now you'll
find my_package/SimpleData.java.

If you wanted to also change the optimization for
speed you'd add opt=speed with the comma seperator
as follows:

../src/protoc '--javamicro_out=opt=speed,java_package=src/test/proto/simple-data.proto|my_package:.' src/test/proto/simple-data.proto

Finally if you also wanted an outer class name you'd
do the following:

../src/protoc '--javamicro_out=opt=speed,java_package=src/test/proto/simple-data.proto|my_package,java_outer_classname=src/test/proto/simple-data.proto|OuterName:.' src/test/proto/simple-data.proto

Now you'll find my_packate/OuterName.java.

As mentioned java_package and java_outer_classname
may also be specified in the file. In the example
below we must define java_outer_classname because
there are multiple messages in
src/test/proto/two-messages.proto

package testmicroruntime;

option java_package = "com.example";
option java_outer_classname = "TestMessages";

message TestMessage1 {
  required int32 id = 1;
}

message TestMessage2 {
  required int32 id = 1;
}

This could be compiled using:

../src/protoc --javamicro_out=. src/test/proto/two-message.proto

With the result will be com/example/TestMessages.java


Usage
=====

The complete documentation for Protocol Buffers is available via the
web at:

  http://code.google.com/apis/protocolbuffers/
