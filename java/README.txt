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
because it does not include support for the descriptor and
reflection, and enums are generated as integer constants in
the parent message or the file's outer class, with no
protection against invalid values set to enum fields. Also,
not currently supported are packed repeated elements or
extensions.

To create a jar file for the runtime and run tests invoke
"mvn package -P micro" from the <protobuf-root>/java
directory. The generated jar file is
<protobuf-root>java/target/protobuf-java-2.2.0-micro.jar.

If you wish to compile the MICRO_RUNTIME your self, place
the 7 files below, in <root>/com/google/protobuf and
create a jar file for use with your code and the generated
code:

ByteStringMicro.java
CodedInputStreamMicro.java
CodedOutputStreamMicro.java
InvalidProtocolBufferException.java
MessageMicro.java
WireFormatMicro.java

If you wish to change on the code generator it is located
in /src/google/protobuf/compiler/javamicro.

To generate code for the MICRO_RUNTIME invoke protoc with
--javamicro_out command line parameter. javamicro_out takes
a series of optional sub-parameters separated by commas
and a final parameter, with a colon separator, which defines
the source directory. Sub-parameters begin with a name
followed by an equal and if that sub-parameter has multiple
parameters they are seperated by "|". The command line options
are:

opt                  -> speed or space
java_use_vector      -> true or false
java_package         -> <file-name>|<package-name>
java_outer_classname -> <file-name>|<package-name>
java_multiple_files  -> true or false

opt={speed,space} (default: space)
  This changes the code generation to optimize for speed or
  space. When opt=speed this changes the code generation
  for strings so that multiple conversions to Utf8 are
  eliminated.

java_use_vector={true,false} (default: false)
  This specifies the collection class for repeated elements.
  If false, repeated elements use java.util.ArrayList<> and
  the code must be compiled with Java 1.5 or above. If true,
  repeated elements use java.util.Vector and the code can
  be compiled with Java 1.3 or above. The 'source'
  parameter of 'javac' may be used to control the version
  of the source: "javac -source 1.3". You can also change
  the <source> xml element for the maven-compiler-plugin.
  Below is for 1.5 sources:

      <plugin>
        <artifactId>maven-compiler-plugin</artifactId>
        <configuration>
          <source>1.5</source>
          <target>1.5</target>
        </configuration>
      </plugin>

  And below would be for 1.3 sources (note when changing
  to 1.3 you must also set java_use_vector=true):

      <plugin>
        <artifactId>maven-compiler-plugin</artifactId>
        <configuration>
          <source>1.3</source>
          <target>1.5</target>
        </configuration>
      </plugin>

java_package=<file-name>|<package-name> (no default)
  This allows overriding the 'java_package' option value
  for the given file from the command line. Use multiple
  java_package options to override the option for multiple
  files. The final Java package for each file is the value
  of this command line option if present, or the value of
  the same option defined in the file if present, or the
  proto package if present, or the default Java package.

java_outer_classname=<file-name>|<outer-classname> (no default)
  This allows overriding the 'java_outer_classname' option
  for the given file from the command line. Use multiple
  java_outer_classname options to override the option for
  multiple files. The final Java outer class name for each
  file is the value of this command line option if present,
  or the value of the same option defined in the file if
  present, or the file name converted to CamelCase. This
  outer class will nest all classes and integer constants
  generated from file-scope messages and enums.

java_multiple_files={true,false} (no default)
  This allows overriding the 'java_multiple_files' option
  in all source files and their imported files from the
  command line. The final value of this option for each
  file is the value defined in this command line option, or
  the value of the same option defined in the file if
  present, or false. This specifies whether to generate
  package-level classes for the file-scope messages in the
  same Java package as the outer class (instead of nested
  classes in the outer class). File-scope enum constants
  are still generated as integer constants in the outer
  class. This affects the fully qualified references in the
  Java code. NOTE: because the command line option
  overrides the value for all files and their imported
  files, using this option inconsistently may result in
  incorrect references to the imported messages and enum
  constants.


IMPORTANT: change of javamicro_out behavior:

In previous versions, if the outer class name has not been
given explicitly, javamicro_out would not infer the outer
class name from the file name, and would skip the outer
class generation. This makes the compilation succeed only
if the source file contains a single message and no enums,
and the generated class for that message is placed at the
package level. To re-align with java_out, javamicro_out
will now always generate the outer class, inferring its
name from the file name if not given, as a container of the
message classes and enum constants. To keep any existing
single-message source file from causing the generation of
an unwanted outer class, you can set the option
java_multiple_files to true, either in the file or as a
command line option.


Below are a series of examples for clarification of the
various parameters and options. Assuming this file:

src/proto/simple-data-protos.proto:

    package testprotobuf;

    message SimpleData {
      optional fixed64 id = 1;
      optional string description = 2;
      optional bool ok = 3 [default = false];
    };

and the compiled protoc in the current working directory,
then a simple command line to compile this file would be:

./protoc --javamicro_out=. src/proto/simple-data-protos.proto

This will create testprotobuf/SimpleDataProtos.java, which
has the following content (extremely simplified):

    package testprotobuf;

    public final class SimpleDataProtos {
      public static final class SimpleData
          extends MessageMicro {
        ...
      }
    }

The message SimpleData is compiled into the SimpleData
class, nested in the file's outer class SimpleDataProtos,
whose name is implicitly defined by the proto file name
"simple-data-protos".

The directory, aka Java package, testprotobuf is created
because on line 1 of simple-data-protos.proto is
"package testprotobuf;". If you wanted a different
package name you could use the java_package option in the
file:

    option java_package = "my_package";

or in command line sub-parameter:

./protoc '--javamicro_out=\
java_package=src/proto/simple-data-protos.proto|my_package:\
.' src/proto/simple-data-protos.proto

Here you see the new java_package sub-parameter which
itself needs two parameters the file name and the
package name, these are separated by "|". The value set
in the command line overrides the value set in the file.
Now you'll find SimpleDataProtos.java in the my_package/
directory.

If you wanted to also change the optimization for
speed you'd add opt=speed with the comma seperator
as follows:

./protoc '--javamicro_out=\
opt=speed,\
java_package=src/proto/simple-data-protos.proto|my_package:
.' src/proto/simple-data-protos.proto

If you also wanted a different outer class name you'd
do the following:

./protoc '--javamicro_out=\
opt=speed,\
java_package=src/proto/simple-data-protos.proto|my_package,\
java_outer_classname=src/proto/simple-data-protos.proto|OuterName:\
.' src/proto/simple-data-protos.proto

Now you'll find my_package/OuterName.java and the
message class SimpleData nested in it.

As mentioned java_package, java_outer_classname and
java_multiple_files may also be specified in the file.
In the example below we must define
java_outer_classname because otherwise the outer class
and one of the message classes will have the same name,
which is forbidden to prevent name ambiguity:

src/proto/sample-message.proto:

    package testmicroruntime;

    option java_package = "com.example";
    option java_outer_classname = "SampleMessageProtos";

    enum MessageType {
      SAMPLE = 1;
      EXAMPLE = 2;
    }

    message SampleMessage {
      required int32 id = 1;
      required MessageType type = 2;
    }

    message SampleMessageContainer {
      required SampleMessage message = 1;
    }

This could be compiled using:

./protoc --javamicro_out=. src/proto/sample-message.proto

and the output will be:

com/example/SampleMessageProtos.java:

    package com.example;

    public final class SampleMessageProtos {
      public static final int SAMPLE = 1;
      public static final int EXAMPLE = 2;
      public static final class SampleMessage
          extends MessageMicro {
        ...
      }
      public static final class SampleMessageContainer
          extends MessageMicro {
        ...
      }
    }

As you can see the file-scope enum MessageType is
disassembled into two integer constants in the outer class.
In javamicro_out, all enums are disassembled and compiled
into integer constants in the parent scope (the containing
message's class or the file's (i.e. outer) class).

You may prefer the file-scope messages to be saved in
separate files. You can do this by setting the option
java_multiple_files to true, in either the file like this:

    option java_multiple_files = true;

or the command line like this:

./protoc --javamicro_out=\
java_multiple_files=true:\
. src/proto/sample-message.proto

The java_multiple_files option causes javamicro to use a
separate file for each file-scope message, which resides
directly in the Java package alongside the outer class:

com/example/SampleMessageProtos.java:

    package com.example;
    public final class SampleMessageProtos {
      public static final int SAMPLE = 1;
      public static final int EXAMPLE = 2;
    }

com/example/SampleMessage.java:

    package com.example;
    public final class SampleMessage
        extends MessageMicro {
      ...
    }

com/example/SampleMessageContainer.java:

    package com.example;
    public final class SampleMessageContainer
        extends MessageMicro {
      ...
    }

As you can see, the outer class now contains only the
integer constants, generated from the file-scope enum
"MessageType". Please note that message-scope enums are
still generated as integer constants in the message class.


Nano version
============================

Nano is a special code generator and runtime library designed specially
for Android, and is very resource-friendly in both the amount of code
and the runtime overhead. An overview of Nano features:

- No descriptors or message builders.
- All messages are mutable; fields are public Java fields.
- For optional fields only, encapsulation behind setter/getter/hazzer/
  clearer functions is opt-in, which provide proper 'has' state support.
- If not opted in, has state is not available. Serialization outputs
  all fields not equal to their defaults (see important implications
  below).
- Required fields are always serialized.
- Enum constants are integers; protection against invalid values only
  when parsing from the wire.
- Enum constants can be generated into container interfaces bearing
  the enum's name (so the referencing code is in Java style).
- CodedInputByteBufferNano can only take byte[] (not InputStream).
- Similarly CodedOutputByteBufferNano can only write to byte[].
- Repeated fields are in arrays, not ArrayList or Vector. Null array
  elements are allowed and silently ignored.
- Full support of serializing/deserializing repeated packed fields.
- Support of extensions.
- Unset messages/groups are null, not an immutable empty default
  instance.
- toByteArray(...) and mergeFrom(...) are now static functions of
  MessageNano.
- The 'bytes' type translates to the Java type byte[].

The generated messages are not thread-safe for writes, but may be
used simultaneously from multiple threads in a read-only manner.
In other words, an appropriate synchronization mechanism (such as
a ReadWriteLock) must be used to ensure that a message, its
ancestors, and descendants are not accessed by any other threads
while the message is being modified. Field reads, getter methods
(but not getExtension(...)), toByteArray(...), writeTo(...),
getCachedSize(), and getSerializedSize() are all considered read-only
operations.

IMPORTANT: If you have fields with defaults and opt out of accessors

How fields with defaults are serialized has changed. Because we don't
keep "has" state, any field equal to its default is assumed to be not
set and therefore is not serialized. Consider the situation where we
change the default value of a field. Senders compiled against an older
version of the proto continue to match against the old default, and
don't send values to the receiver even though the receiver assumes the
new default value. Therefore, think carefully about the implications
of changing the default value. Alternatively, turn on accessors and
enjoy the benefit of the explicit has() checks.

IMPORTANT: If you have "bytes" fields with non-empty defaults

Because the byte buffer is now of mutable type byte[], the default
static final cannot be exposed through a public field. Each time a
message's constructor or clear() function is called, the default value
(kept in a private byte[]) is cloned. This causes a small memory
penalty. This is not a problem if the field has no default or is an
empty default.

Nano Generator options

java_package           -> <file-name>|<package-name>
java_outer_classname   -> <file-name>|<package-name>
java_multiple_files    -> true or false
java_nano_generate_has -> true or false [DEPRECATED]
optional_field_style   -> default or accessors
enum_style             -> c or java
ignore_services        -> true or false
parcelable_messages    -> true or false

java_package:
java_outer_classname:
java_multiple_files:
  Same as Micro version.

java_nano_generate_has={true,false} (default: false)
  DEPRECATED. Use optional_field_style=accessors.

  If true, generates a public boolean variable has<fieldname>
  accompanying each optional or required field (not present for
  repeated fields, groups or messages). It is set to false initially
  and upon clear(). If parseFrom(...) reads the field from the wire,
  it is set to true. This is a way for clients to inspect the "has"
  value upon parse. If it is set to true, writeTo(...) will ALWAYS
  output that field (even if field value is equal to its
  default).

  IMPORTANT: This option costs an extra 4 bytes per primitive field in
  the message. Think carefully about whether you really need this. In
  many cases reading the default works and determining whether the
  field was received over the wire is irrelevant.

optional_field_style={default,accessors,reftypes} (default: default)
  Defines the style of the generated code for fields.

  * default *

  In the default style, optional fields translate into public mutable
  Java fields, and the serialization process is as discussed in the
  "IMPORTANT" section above. 

  * accessors *

  When set to 'accessors', each optional field is encapsulated behind
  4 accessors, namely get<fieldname>(), set<fieldname>(), has<fieldname>()
  and clear<fieldname>() methods, with the standard semantics. The hazzer's
  return value determines whether a field is serialized, so this style is
  useful when you need to serialize a field with the default value, or check
  if a field has been explicitly set to its default value from the wire.

  In the 'accessors' style, required and nested message fields are still
  translated to one public mutable Java field each, repeated fields are still
  translated to arrays. No accessors are generated for them.

  IMPORTANT: When using the 'accessors' style, ProGuard should always
  be enabled with optimization (don't use -dontoptimize) and allowing
  access modification (use -allowaccessmodification). This removes the
  unused accessors and maybe inline the rest at the call sites,
  reducing the final code size.
  TODO(maxtroy): find ProGuard config that would work the best.

  * reftypes *

  When set to 'reftypes', each proto field is generated as a public Java
  field. For primitive types, these fields use the Java reference types
  such as java.lang.Integer instead of primitive types such as int.

  In the 'reftypes' style, fields are initialized to null (or empty
  arrays for repeated fields), and their default values are not available.
  They are serialized over the wire based on equality to null.

  The 'reftypes' mode has some additional cost due to autoboxing and usage
  of reference types. In practice, many boxed types are cached, and so don't
  result in object creation. However, references do take slightly more memory
  than primitives.

  The 'reftypes' mode is useful when you want to be able to serialize fields
  with default values, or check if a field has been explicitly set to the
  default over the wire without paying the extra method cost of the
  'accessors' mode.

  Note that if you attempt to write null to a required field in the reftypes
  mode, serialization of the proto will cause a NullPointerException. This is
  an intentional indicator that you must set required fields.

  NOTE
  optional_field_style=accessors or reftypes cannot be used together with
  java_nano_generate_has=true. If you need the 'has' flag for any
  required field (you have no reason to), you can only use
  java_nano_generate_has=true.

enum_style={c,java} (default: c)
  Defines where to put the int constants generated from enum members.

  * c *

  Use C-style, so the enum constants are available at the scope where
  the enum is defined. A file-scope enum's members are referenced like
  'FileOuterClass.ENUM_VALUE'; a message-scope enum's members are
  referenced as 'Message.ENUM_VALUE'. The enum name is unavailable.
  This complies with the Micro code generator's behavior.

  * java *

  Use Java-style, so the enum constants are available under the enum
  name and referenced like 'EnumName.ENUM_VALUE' (they are still int
  constants). The enum name becomes the name of a public interface, at
  the scope where the enum is defined. If the enum is file-scope and
  the java_multiple_files option is on, the interface will be defined
  in its own file. To reduce code size, this interface should not be
  implemented and ProGuard shrinking should be used, so after the Java
  compiler inlines all referenced enum constants into the call sites,
  the interface remains unused and can be removed by ProGuard.

ignore_services={true,false} (default: false)
  Skips services definitions.

  Nano doesn't support services. By default, if a service is defined
  it will generate a compilation error. If this flag is set to true,
  services will be silently ignored, instead.

parcelable_messages={true,false} (default: false)
  Android-specific option to generate Parcelable messages.


To use nano protobufs within the Android repo:

- Set 'LOCAL_PROTOC_OPTIMIZE_TYPE := nano' in your local .mk file.
  When building a Java library or an app (package) target, the build
  system will add the Java nano runtime library to the
  LOCAL_STATIC_JAVA_LIBRARIES variable, so you don't need to.
- Set 'LOCAL_PROTO_JAVA_OUTPUT_PARAMS := ...' in your local .mk file
  for any command-line options you need. Use commas to join multiple
  options. In the nano flavor only, whitespace surrounding the option
  names and values are ignored, so you can use backslash-newline or
  '+=' to structure your make files nicely.
- The options will be applied to *all* proto files in LOCAL_SRC_FILES
  when you build a Java library or package. In case different options
  are needed for different proto files, build separate Java libraries
  and reference them in your main target. Note: you should make sure
  that, for each separate target, all proto files imported from any
  proto file in LOCAL_SRC_FILES are included in LOCAL_SRC_FILES. This
  is because the generator has to assume that the imported files are
  built using the same options, and will generate code that reference
  the fields and enums from the imported files using the same code
  style.
- Hint: 'include $(CLEAR_VARS)' resets all LOCAL_ variables, including
  the two above.

To use nano protobufs outside of Android repo:

- Link with the generated jar file
  <protobuf-root>java/target/protobuf-java-2.3.0-nano.jar.
- Invoke with --javanano_out, e.g.:

./protoc '--javanano_out=\
    java_package=src/proto/simple-data.proto|my_package,\
    java_outer_classname=src/proto/simple-data.proto|OuterName\
  :.' src/proto/simple-data.proto

Contributing to nano:

Please add/edit tests in NanoTest.java.

Please run the following steps to test:

- cd external/protobuf
- ./configure
- Run "make -j12 check" and verify all tests pass.
- cd java
- Run "mvn test" and verify all tests pass.
- cd ../../..
- . build/envsetup.sh
- lunch 1
- "make -j12 aprotoc libprotobuf-java-2.3.0-nano aprotoc-test-nano-params NanoAndroidTest" and
  check for build errors.
- Plug in an Android device or start an emulator.
- adb install -r out/target/product/generic/data/app/NanoAndroidTest.apk
- Run:
  "adb shell am instrument -w com.google.protobuf.nano.test/android.test.InstrumentationTestRunner"
  and verify all tests pass.
- repo sync -c -j256
- "make -j12" and check for build errors


Usage
=====

The complete documentation for Protocol Buffers is available via the
web at:

  http://code.google.com/apis/protocolbuffers/
