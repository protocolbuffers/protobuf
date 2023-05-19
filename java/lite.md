# Protocol Buffers - Google's data interchange format

Copyright 2008 Google Inc.

https://developers.google.com/protocol-buffers/

## Use the Protobuf Java Lite Runtime

The Protobuf Java Lite runtime is separated from the main Java runtime because
it's designed and implemented with different constraints. In particular, the Java
Lite runtime is much smaller which makes it more suitable to be used on Android.

In order to achieve maximum performance and code size, we do
NOT guarantee API/ABI stability for Java Lite. If this is not acceptable
for your use-case, use the full Java runtime instead. Note that
the latest version of Java Lite is not compatible with the 3.0.0 version.

You can generate Java Lite code for your .proto files:

    $ protoc --java_out=lite:${OUTPUT_DIR} path/to/your/proto/file

The "optimize_for = LITE_RUNTIME" option in the .proto file no longer has any
effect on Java code.

Include the generated Java files in your project and add a dependency on the
protobuf Java Lite runtime. If you are using Maven, include the following:

```xml
<dependency>
  <groupId>com.google.protobuf</groupId>
  <artifactId>protobuf-javalite</artifactId>
  <version>3.23.0</version>
</dependency>
```

## R8 rule to make production app builds work

The Lite runtime internally uses reflection to avoid generating hashCode/equals/(de)serialization methods. 
R8 by default obfuscates the field names, which makes the reflection fail causing exceptions of the form 
`java.lang.RuntimeException: Field {NAME}_ for {CLASS} not found. Known fields are [ {FIELDS} ]` in MessageSchema.java.

There are open issues for this on the [protobuf Github project](https://github.com/protocolbuffers/protobuf/issues/6463) and [R8](https://issuetracker.google.com/issues/144631039).

Until the issues is resolved you need to add the following line to your `proguard-rules.pro` file inside your project:

```
-keep class * extends com.google.protobuf.GeneratedMessageLite { *; }
```

## Older versions

For the older version of Java Lite (v3.0.0), please refer to:

    https://github.com/protocolbuffers/protobuf/blob/javalite/java/lite.md
