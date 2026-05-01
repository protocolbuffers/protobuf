# Protocol Buffers - Google's data interchange format

Copyright 2008 Google Inc.

## The Protobuf Java Lite Runtime

The Protobuf Java Lite runtime is a separate runtime designed to be used on
mobile clients (especially on Android).

Lite runtime should not be used on server-side, as the tradeoffs it makes are
not suited for that environment. In server contexts, the Full runtime should
always be used instead.

The design goals differ from the Full runtime, focused on small binary size and
lower peak memory usage. To achieve this it takes a number of trade-offs,
including:

*   Offers a subset of features (including no reflection, ProtoJSON or TextProto
    support).

*   It may have slower runtime performance characteristics.

*   It may be less hardened against certain issues that could be a concern on
    server-side if they wouldn't be a concern in the context of mobile clients.

*   In order to achieve maximum performance and code size, we do NOT guarantee
    API/ABI stability for Java Lite.

If these trade-offs are not acceptable for your use-case, use the full Java
runtime instead.

You can generate Java Lite code for your .proto files:

```
$ protoc --java_out=lite:${OUTPUT_DIR} path/to/your/proto/file
```

Include the generated Java files in your project and add a dependency on the
protobuf Java Lite runtime. If you are using Maven, include the following:

```xml
<dependency>
  <groupId>com.google.protobuf</groupId>
  <artifactId>protobuf-javalite</artifactId>
  <version><!--version--></version>
</dependency>
```

And **replace `<!--version-->` with a version from the
[Maven Protocol Buffers \[Lite\] Repository](https://mvnrepository.com/artifact/com.google.protobuf/protobuf-javalite).**
For example, `3.25.3`.

## R8 rule to make production app builds work

The Lite runtime internally uses reflection to avoid generating
hashCode/equals/parse/serialize methods. R8 by default obfuscates the field
names, which makes the reflection fail causing exceptions of the form
`java.lang.RuntimeException: Field {NAME}_ for {CLASS} not found. Known fields
are [ {FIELDS} ]` in MessageSchema.java.

See previous discussion about this on the
[protobuf Github project](https://github.com/protocolbuffers/protobuf/issues/6463)
and [R8](https://issuetracker.google.com/issues/144631039).

In some cases, you may need to create a `proguard-rules.pro` file to mitigate
the obfuscation breaking the reflective codepaths, for example:

```
-keep class * extends com.google.protobuf.GeneratedMessageLite { *; }
```
