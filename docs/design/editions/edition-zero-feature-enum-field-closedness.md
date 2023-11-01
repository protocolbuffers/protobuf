# Edition Zero Feature: Enum Field Closedness

**Author:** [@mcy](https://github.com/mcy)

**Approved:** 2023-02-13

## Background

On 2023-02-10, a CL [@mcy](https://github.com/mcy) submitted to delete
`google::protobuf::Reflection::SupportsUnknownEnumValue()`. Oddly, this function used the
containing message's `syntax`, rather than the enum field's, to determine
whether the enum was open.

It turns out we misunderstood a critical corner-case of proto3 enums. Consider
the following proto files:

```
// enum.proto
syntax = "proto3";
package oh.no;

enum Enum {
  A = 0;
  B = 1;
}

// message.proto
syntax = "proto2";
package oh.no;
import "enum.proto";

message Msg {
  optional Enum enum = 1;
}
```

If we parse the [Protoscope](https://github.com/protocolbuffers/protoscope)
value `1: 2` as an `oh.no.Msg`, and look at the value of `oh.no.Msg.enum`, we
will find that it is not present, and that there is a VARINT of value 2 in the
`UnknownFieldSet`.

This is because Protobuf sometimes implements the openness of an enum by its
usage, *not* its definition.

This case is actually quite difficult to observe, because the converse doesn't
work: the proto compiler rejects proto2-enum-valued fields in proto3 messages,
because such enums can have nonzero defaults, which proto3 does not support due
to implicit presence.

### Languages

<table>
  <tr>
   <td style="background-color: #cccccc">Language
   </td>
   <td style="background-color: #cccccc">Open/Closed handling
   </td>
  </tr>
  <tr>
   <td>C++
   </td>
   <td>Determined by the field using the enum's file
   </td>
  </tr>
  <tr>
   <td>Java
   </td>
   <td>Determined by the field using the enum's file
   </td>
  </tr>
  <tr>
   <td>UPB (non-ruby)
   </td>
   <td>Determined by the enum's definition file
   </td>
  </tr>
  <tr>
   <td>UPB (ruby)
   </td>
   <td>All enums treated as open
   </td>
  </tr>
  <tr>
   <td>C#
   </td>
   <td><strong>All</strong> enums treated as open
   </td>
  </tr>
  <tr>
   <td>Obj-C
   </td>
   <td>by usage (&lt; 22.x)
<p>
by definition  (>= 22.x)
<p>
It looks like this was handled by the field's usage, but in Nov as part of the syntax cleanup, we stopped looking at syntax and captured things on the enum definition, so it's now defined by the enum.
   </td>
  </tr>
  <tr>
   <td>Swift
   </td>
   <td>Determined by the enum's definition file
<p>
Swift uses the ability for enums to have associated values, so an enum defined in a proto3 syntax file gets a value that holds all unknown values. So a proto2 syntax defined message will still end up with the enum using that to hold unknown values.
   </td>
  </tr>
  <tr>
   <td>Go
   </td>
   <td><strong>All</strong> enums treated as open
   </td>
  </tr>
  <tr>
   <td>Apps JSPB
   </td>
   <td><strong>All</strong> enums treated as open
   </td>
  </tr>
  <tr>
   <td>ImmutableJs
   </td>
   <td><strong>All</strong> enums treated as open
   </td>
  </tr>
  <tr>
   <td>JsProto
   </td>
   <td><strong>All</strong> enums treated as open
   </td>
  </tr>
</table>

### Impact

Approximately 2.99% of enum fields import enums across syntaxes and 1.77% of
enums are imported across syntaxes.

6.14% of fields being enum fields, meaning 0.18% of fields are affected when
used by affected languages.

## Overview

This document proposes adding an additional feature to
[Edition Zero Features](edition-zero-features.md), specified as the following
.proto fragment:

```
message Features {
  // ...
  optional bool legacy_treat_enum_as_closed = ??? [
      retention = RUNTIME,
      target = FILE,
      target = FIELD
  ];
}
```

The name of this field captures the desired intent: this is a bad legacy
behavior that we believe is rare and want to stamp out. Edition 2023 would set
this to false by default, and `proto2` would treat it as implicitly true. It
also does not permit the converse: you cannot force a field to be open, because
that is currently not possible and we don't want to add more special cases.

Additionally, we would like to make special dispensation in migration tooling
for this field: it should not be set unconditionally when migrating from proto2
-> editions, but *only* on proto2 fields that are of proto3 enum type. We should
also want to build an allowlist for this, like we do for `required`.

This option can also help in migrating enums from closed to open, since we can
use it to migrate individual use-sites by marking the enum as open and all of
its uses as treat-as-closed in one CL, and then deleting the treat-as-closed
annotations one by one.

An open (lol) question is whether we should move `is_closed` from
`EnumDescriptor` to `FieldDescriptor`.

## Recommendation

Use the "define official behavior" alternative below. Given the wide variety of
behavior in different languages, a singular global setting will always leave
some of our languages in the lurch. As such, we will use per language features
to allow each language to control its own evolution while we define the
"correct" behavior.

For example, in C++ we will define:

```
// Determines if the given enum field is treated as closed based on legacy
// non-conformant behavior.
//
// Conformant behavior determines closedness based on the enum and
// can be queried using EnumDescriptor::is_closed().
//
// Some runtimes currently have a quirk where non-closed enums are
// treated as closed when used as the type of fields defined in a
// `syntax = proto2;` file. This quirk is not present in all runtimes; as of
// writing, we know that:
//
// - C++, Java, and C++-based Python share this quirk.
// - UPB and UPB-based Python do not.
// - PHP and Ruby treat all enums as open regardless of declaration.
//
// Care should be taken when using this function to respect the target
// runtime's enum handling quirks.

bool FieldDescriptor::legacy_enum_field_treated_as_closed() const {
  return type() == TYPE_ENUM && file().syntax() == FileDescriptor::SYNTAX_PROTO2;
}
```

In Java, `FileDescriptor.supportsUnknownEnumValue()` will need to be deprecated
and replaced with the above.

## Alternatives

### Define official behavior

Define the official behavior to be "Enums open-ness should be defined by the
definition of the enum." Add a conformance test for this behavior. Use per
language features to eventually converge implementations that are out of
conformance. We choose to define this as "enum openness is defined by the
definition" because that matches the model for almost all other proto3/proto2
properties.

#### Pros

*   Clarifies desired behavior
*   Existing implementations can change incrementally using editions
*   Avoids complicating global features for something that is a per-language
    issue

#### Cons

*   When migrating from syntax to edition zero, Prototiller will need to know
    all used languages to make the upgrade a trivial change (this is already the
    case for other edition upgrades).

### Make `Features.enum` a field-level feature

Here, we don't add `legacy_treat_enum_as_closed` and instead make closeness a
bona fide property of fields, not enums.

#### Pros

*   Reflects the current behavior of Protobuf for our largest languages
    (C++/Java).
*   Removes the possibility of making a mistake in reflective code that checks
    `is_closed()` on `EnumDescriptor` rather than `FieldDescriptor`.

#### Cons

*   Doesn't handle the case for languages other than C++/Java
*   Harder to migrate individual enums to open, since the property is not in
    control of the owner of the type.
*   Conceptually unpleasant, since it gives locality to the meaning of
    `IsValid`, unless we want to believe that `IsValid` merely states whether
    the value has a name we know of.

### Allow `Features.enum` on both enums and fields

This allows enum owners some more control without needing to introduce a
strictly "legacy do not use" feature.

#### Pros

*   We don't introduce a "legacy do not use" option, and don't need to play the
    allowlist game.

#### Cons

*   We need to support closed-enums-treated-as-open, which is a functionality
    Protobuf does not offer today.

### Name the feature `Features.treat_as_closed_for_migration`

This is an aesthetic choice if we feel this is a useful knob for migration, that
still highlights its temporary nature.

#### Pros

*   We don't introduce a "legacy do not use" option, and don't need to play the
    allowlist game.
*   Clearly underscores that this is for migration in a specific desirable
    direction (closed -> open).

#### Cons

*   People may use it because they like closed enums for some reason and don't
    fully appreciate the ramifications.

### Do Nothing

We can simply keep the current editions enum semantics.

#### Pros

*   No extra work.

#### Cons

*   proto2 -> editions is not a no-op in some cases. This breaks a lot of the
    draw of moving to editions, even though it is possible to detect the no-ops
    in advance.
*   This would immediately add a blocker to our syntax reflection large-scale
    change
