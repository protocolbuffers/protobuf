# Application note: Field presence

This application note explains the various presence tracking disciplies for protobuf fields. It also explains how to enable experimental support for explicit presence tracking for singular proto3 fields with basic types.

## Background

_Field presence_ is the notion of whether a protobuf field has a value. Different types of fields follow different _presence disciplines_, or semantics for translating between the _API representation_ and the _serialized representation_.

The _implicit_ presence discipline relies upon the field value itself to express notional presence. This can also be called _in-band_ presence tracking, since presence (or, usually, absence) is represented by an otherwise valid value.

The _explicit_ presence discipline relies upon state tracking which is orthogonal to a field value to express notional presence. This could also be called _external_ or _out-of-band_ presence tracking.

### Presence in _tag-value stream_ (wire format) serialization

The wire format is a stream of tagged, _self-delimiting_ values. By definition, the wire format represents a sequence of _present_ values. In other words, every value found within a serialization represents a _present_ field; furthermore, the serialization contains no information about not-present values.

The generated API for a proto message includes (de)serialization definitions which translate between API types and a stream of definitionally _present_ (tag, value) pairs. This translation is designed to be forward- and backward-compatibile across changes to the message definition; however, this compatibility introduces some (perhaps surprising) considerations when deserializing wire-formatted messages:

-   "Empty" length-delimited values (such as empty strings or empty message-typed fields) can be represented in serialized values. However, if the generated API does not track presence, then these values may not be re-serialized; i.e., the empty field may be "not present" after a serialization round-trip.
-   When deserializing, duplicate field values may be handled in different ways.
	-   Duplicate `repeated` fields are typically appended to the field's API representation. (Note that serializing a _packed_ repeated field produces only one, length-delimited value in the tag stream.)
	-   Duplicate `optional` field values follow the rule that "the last one wins."
-   `oneof` fields expose the API-level invariant that only one field is set at a time. However, the wire format may include multiple (tag, value) pairs which notionally belong to the `oneof`. Similar to `optional` fields, the generated API follows the "last one wins" rule.
-   Out-of-range values are not returned for fields in the generated API. However, out-of-range values may be stored as _unknown fields_ in the API, even though the wire-format tag is recognized.

### Presence in _named-field mapping_ formats

Protobufs can be represented in human-readable, textual forms. Two notable formats are TextFormat (the output format produced by generated message `DebugString` methods) and JSON. 

These formats have correctness requirements of their own, and are generally stricter than _tagged-value stream_ formats. However, TextFormat more closely mimics the semantics of the wire format, and does, in certain cases, provide similar semantics (for example, appending repeated name-value mappings to a repeated field). In particular, similar to the wire format, TextFormat only includes fields which are present.

JSON is a much stricter format, however, and cannot validly represent some semantics of the wire format or TextFormat. Notably, JSON _elements_ are semantically unordered, and each member must have a unique name. JSON includes a `null` value, which is often used to represent a _defined but not-present field_. Because JSON elements are unordered, there is no way to unambiguously interpret the "last one wins" rule. Notably, this means that it may not be possible to interpret `oneof` fields unambiguously. In theory, JSON _can_ represent presence in a semantic-preserving fashion; in practice, however, presence correctness can vary depending upon implementation choices, especially if JSON was chosen as a means to interoperate with clients not using protobufs.

### Presence in proto2 APIs

This table outlines the presence disciplines for fields in proto2 APIs (either generated APIs or using dynamic reflection):

Field type                                   | Presence tracking
-------------------------------------------- | -----------------
Singular numeric (integer or floating point) | Explicit
Singular enum                                | Explicit
Singular string or bytes                     | Explicit
Singular message                             | Explicit
Repeated                                     | Implicit
Oneofs                                       | Explicit
Maps                                         | Implicit

Singular fields (of all types) track presence explicitly in the generated API. The generated message interface includes methods to query presence of fields. For example, the field `foo` has a corresponding `has_foo` method. (The specific name follows the same language-specific naming convention as the field accessors.) These methods are sometimes referred to as "hazzers" within the protobuf implementation.

Similar to singular fields, `oneof` fields explicitly track which one of the members, if any, contains a value. For example, consider this example `oneof`:

```
oneof foo {
  int32 a = 1;
  float b = 2;
}
```

Depending on the target language, the generated API would generally include several methods:

-   A hazzer for the oneof: `has_foo`
-   A _oneof case_ method: `foo`
-   Hazzers for the members: `has_a`, `has_b`
-   Getters for the members: `a`, `b`

Repeated fields and maps follow implicit presence: there is no distinction between an _empty_ and a _not-present_ repeated field.

### Presence in proto3 APIs

This table outlines the presence disciplines used for fields in proto3 APIs (either generated APIs or using dynamic reflection):

Field type                                   | `optional` | Presence tracking
-------------------------------------------- | ---------- | -----------------
Singular numeric (integer or floating point) | No         | Implicit
Singular enum                                | No         | Implicit (see below)
Singular string or bytes                     | No         | Implicit
Singular numeric (integer or floating point) | Yes        | Explicit
Singular enum                                | Yes        | Explicit
Singular string or bytes                     | Yes        | Explicit
Singular message                             | N/A        | Explicit or Hybrid
Repeated                                     | N/A        | Implicit
Oneofs                                       | N/A        | Explicit
Maps                                         | N/A        | Implicit

Similar to proto2 APIs, proto3 uses implicit presence tracking for repeated fields. As of the protobuf 3.12 release, singular fields of basic types (numeric, string, bytes, and enums) with the `optional` label follow the _explicit presence_ discipline, like proto2.

Without the `optional` label, proto3 APIs follow the _implicit presence_ discipline for basic types (numeric, string, bytes, and enums). For string- and bytes-typed fields, the default value is the zero-length value; for numeric fields, this is the 0 value.

The default value for enum-typed fields under implicit presence is the corresponding 0-mapped enumerator. Under proto3 syntax rules, all enum types are required to have an enumerator value which maps to 0. By convention, this is an `UNKNOWN` or similarly-named enumerator. Depending on whether the zero value is notionally within the domain of valid values, this may be tantamount to _explicit presence_ discipline for some applications.

Under the _implicit presence_ discipline, the API considers the default value to be synonymous with "not present." So, if a field contains its default value, it is not serialized; to notionally "clear" a field, an API user sets it to the default value.

Message-typed fields generally expose some notion of presence under proto3 APIs, usually be exposing a language-specific `null` value when the field is not "present." However, generated APIs may also expose "hybrid" getters, so that applications do not have to check nullity (similar to proto2 APIs, these "hybrid" proto3 APIs return a default, un-populated message value if the underlying field is not present).

Oneof fields affirmatively expose presence, although the same set of hazzer methods may not generated as in proto2 APIs.

## Semantic differences

When the _implicit presence_ discipline is used for a singular field, the default value acts as a sentiel for presence; however, under _explicit presence_ discipline, the field's presence is tracked separately, without consideration of the value. This results in some semantic differences between the presence tracking disciplines when the default value is set.

For a singular field with numeric, enum, or string type:

-   _Implicit presence_ discipline:
    -   The default value may mean:
		-   the field was explicitly cleared;
		-   the field was explicitly set to its default value, and the default value is within the application-specific domain of values; or
		-   the field was never set to a value.
	-   To "clear" a field, it is set to its default value.
	-   Default values are not serialized.
	-   Default values are not merged-from.
-   _Explicit presence_ discipline:
	-   A generated `has_foo` method indicates whether or not the field `foo` has been set (and not cleared).
	-   A generated `clear_foo` method must be used to clear (i.e., un-set) the value.
	-   Explicitly set values are always serialized, including default values.
	-   Un-set fields are never merged-from. Explicitly set fields -- including default values -- _are_ merged-from.

### Considerations for merging

Under the _implicit presence_ discipline, it is effectively impossible to set a target field to the default value using the protobuf's API merging functions. This is because only non-default values are "present," and merging only updates the target message using "present" values from the update message.

The difference in merging behavior has further implications for protocols which rely on partial "patch" updates. If field presence is not tracked, then an update patch alone cannot represent an update to the default value: only non-default values are merged-from. Updating to set a default value in this case requires some external mechanism, such as `FieldMask`. However, if presence _is_ tracked, then all explicitly-set values -- even default values -- will be merged into the target.

### Considerations for change-compatibility

Changing a field between explicit and implicit tracking disciplines is a binary-compatible change for serialized values.

However, the serialized representation of the message may differ, depending on which version of the message definition was used for serialization. This change may or may not be safe, depending on the application's semantics.

For example, consider two clients with different versions of a message definition. Client A uses this definition of the message, using the _implicit presence_ discipline for field `foo`:

```
syntax = "proto3";
message Msg {
  int32 foo = 1;
}
```

Client B uses a definition of the same message, except that it uses the _explicit presence_ discipline:

```
syntax = "proto3";
message Msg {
  optional int32 foo = 1;
}
```

Now, consider scenarios where the clients repeatedly exchange a serialized message value, and client B checks for presence:

```
// Client A:
Msg m_a;
m_a.set_foo(1);
assert(m_a.has_foo());           // OK
Send(m_a.SerializeAsString());   // to channel

// Client B:
Msg m_b;
m_b.ParseFromString(Receive());  // from channel
assert(m_b.foo() == 1);          // OK
Send(m_b.SerializeAsString());   // to channel

// Client A:
m_a.ParseFromString(Receive());  // from channel
assert(m_a.has_foo());           // OK
assert(m_a.foo() == 1);          // OK
m_a.set_foo(0);
Send(m_a.SerializeAsString());   // to channel

// Client B:
Msg m_b;
m_b.ParseFromString(Receive());  // from channel
assert(m_b.foo() == 1);          // OK
Send(m_b.SerializeAsString());   // to channel

// Client A:
m_a.ParseFromString(Receive());  // from channel
assert(m_a.has_foo());           // FAIL
```

If client A depends on the _explicit presence_ discipline for field, then a "round trip" through client B following the _implicit presence_ discipline will be lossy (and hence not a safe change) from the perspective of client A.

## How to enable _explicit presence_ in proto3

These are the general steps to use the experimental field tracking support for proto3:

1.  Add an `optional` field to a `.proto` file.
1.  Run `protoc` (from release 3.12 or later) with an extra flag to recognize `optional` (i.e,. explicit presence) in proto3 files.
1.  Use the generated "hazzer" methods and "clear" methods in application code, instead of comparing or setting default values.

### `.proto` file changes

This is an example of a proto3 message with fields which follow both implicit and explicit presence disciplines:

```
syntax = "proto3";
package example;

message MyMessage {
  // No presence tracking (implicit presence):
  int32 not_tracked = 1;

  // Presence tracking (explicit presence):
  optional int32 tracked = 2;
}
```

### `protoc` invocation

To enable presence tracking for proto3 messages, pass the `--experimental_allow_proto3_optional` flag to protoc. Without this flag, the `optional` label is an error in files using proto3 syntax. This flag is available in protobuf release 3.12 or later (or at HEAD, if you are reading this application note from Git).

### Using the generated code

The generated code for proto3 fields with explicit presence (the `optional` label) will be the same as it would be in a proto2 file.

This is the definition used in the "implicit presence" examples below:

```
syntax = "proto3";
package example;
message Msg {  
  int32 foo = 1;
}
```

This is the definition used in the "explicit presence" examples below:

```
syntax = "proto3";
package example;
message Msg {
  optional int32 foo = 1;
}
```

In the examples, a function `GetProto` constructs and returns a message of type `Msg` with unspecified contents.

#### C++ example

Implicit presence:

```
Msg m = GetProto();
if (m.foo() != 0) {
  // "Clear" the field:
  m.set_foo(0);
} else {
  // Default value: field may not have been present.
  m.set_foo(1);
}
```

Explicit presence:

```
Msg m = GetProto();
if (m.has_foo()) {
  // Clear the field:
  m.clear_foo();
} else {
  // Field is not present, so set it.
  m.set_foo(1);
}
```

#### C# example

Implicit presence:

```
var m = GetProto();
if (m.Foo != 0) {
  // "Clear" the field:
  m.Foo = 0;
} else {
  // Default value: field may not have been present.
  m.Foo = 1;
}
```

Explicit presence:

```
var m = GetProto();
if (m.HasFoo) {
  // Clear the field:
  m.ClearFoo();
} else {
  // Field is not present, so set it.
  m.Foo = 1;
}
```

#### Go example

Implicit presence:

```
m := GetProto()
if (m.GetFoo() != 0) {
  // "Clear" the field:
  m.Foo = 0;
} else {
  // Default value: field may not have been present.
  m.Foo = 1;
}
```

Explicit presence:

```
m := GetProto()
if (m.HasFoo()) {
  // Clear the field:
  m.Foo = nil
} else {
  // Field is not present, so set it.
  m.Foo = proto.Int32(1);
}
```

#### Java example

These examples use a `Builder` to demonstrate clearing. Simply checking presence and getting values from a `Builder` follows the same API as the message type.

Implicit presence:

```
Msg.Builder m = GetProto().toBuilder();
if (m.getFoo() != 0) {
  // "Clear" the field:
  m.setFoo(0);
} else {
  // Default value: field may not have been present.
  m.setFoo(1);
}
```

Explicit presence:

```
Msg.Builder m = GetProto().toBuilder();
if (m.hasFoo()) {
  // Clear the field:
  m.clearFoo()
} else {
  // Field is not present, so set it.
  m.setFoo(1);
}
```

#### Python example

Implicit presence:

```
m = example.Msg()
if m.foo != 0:
  // "Clear" the field:
  m.foo = 0
else:
  // Default value: field may not have been present.
  m.foo = 1
```

Explicit presence:

```
m = example.Msg()
if m.HasField('foo'):
  // Clear the field:
  m.ClearField('foo')
else:
  // Field is not present, so set it.
  m.foo = 1
```

#### Ruby example

Implicit presence:

```
m = Msg.new
if m.foo != 0
  // "Clear" the field:
  m.foo = 0
else
  // Default value: field may not have been present.
  m.foo = 1
end
```

Explicit presence:

```
m = Msg.new
if m.has_foo?
  // Clear the field:
  m.clear_foo
else
  // Field is not present, so set it.
  m.foo = 1
end
```

#### Javascript example

Implicit presence:

```
var m = new Msg();
if (m.getFoo() != 0) {
  // "Clear" the field:
  m.setFoo(0);
} else {
  // Default value: field may not have been present.
  m.setFoo(1);
}
```

Explicit presence:

```
var m = new Msg();
if (m.hasFoo()) {
  // Clear the field:
  m.clearFoo()
} else {
  // Field is not present, so set it.
  m.setFoo(1);
}
```

#### Objective C example

Implicit presence:

```
Msg *m = [[Msg alloc] init];
if (m.foo != 0) {
  // "Clear" the field:
  m.foo = 0;
} else {
  // Default value: field may not have been present.
  m.foo = 1;
}
```

Explicit presence:

```
Msg *m = [[Msg alloc] init];
if (m.hasFoo()) {
  // Clear the field:
  [m clearFoo];
} else {
  // Field is not present, so set it.
  [m setFoo:1];
}
```
