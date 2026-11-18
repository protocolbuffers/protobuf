# Java Lite For Editions

**Author:** [@zhangskz](https://github.com/zhangskz)

**Approved:** 2023-05-26

## Background

The "Lite" implementation for Java utilizes a custom format for embedding
descriptors motivated by critical code-size and performance requirements for
Android.

The code generator for Java Lite encodes an descriptor-like info string which is
stored into `RawMessageInfo`. This is decoded into `MessageSchema` which serves
as the descriptor-like schema for Java lite for parsing and serialization.

The current implementation makes significant use of an `is_proto3` bit in the
encoding, which is problematic for editions. Note that any parser changes to the
format would also need to maintain backwards compatibility, due to our
guarantees for parsers to remain backwards compatible within a major version.

## Overview

Fortunately, we already have corresponding bits for most
[Editions Zero Features](edition-zero-features.md) in the corresponding
`MessageInfo` field entry encoding.

We will move existing remaining syntax usages reading `is_proto3` to use these
bits. Several other syntax usages need to be made to be editions compatible by
merging implementations.

As new editions features are added that must be represented in `MessageInfo`, we
will eventually need to revamp `MessageInfo` encoding to support these changes.
However, this should be avoidable for Editions Zero.

## Recommendation

### Encoding: Add Is Edition Bit

`RawMessageInfo` should be augmented with an additional `is_edition` bit in
flags' unused bits.

\[0]: flags, flags & 0x1 = is proto2?, flags & 0x2 = is message?, flags &
**0x4 = is edition?**

The decoded `ProtoSyntax` should add a corresponding Editions option based on
this bit.

```
public enum ProtoSyntax
  PROTO2;
  PROTO3;
  EDITIONS;
```

For now, there is no need to explicitly encode the raw editions string or
feature options. These resolved features will be encoded directly in their
corresponding field entries.

### Encoding: Editions Zero Features

Field entries in `RawMessageInfo` already encode bits corresponding to most
***resolved*** Editions Zero features in `GetExperimentalJavaFieldType`. This is
decoded in `fieldTypeWithExtraBits` by reading the corresponding bits.

<table>
  <tr>
   <td><strong>Edition Zero Feature</strong>
   </td>
   <td><strong>Existing Encoding </strong>
   </td>
   <td><strong>Changes</strong>
   </td>
  </tr>
  <tr>
   <td>features.field_presence
   </td>
   <td> <code>kHasHasBit (0x1000)</code>
   </td>
   <td>Keep as-is.
   </td>
  </tr>
  <tr>
   <td>java.legacy_closed_enum
   </td>
   <td><code>kMapWithProto2EnumValue (0x800)</code>
   </td>
   <td>Replace with <code>kLegacyEnumIsClosedBit</code>
<p>
This will now be set for all enum fields, instead of just enum map values.
<p>
We will still need to check syntax in the interim in case of gencode.
   </td>
  </tr>
  <tr>
   <td><em>features.enum_type</em>
   </td>
   <td><em><code>EnumLiteGenerator</code> writes <code>UNRECOGNIZED(-1)</code> value for open enums in gencode.</em>
<p>
<em>This is not encoded in MessageInfo since this is an enum feature.</em>
   </td>
   <td><em>This is not needed in Editions Zero since enum closedness in Java Lite's runtime is dictated per-field by java.legacy_closed_enum. (<a href="edition-zero-feature-enum-field-closedness.md">Edition Zero Feature: Enum Field Closedness</a>), but should be used when Java non-conformance is fixed.</em>
<p>
<em>Note, this is implicitly encoded in kLegacyEnumIsClosedBit if java.legacy_closed_enum is unset since the corresponding FieldDescriptor helper should fall back on the EnumDescriptor.</em>
   </td>
  </tr>
  <tr>
   <td>features.repeated_field_encoding
   </td>
   <td><code>GetExperimentalJavaFieldTypeForPacked</code>
   </td>
   <td>Keep as-is.
   </td>
  </tr>
  <tr>
   <td>features.string_field_validation
   </td>
   <td><code>kUtf8CheckBit (0x200)</code>
   </td>
   <td>Keep as-is.
<p>
HINT does not apply to Java and will have the same behavior as MANDATORY or NONE
   </td>
  </tr>
  <tr>
   <td>features.message_encoding
   </td>
   <td>Not present.
   </td>
   <td>Encode as type group.
<p>
See below.
   </td>
  </tr>
</table>

Several places already use these bits properly, but there are a few syntax
usages in the decoding that should be replaced by checking the corresponding
feature bit.

There are several unused bits that we could use for future field-level features
before breaking the encoding format, but we should not need these for editions
zero.

The results of the `is_proto3` and feature bits only seem to be used within
protobuf, and don't seem to be publicly exposed.

#### features.message_encoding

In the compiler, message fields with `features.message_encoding = DELIMITED`
should be treated as a group *before* encoding message info.

This means that `GetExperimentalJavaFieldTypeForSingular`, should encode the
field's type `GROUP` (17), instead of its actual type `MESSAGE` (9), e.g.

```
int GetExperimentalJavaFieldTypeForSingular(const FieldDescriptor* field) {
  int result = field->type();
  if (result == FieldDescriptor::TYPE_MESSAGE) {
    if (field->isDelimited()) {
      return 17; // GROUP
    }
  }
}
```

`ImmutableMessageFieldLiteGenerator::GenerateFieldInfo` calls this when
generating the message field's field info.

The nested message's `MessageInfo` encoding does not need to be changed as this
is already identical for group and message.

Since each message field will be handled separately, this means that the
post-editions proto file below

```
// foo.proto
edition = "tbd"

message Foo {
  message Bar {
    int32 x = 1;
    repeated int32 y = 2;
  }
  Bar bar = 1 [features.message_encoding = DELIMITED];
  Bar baz = 2; // not DELIMITED

}
```

will be encoded and treated by `MessageSchema` like its pre-editions equivalent
below.

```
message Foo {
  group Bar = 1 {
    int32 x = 1;
    repeated int32 y = 2;
  }
  Bar baz = 2; // not DELIMITED
}
```

We recommended this alternative to minimize changes to the encoding and how
groups are treated.

In a future breaking change, we could consider renaming `FieldType.GROUP` to
`FieldType.MESSAGE_DELIMITED` while preserving the same number and encoding for
clarity. For now, we will leave the naming for this enum as-is.

##### Alternative: Add kIsMessageEncodingDelimitedBit

Alternatively, we could encode `features.message_encoding = DELIMITED` as-is as
type `MESSAGE`. The `MessageInfo` encoding would encode these as a normal
message field, using an unused (0x1100) bit as `kIsMessageEncodingDelimitedBit`.

This could be used to indicate that the message should be parsed/serialized from
the wire-format as if it were a group. This would need to be passed along to
`MessageSchema` which would then handle treating Messages with this bit set as
groups e.g. in `case Message`.

This is less ideal, since it would require handling this in multiple places.

### Unify non-feature syntax usages

There are several places that branch on syntax into separate proto2/proto3
codepaths. These generally duplicate a lot of code and should be unified into a
single syntax-agnostic code path branching on the relevant feature bits.

This code tends to be pretty opaque, so we should document this with comments or
add helpers (e.g. `isEnforceUtf8`) to indicate what feature bits are used as we
make changes here.

<table>
  <tr>
   <td><code>ManifestSchemaFactory.newSchema()</code>
   </td>
   <td>MessageInfo -> Schema
   </td>
   <td>Allow extensions for editions.
   </td>
  </tr>
  <tr>
   <td><code>MessageSchema.getSerializedSize()</code>
   </td>
   <td>Message -> Serialized Size
   </td>
   <td>Unify getSerializedSizeProto2/3
   </td>
  </tr>
  <tr>
   <td><code>MessageSchema.writeTo()</code>
   </td>
   <td>Serialize Message
   </td>
   <td>Unify writeFieldsInAscendingOrderProto2/3
   </td>
  </tr>
  <tr>
   <td><code>MessageSchema.mergeFrom()</code>
   </td>
   <td>Parse Message
   </td>
   <td>Unify parseProto2/3Message
   </td>
  </tr>
  <tr>
   <td><code>DescriptorMessageInfoFactory.convert()</code>
   </td>
   <td>Descriptor -> MessageInfo
   </td>
   <td>Unify convertProto2/3
   </td>
  </tr>
</table>

There is a lot of dead code in Java Lite so several syntax usages can also be
deleted or merged where possible.

## Alternatives

### Alternative 1: Introduce New Backwards-compatible MessageInfo Encoding

Add a new backwards-compatible `MessageInfo` encoding for editions.

The `is_edition` bit could toggle the encoding format being used, where
`is_edition == true` indicates the new encoding format but `is_edition == false`
indicates the old encoding.

This would allow us to encode additional information that the current encoding
format does not currently have available bits to support, such as the editions
string or additional features.

For example, the current encoding format only has a fixed number of available
field entry bits where we could encode new feature bits. We will need to
introduce a new encoding format once we exceed these, or if we want to encode
features at the message level.

In a future major version bump when support for proto2/3 is officially dropped,
we could drop support for the previous encoding format.

The recommendation is to revisit alternative 1 along with alternative 2
post-Editions zero as we need to support additional feature bits.

#### Pros

*   Future-proof for future editions and features

#### Cons

*   Blocks editions zero on more complex encoding changes that won't be used
    yet.
*   Requires more invasive updates to all MessageInfo decodings

### Alternative 2: Move to MiniDescriptor encoding

We could switch Java Lite to use the MiniDescriptor encoding specification.

Like Java Lite, this encoding seems to be optimized to be lightweight and with
minimal descriptor information.

MiniDescriptors do not encode proto2/proto3 syntax currently, which makes it
mostly editions-compatible. MiniDescriptors encode FieldModifier/MessageModifier
bits that correspond to some editions zero similarly to the Java Lite field
feature bits, and can be augmented to support additional features.

Supposedly, this encoding format *should* support an arbitrary number of
modifier bits, but this needs to be double-checked to verify there isn't a
similar hard limit to the number of features.

It is unclear whether this is sufficiently optimized for Android's needs and how
compatible this would be with Java Lite's Schemas.

The recommendation is to revisit alternative 2 along with alternative 1
post-Editions zero as we need to support additional feature bits.

#### Pros

*   Unify implementations for lower long-term maintenance cost

*   MiniDescriptor encoding will eventually need to be updated for editions
    anyways.

#### Cons

*   Blocks editions zero on more complex encoding changes that aren't necessary.

*   Requires even more invasive updates to all MessageInfo decodings

*   Probably requires major version bumps to break compatibility

*   Unknown code size /schema compatibility constraints that would need to be
    explored.

*   There are a few possible changes to MiniDescriptors on the table that we
    should wait to settle before bringing on additional implementations.

### Alternative 3: Do Nothing

Doing nothing is always an alternative. Describe the pros and cons of it.

#### Pros

*   No work

#### Cons

*   Editions is blocked since Java Lite protos are stuck in the past
