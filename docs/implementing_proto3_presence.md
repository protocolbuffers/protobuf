# How To Implement Field Presence for Proto3

Protobuf release 3.12 adds experimental support for `optional` fields in
proto3. Proto3 optional fields track presence like in proto2. For background
information about what presence tracking means, please see
[docs/field_presence](field_presence.md).

## Document Summary

This document is targeted at developers who own or maintain protobuf code
generators. All code generators will need to be updated to support proto3
optional fields. First-party code generators developed by Google are being
updated already. However third-party code generators will need to be updated
independently by their authors. This includes:

- implementations of Protocol Buffers for other languages.
- alternate implementations of Protocol Buffers that target specialized use
  cases.
- RPC code generators that create generated APIs for service calls.
- code generators that implement some utility code on top of protobuf generated
  classes.

While this document speaks in terms of "code generators", these same principles
apply to implementations that dynamically generate a protocol buffer API "on the
fly", directly from a descriptor, in languages that support this kind of usage.

## Background

Presence tracking was added to proto3 in response to user feedback, both from
inside Google and [from open-source
users](https://github.com/protocolbuffers/protobuf/issues/1606). The [proto3
wrapper
types](https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/wrappers.proto)
were previously the only supported presence mechanism for proto3. Users have
pointed to both efficiency and usability issues with the wrapper types.

Presence in proto3 uses exactly the same syntax and semantics as in proto2.
Proto3 Fields marked `optional` will track presence like proto2, while fields
without any label (known as "singular fields"), will continue to omit presence
information.  The `optional` keyword was chosen to minimize differences with
proto2.

Unfortunately, for the current descriptor protos and `Descriptor` API (as of
3.11.4) it is not possible to use the same representation as proto2. Proto3
descriptors already use `LABEL_OPTIONAL` for proto3 singular fields, which do
not track presence. There is a lot of existing code that reflects over proto3
protos and assumes that `LABEL_OPTIONAL` in proto3 means "no presence." Changing
the semantics now would be risky, since old software would likely drop proto3
presence information, which would be a data loss bug.

To minimize this risk we chose a descriptor representation that is semantically
compatible with existing proto3 reflection. Every proto3 optional field is
placed into a one-field `oneof`. We call this a "synthetic" oneof, as it was not
present in the source `.proto` file.

Since oneof fields in proto3 already track presence, existing proto3
reflection-based algorithms should correctly preserve presence for proto3
optional fields with no code changes. For example, the JSON and TextFormat
parsers/serializers in C++ and Java did not require any changes to support
proto3 presence. This is the major benefit of synthetic oneofs.

This design does leave some cruft in descriptors. Synthetic oneofs are a
compatibility measure that we can hopefully clean up in the future. For now
though, it is important to preserve them across different descriptor formats and
APIs. It is never safe to drop synthetic oneofs from a proto schema. Code
generators can (and should) skip synthetic oneofs when generating a user-facing
API or user-facing documentation. But for any schema representation that is
consumed programmatically, it is important to keep the synthetic oneofs around.

In APIs it can be helpful to offer separate accessors that refer to "real"
oneofs (see [API Changes](#api-changes) below). This is a convenient way to omit
synthetic oneofs in code generators.

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

The experimental check will be removed  in a future release, once we are ready
to make this feature generally available. Ideally this will happen for the 3.13
release of protobuf, sometime in mid-2020, but there is not a specific date set
for this yet. Some of the timing will depend on feedback we get from the
community, so if you have questions or concerns please get in touch via a
GitHub issue.

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
bool FieldIsInOneof(const google::protobuf::FieldDescriptor* field) {
  return field->containing_oneof() != nullptr;
}
```

New:

```c++
bool FieldIsInOneof(const google::protobuf::FieldDescriptor* field) {
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

If your implementation offers reflection, there are a few other changes to make:

### API Changes

The API for reflecting over fields and oneofs should make the following changes.
These match the changes implemented in C++ reflection.

1. Add a `FieldDescriptor::has_presence()` method returning `bool`
   (adjusted to your language's naming convention).  This should return true
   for all fields that have explicit presence, as documented in
   [docs/field_presence](field_presence.md).  In particular, this includes
   fields in a oneof, proto2 scalar fields, and proto3 `optional` fields.
   This accessor will allow users to query what fields have presence without
   thinking about the difference between proto2 and proto3.
2. As a corollary of (1), please do *not* expose an accessor for the
   `FieldDescriptorProto.proto3_optional` field. We want to avoid having
   users implement any proto2/proto3-specific logic. Users should use the
   `has_presence()` function instead.
3. You may also wish to add a `FieldDescriptor::has_optional_keyword()` method
   returning `bool`, which indicates whether the `optional` keyword is present.
   Message fields will always return `true` for `has_presence()`, so this method
   can allow a user to know whether the user wrote `optional` or not. It can
   occasionally be useful to have this information, even though it does not
   change the presence semantics of the field.
4. If your reflection API may be used for a code generator, you may wish to
   implement methods to help users tell the difference between real and
   synthetic oneofs.  In particular:
   - `OneofDescriptor::is_synthetic()`: returns true if this is a synthetic
     oneof.
   - `FieldDescriptor::real_containing_oneof()`: like `containing_oneof()`,
     but returns `nullptr` if the oneof is synthetic.
   - `Descriptor::real_oneof_decl_count()`: like `oneof_decl_count()`, but
     returns the number of real oneofs only.

### Implementation Changes

Proto3 `optional` fields and synthetic oneofs must work correctly when
reflected on. Specifically:

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

### Validating Descriptors

If your reflection implementation supports loading descriptors at runtime,
you must verify that all synthetic oneofs are ordered after all "real" oneofs.

Here is the code that implements this validation step in C++, for inspiration:

```c++
  // Validation that runs for each message.
  // Synthetic oneofs must be last.
  int first_synthetic = -1;
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    if (oneof->is_synthetic()) {
      if (first_synthetic == -1) {
        first_synthetic = i;
      }
    } else {
      if (first_synthetic != -1) {
        AddError(message->full_name(), proto.oneof_decl(i),
                 DescriptorPool::ErrorCollector::OTHER,
                 "Synthetic oneofs must be after all other oneofs");
      }
    }
  }

  if (first_synthetic == -1) {
    message->real_oneof_decl_count_ = message->oneof_decl_count_;
  } else {
    message->real_oneof_decl_count_ = first_synthetic;
  }
```
