# Ahead Of Time (AOT) compilation for the Java Virtual Machine (JVM)"

Ahead Of Time (AOT) compilation build tools such as those provided by [GraalVM's `native-image`](https://www.graalvm.org/reference-manual/native-image/) can require some configuration when using protobuf.
Protobuf for the JVM uses reflection and some of its target classes are not possible to determine in advance.
Historically, there were good reasons to use reflection based on APIs that were published effectively requiring them, and this situation is unlikely to change.

[The Lite version of protobuf for the JVM](https://github.com/protocolbuffers/protobuf/blob/master/java/lite.md)
avoids reflection and may be better suited for use with AOT compilation tooling. This Lite version was originally targeted for use on Android which has similar AOT compilation
goals as GraalVM's native-image tool.

## GraalVM native-image

This section addresses GraalVM's `native-image` configuration specifically as this AOT compilation tool due to its popularity. The `native-image` tool can be configured
with respect to: the [Java Native Interface](https://en.wikipedia.org/wiki/Java_Native_Interface) (JNI), http proxying, reflection, and other resources. While these
considerations can be manually declared as JSON files, we recommend that a JVM application is exercised along with 
[the assisted configuration agent](https://www.graalvm.org/reference-manual/native-image/BuildConfiguration/#assisted-configuration-of-native-image-builds). The agent
will generate files that you can then subsequently point at when invoking `native-image`. We recommend that the generated files are retained with a project's source
code.
