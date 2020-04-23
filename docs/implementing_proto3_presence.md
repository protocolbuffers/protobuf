# How To Implement Field Presence for Proto3

Protobuf release 3.12 adds experimental support for `optional` fields in
proto3. Proto3 optional fields track presence like in proto2. For background
information about what presence tracking means, please see
[docs/field_presence](field_presence.md).

This document is targeted at developers who own or maintain protobuf code
generators. All code generators will need to be updated to support proto3
optional fields. First-party code generators developed by Google are being
updated already. However third-party code generators will need to be updated
independently by their authors. This includes:

- implementations of Protocol Buffers for other languges.
- alternate implementations of Protocol Buffers that target specialized use
  cases.
- code generators that implement some utility code on top of protobuf generated
  classes.

While this document speaks in terms of "code generators", these same principles
apply to implementations that dynamically generate a protocol buffer API "on the
fly", directly from a descriptor, in languages that support this kind of usage.

## Updating a Code Generator

When a user adds an `optional` field to proto3, this is internally rewritten as
a one-field oneof, for backward-compatibility with reflection-based algorithms:

```protobuf
syntax = "proto3";

message Foo {
  // Experimental feature, not generally supported yet!
  optional int32 foo = 1;

  // Internally rewritten to:
  // oneof _foo {
  //   int32 foo = 1 [proto3_optional=true];
  // }
  //
  // We call _foo a "synthetic" oneof, since it was not created by the user.
}
```

As a result, the main two goals when updating a code generator are:

1. Give `optional` fields like `foo` normal field presence, as described in
   [docs/field_presence](field_presence.md) If your implementation already
   supports proto2, a proto3 `optional` field should use exactly the same API
   and internal implementation as proto2 `optional`.
2. Avoid generating any oneof-based accessors for the synthetic oneof. Its only
   purpose is to make reflection-based algorithms work properly if they are
   not aware of proto3 presence. The synthetic oneof should not appear anywhere
   in the generated API.

### Satisfying the Experimental Check

If you try to run `protoc` on a file with proto3 `optional` fields, you will get
an error because the feature is still experimental:

```
$ cat test.proto
syntax = "proto3";

message Foo {
  // Experimental feature, not generally supported yet!
  optional int32 a = 1;
}
$ protoc --cpp_out=. test.proto
test.proto: This file contains proto3 optional fields, but --experimental_allow_proto3_optional was not set.
```

There are two options for getting around this error:

1. Pass `--experimental_allow_proto3_optional` to protoc.
2. Make your filename (or a directory name) contain the string
   `test_proto3_optional`. This indicates that the proto file is specifically
   for testing proto3 optional support, so the check is suppressed.

These options are demonstrated below:

```
# One option:
$ ./src/protoc test.proto --cpp_out=. --experimental_allow_proto3_optional

# Another option:
$ cp test.proto test_proto3_optional.proto
$ ./src/protoc test_proto3_optional.proto --cpp_out=.
$
```

### Signaling That Your Code Generator Supports Proto3 Optional

If you now try to invoke your own code generator with the test proto, you will
run into a different error:

```
$ ./src/protoc test_proto3_optional.proto --my_codegen_out=.
test_proto3_optional.proto: is a proto3 file that contains optional fields, but
code generator --my_codegen_out hasn't been updated to support optional fields in
proto3. Please ask the owner of this code generator to support proto3 optional.
```

This check exists to make sure that code generators get a chance to update
before they are used with proto3 `optional` fields. Without this check an old
code generator might emit obsolete generated APIs (like accessors for a
synthetic oneof) and users could start depending on these. That would create
a legacy migration burden once a code generator actually implements the feature.

To signal that your code generator supports `optional` fields in proto3, you
need to tell `protoc` what features you support. The method for doing this
depends on whether you are using the C++
`google::protobuf::compiler::CodeGenerator`
framework or not.

If you are using the CodeGenerator framework:

```c++
class MyCodeGenerator : public google::protobuf::compiler::CodeGenerator {
  // Add this method.
  uint64_t GetSupportedFeatures() const override {
    // Indicate that this code generator supports proto3 optional fields.
    // (Note: don't release your code generator with this flag set until you
    // have actually added and tested your proto3 support!)
    return FEATURE_PROTO3_OPTIONAL;
  }
}
```

If you are generating code using raw `CodeGeneratorRequest` and
`CodeGeneratorResponse` messages from `plugin.proto`, the change will be very
similar:

```c++
void GenerateResponse() {
  CodeGeneratorResponse response;
  response.set_supported_features(CodeGeneratorResponse::FEATURE_PROTO3_OPTIONAL);

  // Generate code...
}
```

Once you have added this, you should now be able to successfully use your code
generator to generate a file containing proto3 optional fields:

```
$ ./src/protoc test_proto3_optional.proto --my_codegen_out=.
```

### Updating Your Code Generator

Now to actually add support for proto3 optional to your code generator. The goal
is to recognize proto3 optional fields as optional, and suppress any output from
synthetic oneofs.

If your code generator does not currently support proto2, you will need to
design an API and implementation for supporting presence in scalar fields.
Generally this means:

- allocating a bit inside the generated class to represent whether a given field
  is present or not.
- exposing a `has_foo()` method for each field to return the value of this bit.
- make the parser set this bit when a value is parsed from the wire.
- make the serializer test this bit to decide whether to serialize.

If your code generator already supports proto2, then most of your work is
already done. All you need to do is make sure that proto3 optional fields have
exactly the same API and behave in exactly the same way as proto2 optional
fields.

From experience updating several of Google's code generators, most of the
updates that are required fall into one of several patterns. Here we will show
the patterns in terms of the C++ CodeGenerator framework. If you are using
`CodeGeneratorRequest` and `CodeGeneratorReply` directly, you can translate the
C++ examples to your own language, referencing the C++ implementation of these
methods where required.

#### To test whether a field should have presence

Old:

```c++
bool MessageHasPresence(const google::protobuf::Descriptor* message) {
  return message->file()->syntax() ==
         google::protobuf::FileDescriptor::SYNTAX_PROTO2;
}
```

New:

```c++
// Presence is no longer a property of a message, it's a property of individual
// fields.
bool FieldHasPresence(const google::protobuf::FieldDescriptor* field) {
  return field->has_presence();
  // Note, the above will return true for fields in a oneof.
  // If you want to filter out oneof fields, write this instead:
  //   return field->has_presence && !field->real_containing_oneof()
}
```

#### To test whether a field is a member of a oneof

Old:

```c++
bool FieldIsInOneof(const google::protobuf::FielDescriptor* field) {
  return field->containing_oneof() != nullptr;
}
```

New:

```c++
bool FieldIsInOneof(const google::protobuf::FielDescriptor* field) {
  // real_containing_oneof() returns nullptr for synthetic oneofs.
  return field->real_containing_oneof() != nullptr;
}
```

#### To iterate over all oneofs

Old:

```c++
bool IterateOverOneofs(const google::protobuf::Descriptor* message) {
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const google::protobuf::OneofDescriptor* oneof = message->oneof(i);
    // ...
  }
}
```

New:

```c++
bool IterateOverOneofs(const google::protobuf::Descriptor* message) {
  // Real oneofs are always first, and real_oneof_decl_count() will return the
  // total number of oneofs, excluding synthetic oneofs.
  for (int i = 0; i < message->real_oneof_decl_count(); i++) {
    const google::protobuf::OneofDescriptor* oneof = message->oneof(i);
    // ...
  }
}
```

## Updating Reflection

If your implementation supports protobuf reflection, there are a few changes
that you need to make:

1. Reflection for synthetic oneofs should work properly. Even though synthetic
   oneofs do not really exist in the message, you can still make reflection work
   as if they did. In particular, you can make a method like
   `Reflection::HasOneof()` or `Reflection::GetOneofFieldDescriptor()` look at
   the hasbit to determine if the oneof is present or not.
2. Reflection for proto3 optional fields should work properly. For example, a
   method like `Reflection::HasField()` should know to look for the hasbit for a
   proto3 `optional` field. It should not be fooled by the synthetic oneof into
   thinking that there is a `case` member for the oneof.

Once you have updated reflection to work properly with proto3 `optional` and
synthetic oneofs, any code that *uses* your reflection interface should work
properly with no changes. This is the benefit of using synthetic oneofs.

In particular, if you have a reflection-based implementation of protobuf text
format or JSON, it should properly support proto3 optional fields without any
changes to the code. The fields will look like they all belong to a one-field
oneof, and existing proto3 reflection code should know how to test presence for
fields in a oneof.

So the best way to test your reflection changes is to try round-tripping a
message through text format, JSON, or some other reflection-based parser and
serializer, if you have one.
