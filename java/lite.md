# Protocol Buffers - Google's data interchange format

Copyright 2008 Google Inc.

https://developers.google.com/protocol-buffers/

## Use Protobuf Java Lite Runtime

Protobuf Java Lite runtime is separated from the main Java runtime because
it's designed/implemented with different constraints. In particular, Java
Lite runtime has a much smaller code size which makes it more suitable to
be used on Android.

Note that in order to achieve maximum performance and code size, we will
NOT guarantee API/ABI stability for Java Lite. If this is not acceptable
for your use-case, please use the full Java runtime instead. Note that
the latest version of Java Lite is not compatible with the 3.0.0 version.

You can generate Java Lite code for your .proto files:

    $ protoc --java_out=lite:${OUTPUT_DIR} path/to/your/proto/file

Note that "optimize_for = LITE_RUNTIME" option in proto file is deprecated
and will not have any effect any more.

Include the generated Java files in your project and add a dependency on the
protobuf Java runtime. If you are using Maven, use the following:

```xml
<dependency>
  <groupId>com.google.protobuf</groupId>
  <artifactId>protobuf-javalite</artifactId>
  <version>3.8.0</version>
</dependency>
```

For the older version of Java Lite (v3.0.0), please refer to:

    https://github.com/protocolbuffers/protobuf/blob/javalite/java/lite.md
