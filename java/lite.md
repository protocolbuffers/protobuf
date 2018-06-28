# Protocol Buffers - Google's data interchange format

Copyright 2008 Google Inc.

https://developers.google.com/protocol-buffers/

## Use Protobuf Java Lite Runtime

Protobuf Java Lite runtime is separated from the main Java runtime because
it's designed/implemented with different constraints. In particular, Java
Lite runtime has a much smaller code size which makes it more suitable to
be used on Android.

To use Java Lite runtime, you need to install protoc and the protoc plugin for
Java Lite runtime. You can obtain protoc following the instructions in the
toplevel [README.md](../README.md) file. For the protoc plugin, you can
download it from maven:

    https://repo1.maven.org/maven2/com/google/protobuf/protoc-gen-javalite/

Choose the version that works on your platform (e.g., on windows you can
download `protoc-gen-javalite-3.0.0-windows-x86_32.exe`), rename it to
protoc-gen-javalite (or protoc-gen-javalite.exe on windows) and place it
in a directory where it can be find in PATH.

Once you have the protoc and protoc plugin, you can generate Java Lite code
for your .proto files:

    $ protoc --javalite_out=${OUTPUT_DIR} path/to/your/proto/file

Include the generated Java files in your project and add a dependency on the
protobuf Java runtime. If you are using Maven, use the following:

```xml
<dependency>
  <groupId>com.google.protobuf</groupId>
  <artifactId>protobuf-lite</artifactId>
  <version>3.0.1</version>
</dependency>
```

Make sure the version number of the runtime matches (or is newer than) the
version number of the protoc plugin. The version number of the protoc doesn't
matter and any version >= 3.0.0 should work.

### Use Protobuf Java Lite Runtime with Bazel

Bazel has native build rules to work with protobuf. For Java Lite runtime,
you can use the `java_lite_proto_library` rule. Check out [our build files
examples](../examples/BUILD) to learn how to use it.
