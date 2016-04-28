# Protocol Buffers in Swift

## Objective

This document describes the user-facing API and internal implementation of
proto2 and proto3 messages in Apple’s Swift programming language.

One of the key goals of protobufs is to provide idiomatic APIs for each
language. In that vein, **interoperability with Objective-C is a non-goal of
this proposal.** Protobuf users who need to pass messages between Objective-C
and Swift code in the same application should use the existing Objective-C proto
library. The goal of the effort described here is to provide an API for protobuf
messages that uses features specific to Swift—optional types, algebraic
enumerated types, value types, and so forth—in a natural way that will delight,
rather than surprise, users of the language.

## Naming

*   By convention, both typical protobuf message names and Swift structs/classes
    are `UpperCamelCase`, so for most messages, the name of a message can be the
    same as the name of its generated type. (However, see the discussion below
    about prefixes under [Packages](#packages).)

*   Enum cases in protobufs typically are `UPPERCASE_WITH_UNDERSCORES`, whereas
    in Swift they are `lowerCamelCase` (as of the Swift 3 API design
    guidelines). We will transform the names to match Swift convention, using
    a whitelist similar to the Objective-C compiler plugin to handle commonly
    used acronyms.

*   Typical fields in proto messages are `lowercase_with_underscores`, while in
    Swift they are `lowerCamelCase`. We will transform the names to match
    Swift convention by removing the underscores and uppercasing the subsequent
    letter.

## Swift reserved words

Swift has a large set of reserved words—some always reserved and some
contextually reserved (that is, they can be used as identifiers in contexts
where they would not be confused). As of Swift 2.2, the set of always-reserved
words is:

```
_, #available, #column, #else, #elseif, #endif, #file, #function, #if, #line,
#selector, as, associatedtype, break, case, catch, class, continue, default,
defer, deinit, do, dynamicType, else, enum, extension, fallthrough, false, for,
func, guard, if, import, in, init, inout, internal, is, let, nil, operator,
private, protocol, public, repeat, rethrows, return, self, Self, static,
struct, subscript, super, switch, throw, throws, true, try, typealias, var,
where, while
```

The set of contextually reserved words is:

```
associativity, convenience, dynamic, didSet, final, get, infix, indirect,
lazy, left, mutating, none, nonmutating, optional, override, postfix,
precedence, prefix, Protocol, required, right, set, Type, unowned, weak,
willSet
```

It is possible to use any reserved word as an identifier by escaping it with
backticks (for example, ``let `class` = 5``). Other name-mangling schemes would
require us to transform the names themselves (for example, by appending an
underscore), which requires us to then ensure that the new name does not collide
with something else in the same namespace.

While the backtick feature may not be widely known by all Swift developers, a
small amount of user education can address this and it seems like the best
approach. We can unconditionally surround all property names with backticks to
simplify generation.

Some remapping will still be required, though, to avoid collisions between
generated properties and the names of methods and properties defined in the base
protocol/implementation of messages.

# Features of Protocol Buffers

This section describes how the features of the protocol buffer syntaxes (proto2
and proto3) map to features in Swift—what the code generated from a proto will
look like, and how it will be implemented in the underlying library.

## Packages

Modules are the main form of namespacing in Swift, but they are not declared
using syntactic constructs like namespaces in C++ or packages in Java. Instead,
they are tied to build targets in Xcode (or, in the future with open-source
Swift, declarations in a Swift Package Manager manifest). They also do not
easily support nesting submodules (Clang module maps support this, but pure
Swift does not yet provide a way to define submodules).

We will generate types with fully-qualified underscore-delimited names. For
example, a message `Baz` in package `foo.bar` would generate a struct named
`Foo_Bar_Baz`. For each fully-qualified proto message, there will be exactly one
unique type symbol emitted in the generated binary.

Users are likely to balk at the ugliness of underscore-delimited names for every
generated type. To improve upon this situation, we will add a new string file
level option, `swift_package_typealias`, that can be added to `.proto` files.
When present, this will cause `typealias`es to be added to the generated Swift
messages that replace the package name prefix with the provided string. For
example, the following `.proto` file:

```protobuf
option swift_package_typealias = "FBP";
package foo.bar;

message Baz {
  // Message fields
}
```

would generate the following Swift source:

```swift
public struct Foo_Bar_Baz {
  // Message fields and other methods
}

typealias FBPBaz = Foo_Bar_Baz
```

It should be noted that this type alias is recorded in the generated
`.swiftmodule` so that code importing the module can refer to it, but it does
not cause a new symbol to be generated in the compiled binary (i.e., we do not
risk compiled size bloat by adding `typealias`es for every type).

Other strategies to handle packages that were considered and rejected can be
found in [Appendix A](#appendix-a-rejected-strategies-to-handle-packages).

## Messages

Proto messages are natural value types and we will generate messages as structs
instead of classes. Users will benefit from Swift’s built-in behavior with
regard to mutability. We will define a `ProtoMessage` protocol that defines the
common methods and properties for all messages (such as serialization) and also
lets users treat messages polymorphically. Any shared method implementations
that do not differ between individual messages can be implemented in a protocol
extension.

The backing storage itself for fields of a message will be managed by a
`ProtoFieldStorage` type that uses an internal dictionary keyed by field number,
and whose values are the value of the field with that number (up-cast to Swift’s
`Any` type). This class will provide type-safe getters and setters so that
generated messages can manipulate this storage, and core serialization logic
will live here as well. Furthermore, factoring the storage out into a separate
type, rather than inlining the fields as stored properties in the message
itself, lets us implement copy-on-write efficiently to support passing around
large messages. (Furthermore, because the messages themselves are value types,
inlining fields is not possible if the fields are submessages of the same type,
or a type that eventually includes a submessage of the same type.)

### Required fields (proto2 only)

Required fields in proto2 messages seem like they could be naturally represented
by non-optional properties in Swift, but this presents some problems/concerns.

Serialization APIs permit partial serialization, which allows required fields to
remain unset. Furthermore, other language APIs still provide `has*` and `clear*`
methods for required fields, and knowing whether a property has a value when the
message is in memory is still useful.

For example, an e-mail draft message may have the “to” address required on the
wire, but when the user constructs it in memory, it doesn’t make sense to force
a value until they provide one. We only want to force a value to be present when
the message is serialized to the wire. Using non-optional properties prevents
this use case, and makes client usage awkward because the user would be forced
to select a sentinel or placeholder value for any required fields at the time
the message was created.

### Default values

In proto2, fields can have a default value specified that may be a value other
than the default value for its corresponding language type (for example, a
default value of 5 instead of 0 for an integer). When reading a field that is
not explicitly set, the user expects to get that value. This makes Swift
optionals (i.e., `Foo?`) unsuitable for fields in general. Unfortunately, we
cannot implement our own “enhanced optional” type without severely complicating
usage (Swift’s use of type inference and its lack of implicit conversions would
require manual unwrapping of every property value).

Instead, we can use **implicitly unwrapped optionals.** For example, a property
generated for a field of type `int32` would have Swift type `Int32!`. These
properties would behave with the following characteristics, which mirror the
nil-resettable properties used elsewhere in Apple’s SDKs (for example,
`UIView.tintColor`):

*   Assigning a non-nil value to a property sets the field to that value.
*   Assigning nil to a property clears the field (its internal representation is
    nilled out).
*   Reading the value of a property returns its value if it is set, or returns
    its default value if it is not set. Reading a property never returns nil.

The final point in the list above implies that the optional cannot be checked to
determine if the field is set to a value other than its default: it will never
be nil. Instead, we must provide `has*` methods for each field to allow the user
to check this. These methods will be public in proto2. In proto3, these methods
will be private (if generated at all), since the user can test the returned
value against the zero value for that type.

### Autocreation of nested messages

For convenience, dotting into an unset field representing a nested message will
return an instance of that message with default values. As in the Objective-C
implementation, this does not actually cause the field to be set until the
returned message is mutated. Fortunately, thanks to the way mutability of value
types is implemented in Swift, the language automatically handles the
reassignment-on-mutation for us. A static singleton instance containing default
values can be associated with each message that can be returned when reading, so
copies are only made by the Swift runtime when mutation occurs. For example,
given the following proto:

```protobuf
message Node {
  Node child = 1;
  string value = 2 [default = "foo"];
}
```

The following Swift code would act as commented, where setting deeply nested
properties causes the copies and mutations to occur as the assignment statement
is unwound:

```swift
var node = Node()

let s = node.child.child.value
// 1. node.child returns the "default Node".
// 2. Reading .child on the result of (1) returns the same default Node.
// 3. Reading .value on the result of (2) returns the default value "foo".

node.child.child.value = "bar"
// 4. Setting .value on the default Node causes a copy to be made and sets
//    the property on that copy. Subsequently, the language updates the
//    value of "node.child.child" to point to that copy.
// 5. Updating "node.child.child" in (4) requires another copy, because
//    "node.child" was also the instance of the default node. The copy is
//    assigned back to "node.child".
// 6. Setting "node.child" in (5) is a simple value reassignment, since
//    "node" is a mutable var.
```

In other words, the generated messages do not internally have to manage parental
relationships to backfill the appropriate properties on mutation. Swift provides
this for free.

## Scalar value fields

Proto scalar value fields will map to Swift types in the following way:

.proto Type | Swift Type
----------- | -------------------
`double`    | `Double`
`float`     | `Float`
`int32`     | `Int32`
`int64`     | `Int64`
`uint32`    | `UInt32`
`uint64`    | `UInt64`
`sint32`    | `Int32`
`sint64`    | `Int64`
`fixed32`   | `UInt32`
`fixed64`   | `UInt64`
`sfixed32`  | `Int32`
`sfixed64`  | `Int64`
`bool`      | `Bool`
`string`    | `String`
`bytes`     | `Foundation.NSData`

The proto spec defines a number of integral types that map to the same Swift
type; for example, `intXX`, `sintXX`, and `sfixedXX` are all signed integers,
and `uintXX` and `fixedXX` are both unsigned integers. No other language
implementation distinguishes these further, so we do not do so either. The
rationale is that the various types only serve to distinguish how the value is
**encoded on the wire**; once loaded in memory, the user is not concerned about
these variations.

Swift’s lack of implicit conversions among types will make it slightly annoying
to use these types in a context expecting an `Int`, or vice-versa, but since
this is a data-interchange format with explicitly-sized fields, we should not
hide that information from the user. Users will have to explicitly write
`Int(message.myField)`, for example.

## Embedded message fields

Embedded message fields can be represented using an optional variable of the
generated message type. Thus, the message

```protobuf
message Foo {
  Bar bar = 1;
}
```

would be represented in Swift as

```swift
public struct Foo: ProtoMessage {
  public var bar: Bar! {
    get { ... }
    set { ... }
  }
}
```

If the user explicitly sets `bar` to nil, or if it was never set when read from
the wire, retrieving the value of `bar` would return a default, statically
allocated instance of `Bar` containing default values for its fields. This
achieves the desired behavior for default values in the same way that scalar
fields are designed, and also allows users to deep-drill into complex object
graphs to get or set fields without checking for nil at each step.

## Enum fields

The design and implementation of enum fields will differ somewhat drastically
depending on whether the message being generated is a proto2 or proto3 message.

### proto2 enums

For proto2, we do not need to be concerned about unknown enum values, so we can
use the simple raw-value enum syntax provided by Swift. So the following enum in
proto2:

```protobuf
enum ContentType {
  TEXT = 0;
  IMAGE = 1;
}
```

would become this Swift enum:

```swift
public enum ContentType: Int32, NilLiteralConvertible {
  case text = 0
  case image = 1

  public init(nilLiteral: ()) {
    self = .text
  }
}
```

See below for the discussion about `NilLiteralConvertible`.

### proto3 enums

For proto3, we need to be able to preserve unknown enum values that may come
across the wire so that they can be written back if unmodified. We can
accomplish this in Swift by using a case with an associated value for unknowns.
So the following enum in proto3:

```protobuf
enum ContentType {
  TEXT = 0;
  IMAGE = 1;
}
```

would become this Swift enum:

```swift
public enum ContentType: RawRepresentable, NilLiteralConvertible {
  case text
  case image
  case UNKNOWN_VALUE(Int32)

  public typealias RawValue = Int32

  public init(nilLiteral: ()) {
    self = .text
  }

  public init(rawValue: RawValue) {
    switch rawValue {
      case 0: self = .text
      case 1: self = .image
      default: self = .UNKNOWN_VALUE(rawValue)
  }

  public var rawValue: RawValue {
    switch self {
      case .text: return 0
      case .image: return 1
      case .UNKNOWN_VALUE(let value): return value
    }
  }
}
```

Note that the use of a parameterized case prevents us from inheriting from the
raw `Int32` type; Swift does not allow an enum with a raw type to have cases
with arguments. Instead, we must implement the raw value initializer and
computed property manually. The `UNKNOWN_VALUE` case is explicitly chosen to be
"ugly" so that it stands out and does not conflict with other possible case
names.

Using this approach, proto3 consumers must always have a default case or handle
the `.UNKNOWN_VALUE` case to satisfy case exhaustion in a switch statement; the
Swift compiler considers it an error if switch statements are not exhaustive.

### NilLiteralConvertible conformance

This is required to clean up the usage of enum-typed properties in switch
statements. Unlike other field types, enum properties cannot be
implicitly-unwrapped optionals without requiring that uses in switch statements
be explicitly unwrapped. For example, if we consider a message with the enum
above, this usage will fail to compile:

```swift
// Without NilLiteralConvertible conformance on ContentType
public struct SomeMessage: ProtoMessage {
  public var contentType: ContentType! { ... }
}

// ERROR: no case named text or image
switch someMessage.contentType {
  case .text: { ... }
  case .image: { ... }
}
```

Even though our implementation guarantees that `contentType` will never be nil,
if it is an optional type, its cases would be `some` and `none`, not the cases
of the underlying enum type. In order to use it in this context, the user must
write `someMessage.contentType!` in their switch statement.

Making the enum itself `NilLiteralConvertible` permits us to make the property
non-optional, so the user can still set it to nil to clear it (i.e., reset it to
its default value), while eliminating the need to explicitly unwrap it in a
switch statement.

```swift
// With NilLiteralConvertible conformance on ContentType
public struct SomeMessage: ProtoMessage {
  // Note that the property type is no longer optional
  public var contentType: ContentType { ... }
}

// OK: Compiles and runs as expected
switch someMessage.contentType {
  case .text: { ... }
  case .image: { ... }
}

// The enum can be reset to its default value this way
someMessage.contentType = nil
```

One minor oddity with this approach is that nil will be auto-converted to the
default value of the enum in any context, not just field assignment. In other
words, this is valid:

```swift
func foo(contentType: ContentType) { ... }
foo(nil) // Inside foo, contentType == .text
```

That being said, the advantage of being able to simultaneously support
nil-resettability and switch-without-unwrapping outweighs this side effect,
especially if appropriately documented. It is our hope that a new form of
resettable properties will be added to Swift that eliminates this inconsistency.
Some community members have already drafted or sent proposals for review that
would benefit our designs:

*   [SE-0030: Property Behaviors]
    (https://github.com/apple/swift-evolution/blob/master/proposals/0030-property-behavior-decls.md)
*   [Drafted: Resettable Properties]
    (https://github.com/patters/swift-evolution/blob/master/proposals/0000-resettable-properties.md)

### Enum aliases

The `allow_alias` option in protobuf slightly complicates the use of Swift enums
to represent that type, because raw values of cases in an enum must be unique.
Swift lets us define static variables in an enum that alias actual cases. For
example, the following protobuf enum:

```protobuf
enum Foo {
  option allow_alias = true;
  BAR = 0;
  BAZ = 0;
}
```

will be represented in Swift as:

```swift
public enum Foo: Int32, NilLiteralConvertible {
  case bar = 0
  static public let baz = bar

  // ... etc.
}

// Can still use .baz shorthand to reference the alias in contexts
// where the type is inferred
```

That is, we use the first name as the actual case and use static variables for
the other aliases. One drawback to this approach is that the static aliases
cannot be used as cases in a switch statement (the compiler emits the error
*“Enum case ‘baz’ not found in type ‘Foo’”*). However, in our own code bases,
there are only a few places where enum aliases are not mere renamings of an
older value, but they also don’t appear to be the type of value that one would
expect to switch on (for example, a group of named constants representing
metrics rather than a set of options), so this restriction is not significant.

This strategy also implies that changing the name of an enum and adding the old
name as an alias below the new name will be a breaking change in the generated
Swift code.

## Oneof types

The `oneof` feature represents a “variant/union” data type that maps nicely to
Swift enums with associated values (algebraic types). These fields can also be
accessed independently though, and, specifically in the case of proto2, it’s
reasonable to expect access to default values when accessing a field that is not
explicitly set.

Taking all this into account, we can represent a `oneof` in Swift with two sets
of constructs:

*   Properties in the message that correspond to the `oneof` fields.
*   A nested enum named after the `oneof` and which provides the corresponding
    field values as case arguments.

This approach fulfills the needs of proto consumers by providing a
Swift-idiomatic way of simultaneously checking which field is set and accessing
its value, providing individual properties to access the default values
(important for proto2), and safely allows a field to be moved into a `oneof`
without breaking clients.

Consider the following proto:

```protobuf
message MyMessage {
  oneof record {
    string name = 1 [default = "unnamed"];
    int32 id_number = 2 [default = 0];
  }
}
```

In Swift, we would generate an enum, a property for that enum, and properties
for the fields themselves:

```swift
public struct MyMessage: ProtoMessage {
  public enum Record: NilLiteralConvertible {
    case name(String)
    case idNumber(Int32)
    case NOT_SET

    public init(nilLiteral: ()) { self = .NOT_SET }
  }

  // This is the "Swifty" way of accessing the value
  public var record: Record { ... }

  // Direct access to the underlying fields
  public var name: String! { ... }
  public var idNumber: Int32! { ... }
}
```

This makes both usage patterns possible:

```swift
// Usage 1: Case-based dispatch
switch message.record {
  case .name(let name):
    // Do something with name if it was explicitly set
  case .idNumber(let id):
    // Do something with id_number if it was explicitly set
  case .NOT_SET:
    // Do something if it’s not set
}

// Usage 2: Direct access for default value fallback
// Sets the label text to the name if it was explicitly set, or to
// "unnamed" (the default value for the field) if id_number was set
// instead
let myLabel = UILabel()
myLabel.text = message.name
```

As with proto enums, the generated `oneof` enum conforms to
`NilLiteralConvertible` to avoid switch statement issues. Setting the property
to nil will clear it (i.e., reset it to `NOT_SET`).

## Unknown Fields (proto2 only)

To be written.

## Extensions (proto2 only)

To be written.

## Reflection and Descriptors

We will not include reflection or descriptors in the first version of the Swift
library. The use cases for reflection on mobile are not as strong and the static
data to represent the descriptors would add bloat when we wish to keep the code
size small.

In the future, we will investigate whether they can be included as extensions
which might be able to be excluded from a build and/or automatically dead
stripped by the compiler if they are not used.

## Appendix A: Rejected strategies to handle packages

### Each package is its own Swift module

Each proto package could be declared as its own Swift module, replacing dots
with underscores (e.g., package `foo.bar` becomes module `Foo_Bar`). Then, users
would simply import modules containing whatever proto modules they want to use
and refer to the generated types by their short names.

**This solution is simply not possible, however.** Swift modules cannot
circularly reference each other, but there is no restriction against proto
packages doing so. Circular imports are forbidden (e.g., `foo.proto` importing
`bar.proto` importing `foo.proto`), but nothing prevents package `foo` from
using a type in package `bar` which uses a different type in package `foo`, as
long as there is no import cycle. If these packages were generated as Swift
modules, then `Foo` would contain an `import Bar` statement and `Bar` would
contain an `import Foo` statement, and there is no way to compile this.

### Ad hoc namespacing with structs

We can “fake” namespaces in Swift by declaring empty structs with private
initializers. Since modules are constructed based on compiler arguments, not by
syntactic constructs, and because there is no pure Swift way to define
submodules (even though Clang module maps support this), there is no
source-drive way to group generated code into namespaces aside from this
approach.

Types can be added to those intermediate package structs using Swift extensions.
For example, a message `Baz` in package `foo.bar` could be represented in Swift
as follows:

```swift
public struct Foo {
  private init() {}
}

public extension Foo {
  public struct Bar {
    private init() {}
  }
}

public extension Foo.Bar {
  public struct Baz {
    // Message fields and other methods
  }
}

let baz = Foo.Bar.Baz()
```

Each of these constructs would actually be defined in a separate file; Swift
lets us keep them separate and add multiple structs to a single “namespace”
through extensions.

Unfortunately, these intermediate structs generate symbols of their own
(metatype information in the data segment). This becomes problematic if multiple
build targets contain Swift sources generated from different messages in the
same package. At link time, these symbols would collide, resulting in multiple
definition errors.

This approach also has the disadvantage that there is no automatic “short” way
to refer to the generated messages at the deepest nesting levels; since this use
of structs is a hack around the lack of namespaces, there is no equivalent to
import (Java) or using (C++) to simplify this. Users would have to declare type
aliases to make this cleaner, or we would have to generate them for users.
