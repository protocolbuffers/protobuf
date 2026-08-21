# Protobuf Editions Design: Features

**Author:** [@haberman](https://github.com/haberman),
[@fowles](https://github.com/fowles)

**Approved:** 2022-10-13

A proposal to use custom options as our way of defining and representing
features.

## Background

The [Protobuf Editions](what-are-protobuf-editions.md) project uses "editions"
to allow Protobuf to safely evolve over time. An edition is formally a set of
"features" with a default value per feature. The set of features or a default
value for a feature can only change with the introduction of a new edition.
Features define the specific points of change and evolution on a per entity
basis within a .proto file (entities being files, messages, fields, or any other
lexical element in the file). The design in this doc supplants an earlier design
which used strings for feature definition.

Protobuf already supports
[custom options](https://protobuf.dev/programming-guides/proto2#customoptions)
and we will leverage these to provide a rich syntax without introducing new
syntactic forms into Protobuf.

## Sample Usage

Here is a small sample usage of features to give a flavor for how it looks

```
edition = "2023";

package experimental.users.kfm.editions;

import "net/proto2/proto/features_cpp.proto";

option features.repeated_field_encoding = EXPANDED;
option features.enum = OPEN;
option features.(pb.cpp).string_field_type = STRING;
option features.(pb.cpp).namespace = "kfm::proto_experiments";

message Lab {
  // `Mouse` is open as it inherits the file's value.
  enum Mouse {
    UNKNOWN_MOUSE = 0;
    PINKY = 1;
    THE_BRAIN = 2;
  }
  repeated Mouse mice = 1 [features.repeated_field_encoding = PACKED];

  string name = 2;
  string address = 3 [features.(pb.cpp).string_field_type = CORD];
  string function = 4 [features.(pb.cpp).string_field_type = STRING_VIEW];
}

enum ColorChannel {
  // Turn off the option from the surrounding file
  option features.enum = CLOSED;

  UNKNOWN_COLOR_CHANNEL = 0;
  RED = 1;
  BLUE = 2;
  GREEN = 3;
  ALPHA = 4;
}
```

## Language-Specific Features

We will use extensions to manage features specific to individual code
generators.

```
// In net/proto2/proto/descriptor.proto:
syntax = "proto2";
package proto2;

message Features {
  ...
  extensions 1000;  // for features_cpp.proto
  extensions 1001;  // for features_java.proto
}

```

This will allow third-party code generators to use editions for their own
evolution as long as they reserve a single extension number in
`descriptor.proto`. Using this from a .proto file would look like this:

```
edition = "2023";

import "third_party/protobuf/compiler/cpp/features_cpp.proto"

message Bar {
  optional string str = 1 [features.(pb.cpp).string_field_type = true];
}
```

## Inheritance

To support inheritance, we will specify a single `Features` message that extends
every kind of option:

```
// In net/proto2/proto/descriptor.proto:
syntax = "proto2";
package proto2;

message Features {
  ...
}

message FileOptions {
  optional Features features = ..;
}

message MessageOptions {
  optional Features features = ..;
}
// All the other `*Options` protos.
```

At the implementation level, feature inheritance is exactly the behavior of
`MergeFrom`

```
void InheritFrom(const Features& parent, Features* child) {
  Features tmp(parent);
  tmp.MergeFrom(child);
  child->Swap(&tmp);
}
```

which means that custom backends will be able to faithfully implement
inheritance without difficulty.

## Target Attributes

While inheritance can be useful for minimizing changes or pushing defaults
broadly, it can be overused in ways that would make simple refactoring of
`.proto` files harder. Additionally, not all features are meaningful on all
entities (for example `features.enum = OPEN` is meaningless on a field).

To avoid these issues, we will introduce "target" attributes on features
(similar in concept to the "target" attribute on Java annotations).

```
enum FeatureTargetType {
  FILE = 0;
  MESSAGE = 1;
  ENUM = 2;
  FIELD = 3;
  ...
};
```

These will restrict the set of entities to which a feature may be attached.

```
message Features {
  ...

  enum EnumType {
    OPEN = 0;
    CLOSED = 1;
  }
  optional EnumType enum = 2 [
      target = ENUM
  ];
}
```

## Retention

To reduce the size of descriptors in protobuf runtimes, features will be
permitted to specify retention rules (again similar in concept to "retention"
attributes on Java annotations).

```
enum FeatureRetention {
  SOURCE = 0;
  RUNTIME = 1;
}
```

## Specification of an Edition

An edition is, effectively, an instance of the `Feature` proto which forms the
base for performing inheritance using `MergeFrom`. This allows `protoc` and
specific language generators to leverage existing formats (like text-format) for
specifying the value of features at a given edition.

Although naively we would think that field defaults are the right approach, this
does not quite work, because the default is editions-dependent. Instead, we
propose adding the following to the protoc-provided `features.proto`:

```
message Features {
  // ...
  message EditionDefault {
    optional string edition = 1;
    optional string default = 2;  // Textproto value.
  }

  extend FieldOptions {
    // Ideally this is a map, but map extensions are not permitted...
    repeated EditionDefault edition_defaults = 9001;
  }
}
```

To build the edition defaults for a particular edition `current` in the context
of a particular file `foo.proto`, we execute the following algorithm:

1.  Construct a new `Features feats;`.
2.  For each field in `Features`, take the value of the
    `Features.edition_defaults` option (call it `defaults`), and sort it by the
    value of `edition` (per the total order for edition names,
    [Life of an Edition](life-of-an-edition.md)).
3.  Binsearch for the latest edition in `defaults` that is earlier or equal to
    `current`.
    1.  If the field is of singular, scalar type, use that value as the value of
        the field in `feats`.
    2.  Otherwise, the value of the field in `feats` is given by merging all of
        the values less than `current`, starting from the oldest edition.
4.  For the purposes of this algorithm, `Features`'s fields all behave as if
    they were `required`; failure to find a default explicitly via the editions
    default search mechanism should result in a compilation error, because it
    means the file's edition is too old.
5.  For each extension of `Features` that is visible from `foo.proto` via
    imports, perform the same algorithm as above to construct the editions
    default for that extension message, and add it to `feat`.

This algorithm has the following properties:

*   Language-scoped features are discovered via imports, which is how they need
    to be imported for use in a file in the first place.
*   Every value is set explicitly, so we correctly reject too-old files.
*   Files from "the future" will not be rejected out of hand by the algorithm,
    allowing us to provide a flag like `--allow-experimental-editions` for ease
    of allowing backends to implement a new edition.

## Edition Zero Features

Putting the parts together, we can offer a potential `Feature` message for
edition zero: [Edition Zero Features](edition-zero-features.md).

```
message Features {
  enum FieldPresence {
    EXPLICIT = 0;
    IMPLICIT = 1;
    LEGACY_REQUIRED = 2;
  }
  optional FieldPresence field_presence = 1 [
      retention = RUNTIME,
      target = FIELD,
      (edition_defaults) = {
        edition: "2023", default: "EXPLICIT"
      }
  ];

  enum EnumType {
    OPEN = 0;
    CLOSED = 1;
  }
  optional EnumType enum = 2 [
      retention = RUNTIME,
      target = ENUM,
      (edition_defaults) = {
        edition: "2023", default: "OPEN"
      }
  ];

  enum RepeatedFieldEncoding {
    PACKED = 0;
    EXPANDED = 1;
  }
  optional RepeatedFieldEncoding repeated_field_encoding = 3 [
      retention = RUNTIME,
      target = FIELD,
      (edition_defaults) = {
        edition: "2023", default: "PACKED"
      }
  ];

  enum StringFieldValidation {
    REQUIRED = 0;
    HINT = 1;
    SKIP = 2;
  }
  optional StringFieldValidation string_field_validation = 4 [
      retention = RUNTIME,
      target = FIELD,
      (edition_defaults) = {
        edition: "2023", default: "REQUIRED"
      }
  ];

  enum MessageEncoding {
    LENGTH_PREFIXED = 0;
    DELIMITED = 1;
  }
  optional MessageEncoding message_encoding = 5 [
      retention = RUNTIME,
      target = FIELD,
      (edition_defaults) = {
        edition: "2023", default: "LENGTH_PREFIXED"
      }
  ];

  extensions 1000;  // for features_cpp.proto
  extensions 1001;  // for features_java.proto
}
```
