# Prototiller Requirements for Edition Zero

**Authors:** [@mkruskal-google](https://github.com/mkruskal-google)

**Approved:** 2023-07-07

## Background

[Edition Zero Features](edition-zero-features.md) lays out the design for our
first edition, which will unify proto2 and proto3 via features. In order to
migrate internal Google repositories (and aid OSS migrations), we will be
leveraging Prototiller to upgrade from legacy syntax to the new editions model.
We will also be using Prototiller for every edition bump, but that's out of
scope for this document (more general requirements are laid out in
[Prototiller Requirements for Editions](prototiller-reqs-for-editions.md)).

## Overview

The way the edition zero features were derived, there will always exist a no-op
transformation from proto2/proto3. However, the details of this transformation
can be fairly complex and depend on a lot of different factors.

A temporary script has been created as a placeholder, which manages to get
fairly good coverage of these rules. Notably, it can't handle groups inside
extensions or oneofs. This, along with its golden tests, can serve as a useful
benchmark for Prototiller.

### Feature Optimization

One important piece of Prototiller that will ease the friction of the Edition
Zero large-scale change is a feature optimization phase. If certain features
aren't necessary to make the upgrade a no-op, we shouldn't add them and should
instead rely on the edition defaults for future changes. Similarly, we should
try to minimize the total size of the change by collapsing a feature
specification to a higher level (e.g. file-level defaults of a field feature).

## Frontend Feature Transformations

This section details all of our frontend features and how to transform from
proto2/proto3 to edition zero.

### Field Presence

The `field_presence` feature defaults to `EXPLICIT`, which matches proto2/proto3
optional behavior. The `LEGACY_REQUIRED` value corresponds to proto2 required
fields, and `IMPLICIT` corresponds to non-optional proto3 fields. In order to
minimize changes, file-level defaults should be utilized.

Example transformations:

<table>
  <tr>
   <td>
<pre>syntax = "proto2";
message Foo {
  optional string bar = 1;
  required string baz = 2;
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  string bar = 1;
  string baz = 2 [
    features.field_presence = LEGACY_REQUIRED];
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto3";

message Foo {
  optional string bar = 1;
  string baz = 2;
  string bam = 3;
}</pre>
   </td>
   <td>
<pre>edition = "2023";
features.field_presence = IMPLICIT;

message Foo {
  string bar = 1 [features.field_presence = EXPLICIT];
  string baz = 2;
  string bam = 3;
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto3";

message Foo {
  optional string bar = 1;
  optional string baz = 2;
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  string bar = 1;
  string baz = 2;
}</pre>
   </td>
  </tr>
</table>

### Enum Type

The `enum_type` feature defaults to `OPEN`, which matches proto3 behavior. The
`CLOSED` value corresponds to typical proto2 behavior. In order to minimize
changes, file-level defaults should be utilized.

Example transformations:

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

enum Foo {
  VALUE1 = 0;
  VALUE2 = 1;
}</pre>
   </td>
   <td>
<pre>edition = "2023";
features.enum_type = CLOSED;

enum Foo {
  VALUE1 = 0;
  VALUE2 = 1;
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto3";

enum Foo {
  VALUE1 = 0;
  VALUE2 = 1;
}</pre>
   </td>
   <td>
<pre>edition = "2023";

enum Foo {
  VALUE1 = 0;
  VALUE2 = 1;
}</pre>
   </td>
  </tr>
</table>

### Repeated Field Encoding

The `repeated_field_encoding` feature defaults to `PACKED`, which matches proto3
behavior. The `EXPANDED` value corresponds to the default proto2 behavior. Both
proto2 and proto3 can have the default behavior overridden by using the `packed`
field option. All of these should be replaced in the migration to edition zero.
Minimization of changes will be a little more complicated here, since there
could exist files where the majority of repeated fields have been overridden.

Example transformations:

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

message Foo {
  repeated int32 bar = 1;
  repeated int32 baz = 2 [packed = true];
  repeated int32 bam = 3;
}</pre>
   </td>
   <td>
<pre>edition = "2023";
features.repeated_field_encoding = EXPANDED;

message Foo {
  repeated int32 bar = 1;
  repeated int32 baz = 2 [
    features.repeated_field_encoding = PACKED];
  repeated int32 bar = 3;
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto3";

message Foo {
  repeated int32 bar = 1;
  repeated int32 baz = 2 [packed = false];
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  repeated int32 bar = 2;
  repeated int32 baz = 2 [
    features.repeated_field_encoding = EXPANDED];
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

message Foo {
  repeated int32 x = 1 [packed = true];
  // Strings are never packed.
  repeated string z = 1;
  repeated string w = 2;
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  repeated int32 x = 1;
  repeated string z = 1;
  repeated string w = 2;
}</pre>
   </td>
  </tr>
</table>

### Message Encoding

The `message_encoding` feature is designed to replace the proto2-only `group`
syntax (with value `DELIMITED`), with a default that will always be
`LENGTH_PREFIXED`. This is a somewhat awkward transformation in the general
case, since we allow group definitions anywhere fields exist even if message
definitions can't. The basic transformation is to create a new message type in
the nearest enclosing scope with the same name as the field, and lowercase the
field and give it that type.

Example transformations:

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

message Foo {
  optional group Bar = 1 {
    optional int32 x = 1;
  }
  optional Bar baz = 2;
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  message Bar {
    int32 x = 1;
  }
  Bar bar = 1 [features.message_encoding = DELIMITED];
  Bar baz = 2;
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

message Foo {
  oneof foo {
    group Bar = 1 {
      optional int32 x = 1;
    }
  }
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  message Bar {
    int32 x = 1;
  }
  oneof foo {
    Bar bar = 1 [
      features.message_encoding = DELIMITED];
  }
}</pre>
   </td>
  </tr>
</table>

### JSON Format

The `json_format` feature is a bit of an outlier, because (at least for edition
zero) it only affects the frontend build of the proto file. The `ALLOW` value
(proto3 behavior) enables all JSON mapping conflict checks on field names,
unless `deprecated_legacy_json_field_conflicts` is set. The `LEGACY_BEST_EFFORT`
value (proto2 behavior) disables these checks. The ideal minimal transformation
would be to switch to `ALLOW` in all cases except where
`deprecated_legacy_json_field_conflicts` is set or there exist JSON mapping
conflicts. In those cases we can fallback to `LEGACY_BEST_EFFORT`.

Alternatively, if it's difficult for Prototiller to handle, we could do a
followup large-scale change to remove all `LEGACY_BEST_EFFORT` instances that
pass build.

Example transformations:

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

message Foo {
  optional string bar = 1;
  optional string baz = 2;
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  string bar = 1;
  string baz = 2;
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto3";

message Foo {
  string bar = 1;
  string baz = 2;
}</pre>
   </td>
   <td>
<pre>edition = "2023";
features.field_presence = IMPLICIT;

message Foo {
  string bar = 1;
  string baz = 2;
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

message Foo {
  // Warning only
  string bar = 1;
  string bar_ = 2;
}</pre>
   </td>
   <td>
<pre>edition = "2023";
features.json_format = LEGACY_BEST_EFFORT;

message Foo {
  string bar = 1;
  string bar_ = 2;
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto3";

message Foo {
  option
  deprecated_legacy_json_field_conflicts = true;
  string bar = 1;
  string baz = 2 [json_name = "bar"];
}</pre>
   </td>
   <td>
<pre>edition = "2023";
features.field_presence = IMPLICIT;
features.json_format = LEGACY_BEST_EFFORT;

message Foo {
  string bar = 1;
  string baz = 2;
}</pre>
   </td>
  </tr>
</table>

## Backend Feature Transformations

This section details our backend-specific features and how to transform from
proto2/proto3 to edition zero.

In order to limit bloat, it would be ideal if we could check whether or not a
proto file is ever used to generate code in the target language. If it's not,
there's no reason to add backend-specific features.

### Legacy Closed Enum

Java and C++ by default treat proto3 enums as closed if they're used in a proto2
message. The internal `cc_open_enum` field option can override this, but it has
**very** limited use and may not be worth considering. While the enum type
behavior should still be determined by Enum Type, we'll need to add this feature
to proto2 files using proto3 messages (the reverse is disallowed).

Example transformations:

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

import "some_proto3_file.proto"

enum Proto2Enum {
  BAR = 0;
}

message Foo {
  optional Proto3Enum bar = 1;
  optional Proto2Enum baz = 2;
}</pre>
   </td>
   <td>
<pre>edition = "2023";
import "third_party/protobuf/cpp_features.proto"
import "third_party/protobuf/java_features.proto"
import "some_proto3_file.proto"

features.enum_type = CLOSED;

message Foo {
  Proto3Enum bar = 1 [
    features.(pb.cpp).legacy_closed_enum = true,
    features.(pb.java).legacy_closed_enum = true];
  Proto2Enum baz = 2;
}</pre>
   </td>
  </tr>
</table>

### UTF8 Validation

This feature is pending approval of *Editions Zero Feature: utf8_validation*
(not available externally).

## Other Transformations

In addition to features, there are some other changes we've made for edition
zero.

### Reserved Identifier Syntax

With *Protobuf Change Proposal: Reserved Identifiers* (not available
externally), we've decided to switch from strings to identifiers for reserved
fields. This *should* be a trivial change, but if the proto file contains
strings that aren't valid identifiers there's some ambiguity. They're currently
ignored today, but they could be typos we wouldn't want to just blindly delete.
So instead, we'll leave behind a comment.

Example transformations:

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

message Foo {
  reserved "bar", "baz";
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  reserved bar, baz;
}</pre>
   </td>
  </tr>
</table>

<table>
  <tr>
   <td>
<pre>syntax = "proto2";

message Foo {
  reserved "bar", "1";
}</pre>
   </td>
   <td>
<pre>edition = "2023";

message Foo {
  reserved bar;
  /*reserved "1";*/
}</pre>
   </td>
  </tr>
</table>
