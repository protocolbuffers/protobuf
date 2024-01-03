# Prototiller Requirements for Editions

**Author:** [@mcy](https://github.com/mcy)

**Approved:** 2022-11-29

## Background

Prototiller is Protobuf's new mass refactoring Swiss army knife, similar to
Buildozer. We plan to use Prototiller to enable LSCs within google3 and to allow
users (internal and external) to modify `.proto` files safely.

Prototiller is being developed as part of the
[Editions](../editions/what-are-protobuf-editions.md) project, and will
prioritize enabling Editions-related refactorings to unblock Editions migrations
in 2023. This document describes the relevant requirements.

## Overview

*Protochangifier Semantic Actions* (not available externally) describes the
original design for the Prototiller interface; it would consume a Protobuf
message that described changes to apply to a `.proto` file passed as input. In
this document, we prescribe a variant of this interface that fulfills *only* the
needs of Editions, while remaining extensible for future change actions.

Broad requirements are as follows:

*   Actions must include the following Editions-oriented upgrade workflows:
    *   Upgrade a file to a particular edition, regardless of whether it's in
        syntax mode or editions mode, updating features in such a way to be a
        no-op. **This is the highest-priority workflow.**
    *   "Clean up" features in a particular file: i.e., run a simple algorithm
        to determine the smallest set of features that need to be present at
        each level of the file.
    *   Modify features from a particular syntax element.
*   Actions must be both specific to particular syntax elements (for when change
    specs are checked in alongside `.proto` files by Schema Consumers), and
    generic (so that a single change spec or set of change specs can power a
    large-scale change).

In this document we provide a recommendation for a Protobuf schema based on the
original Protochangifier design, but geared towards these specific needs.

This is only a recommendation; the Prototiller project owners should modify this
to suit the implementation; only the requirements in this document are binding,
and the schema is merely an illustration of those requirements.

The suggested schema is as follows.

```
syntax = "proto2";

package prototiller;

// This is the proto that Prototiller accepts as input.
message ChangeSpec {
  // Actions to execute on the file.
  repeated Action actions = 1;

  // Some changes may result in a wireformat break; changing field type is
  // usually unsafe. By default, Prototiller does not allow such changes,
  // users can set allow_unsafe_wire_format_changes to true to force the change.
  optional bool allow_unsafe_wire_format_changes = 2 [default = false];
  optional bool allow_unsafe_text_format_changes = 3 [default = false];
  optional bool allow_unsafe_json_format_changes = 4 [default = false];
}

// A single action. See messages below for description of their
// semantics.
message Action {
  oneof kind {
    UpgradeEdition upgrade_edition = 20;
    CleanUpFeatures clean_up_features = 21;
    ModifyFeature modify_feature = 22;
  }
}

// Upgrades the edition of a file to a specified edition.
// Treats syntax mode as being a weird, special edition that cannot be
// upgraded to.
//
// This action is always safe.
message UpgradeEdition {
  // The edition to upgrade to.
  optional string edition = 1;
}

// Cleans up features in a file, such that there are as few explicitly set
// features as necessary.
//
// This action is always safe.
message CleanUpFeatures {}

// Modifies a specific feature on all syntax elements that match and which can
// host that particular feature.
//
// Prototiller must be aware of which changes affect wire format, so that it
// can flag them as unsafe.
message ModifyFeature {
  // The name of the feature to modify.
  repeated proto2.UninterpretedOption.NamePart feature = 1;

  // A pattern for matching paths to syntax elements to modify.
  //
  // Elements of this field can either be identifiers, or the string "*", which
  // matches all identifiers. Thus, ["foo", "Bar"] matches the message foo.Bar,
  // ["foo", "Bar", "*"] matches all fields and nested types of foo.Bar
  // (recursively), and ["*"] matches all elements of a file.
  repeated string path_pattern = 2;

  // The value to set the feature to. If not set, this means that the
  // feature should be deleted.
  oneof value {
    int64 int_value = 20;
    double double_value = 21;
    // ... and so on.
  }
}
```

## Alternatives Considered

This document does not capture a design so much as requirements that a design
must satisfy, so we will be brief on potential alternatives to the requirements,
and why we decided against them.

*   Omit feature cleanup as its own action, and let it happen implicitly as part
    of other actions.
    *   It is desirable to be able to aggressively run this operation
        everywhere, potentially even as part of "format on save" in Cider and
        other IDEs.
*   Make `ModifyFeature` operate on all syntax elements of a file
    simultaneously.
    *   `ModifyFeature` is intended so that SchemaConsumers can affect
        fine-grained control of features in `.proto` files they import. Users
        will want to be able to wipe out a feature from all fields in a file, or
        perhaps just on a handful of fields they care about. Offering simple
        pattern-matching supports both.
