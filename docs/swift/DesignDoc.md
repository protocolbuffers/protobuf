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

### Collisions with fundamental types

The Swift standard library also has a large number of types that are present in
the global namespace (due to an implicit import of the `Swift` module). Some
are very frequently used types (`Int`, `String`, `Array`, `Dictionary`), and
there are many less frequently used ones as well. (Rather than list them
exhaustively here, we direct readers to [swiftdoc.org](http://swiftdoc.org/)
for a full list across Swift versions.) Any of these could be the source of a
collision where a message or enum with the same name could cause confusion, or
potentially override the Swift symbol in unexpected ways.

For types nested inside packages, the message/enum name itself does not pose an
issue, because it will always be prefixed by the package path
(`Some_Package_Int`) or a custom prefix (`SPInt`). However, there is still the
potential for collisions among nested messages/enums, messages/enums that are
not in a package, or the very unfortunate case where a package-prefixed name
might still collide with a Swift type.

In these scenarios, we will append `Message` to message types and `Enum` to
enum types if the package-prefixed name would collide with an existing Swift
type. For example, the following proto:

```protobuf
message Foo {
  message CollectionType {
    repeated string values = 1;
  }
}
```

would translate to the following Swift code:

```swift
struct Foo: ProtoMessage {
  struct CollectionTypeMessage: ProtoMessage {
    var valuesArray: [String]
  }
}
```

Notice that the `CollectionType` message has had its name changed to avoid a
collision with Swift's `CollectionType` protocol.

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
level option, `swift_proto_package_prefix`, that can be added to `.proto` files.
When present, this will cause the package portion of the top-level type names
to be replaced with the provided string. For example, the following `.proto`
file:

```protobuf
option swift_proto_package_prefix = "FBP";
package foo.bar;

message Baz {
  // Message fields
}
```

would generate the following Swift source:

```swift
public struct FBPBaz {
  // Message fields and other methods
}
```

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

The user-facing API for a `ProtoMessage` will be as follows:

```swift
public protocol ProtoMessage {

  var serializedSize: Int { get }

  init()
  init(data: NSData) throws

  mutating func merge(fromData data: NSData) throws

  func data() throws -> NSData
  func write(toOutputStream stream: NSOutputStream) throws

  // Public only as an implementation detail:

  mutating func merge(fromCodedInputStream stream: CodedInputStream) throws
  func write(toCodedOutputStream stream: CodedOutputStream) throws
}
```

The methods listed at the end are public only as an implementation detail,
because the generated code will be implemented in terms of them to read and
write the proto wire format. Users should never need to call them directly or
use the coded stream types. The remaining methods and initializers are
implemented in a protocol extension in terms of those coded stream methods.

Conformance to `Equatable`, `Hashable`, and `CustomDebugStringConvertible` are
also desirable but not currently shown, to focus on a small API intended for
mobile clients. The implementations and generation of those methods are fairly
straightforward, but it should be noted that generating equality and hashability
for very large messages that are not likely to use those features would add
significant bloat to an application. (Mirrors could be investigated as a way of
reducing this duplication using a visitor pattern, albeit with some performance
trade-offs.)

### Fields

Fields in a message will be represented as non-optional properties. We explored
using optionals (regular and implicitly unwrapped), but determined that they
were unsuitable; protocol buffer users expect to receive default values, not
`nil`, when retrieving fields, and this is more important in proto2 where fields
can have explicit non-zero defaults. Implicitly unwrapped computed properties
could mitigate the `clear` situation by treating assignment of `nil` as clearing
the field, but we would still need to create a `has` method because the property
getter would never return nil (it would always return the default value, and
there would be no way to distinguish "not set" from "explicitly set to the
default value"). In that sense, having the property be optional seems misleading
to users.

In proto2, each field will have associated `has` and `clear` methods that let
clients test whether a field has been explicitly set and to clear its value,
respectively.

In proto3, we can skip generation of `has` and `clear` methods, with the
expectation that users will use the zero/empty value of the appropriate type to
test for presence or clear the field. The exception to this is message fields,
where we should still provide those methods since comparing against the empty
message could be costly for large types. (One caveat: we must preserve the last
set case of a oneof field even if it is set to the zero/empty value.)

It is our hope that a new form of resettable properties will be added to Swift
that lets us encode the `has` and `clear` concepts directly in the property
instead of creating parallel methods. Some community members have already
drafted or sent proposals for review that would benefit our designs:

*   [SE-0030: Property Behaviors]
    (https://github.com/apple/swift-evolution/blob/master/proposals/0030-property-behavior-decls.md)
*   [Drafted: Resettable Properties]
    (https://github.com/patters/swift-evolution/blob/master/proposals/0000-resettable-properties.md)

Lastly, we note some name transformations that we would apply to fields:

* Repeated fields will have `Array` appended to their names. This is because
  there is a trend in some messages to name repeated fields with the singular
  version of the name (`descriptor.proto` in particular does this), and
  `foo.name.append(bar)` looks potentially like appending a string to another
  string whereas `foo.nameArray.append(bar)` is much clearer.
* If a field name would collide with one of the properties or methods in
  `ProtoMessage`'s public interface, we append `Field` to its name.

### Backing storage

Data for fields in a message may be stored in various ways to maximize
performance. In particular, we note the following:

* Message fields must be hoisted into a nested storage `class` inside the
  message because structs in Swift cannot be directly or indirectly recursive.
  Computed propeties on the struct will provide getters and setters that make
  this transparent.

* Repeated fields and maps can be stored properties on the message struct
  itself. They already implement copy-on-write, and letting users touch them
  without indirection avoids creating unnecessary copies that could lead to
  poor performance.

* Primitive fields can either be stored properties on the message struct itself
  or hoisted into the storage class to provide copy-on-write functionality in
  the same style as Swift's collection types. If hoisted, computer properties
  will again be used to make this transparent.

To the last point, we can tune messages heuristically with different storage
models as long as we keep the public interface the same. For example, a small
message with only a few primitive fields might be most efficient represented
as a simple struct with stored properties. On the other hand, a message with a
very large number of primitive fields might benefit from copy-on-write
behavior.

We do not currently make any judgments about when to apply these heuristics;
rather, we simply note that the decisions can be made and will evaluate
benchmarks in the future to guide those decisions.

We examined other approaches intended to reduce code size (such as using a
dictionary to store the fields, keyed by number), but the performance in early
experimentation was found to significantly overshadow the code size benefits,
especially when working with repeated fields (because appending a value to an
array would involve retrieving it from the dictionary but leaving the original
there, appending a value to the retrieved one, which created a copy because the
storage was no longer uniquely referenced, then setting the new array back into
the dictionary, discarding the original).

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
  public var bar: Bar {
    get { ... }
    set { ... }
  }
}
```

If the user explicitly clears `bar`, or if it was never set when read from
the wire, retrieving the value of `bar` would return the default, statically
allocated instance of `Bar` containing default values for its fields. This
achieves the desired behavior for reading default values in the same way that
scalar fields are designed, and also allows users to deep-drill into complex
object graphs to get or set fields without checking for nil at each step.

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
public enum ContentType: Int32 {
  case text = 0
  case image = 1
}
```

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
public enum ContentType: RawRepresentable {
  case text
  case image
  case UNKNOWN_VALUE(Int32)

  public typealias RawValue = Int32

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
public enum Foo: Int32 {
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
  public enum Record {
    case name(String)
    case idNumber(Int32)
    case NOT_SET
  }

  // This is the "Swifty" way of accessing the value
  public var record_oneofCase: Record { ... }

  // Direct access to the underlying fields
  public var name: String! { ... }
  public var idNumber: Int32! { ... }
}
```

This makes both usage patterns possible:

```swift
// Usage 1: Case-based dispatch
switch message.record_oneofCase {
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
