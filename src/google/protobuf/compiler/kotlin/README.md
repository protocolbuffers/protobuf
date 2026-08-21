
# Kotlin DSL Generator

This code generator implements the Kotlin DSL. The Kotlin DSL sits on top of
another proto implementation (written in Java or Kotlin) and adds convenient
support for building proto messages using DSL syntax, as documented in
[Kotlin Generated Code Guide](https://protobuf.dev/reference/kotlin/kotlin-generated/).

This code generator is invoked by passing `--kotlin_out` to `protoc`.

When using Kotlin on the JVM, you will also need to pass `--java_out` to
generate the Java code that implements the generated classes themselves.

When using Kotlin on other platforms (eg. Kotlin Native), there is currently no
support for generating message classes, so it is not possible to use the Kotlin
DSL at this time.
