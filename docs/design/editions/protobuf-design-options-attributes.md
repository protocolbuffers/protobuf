# Protobuf Design: Options Attributes

A proposal to create target and retention attributes to support.

**Author:** [@kfm](https://github.com/fowles)

**Approved:** 2022-08-26

## Background

The [Protobuf Editions](what-are-protobuf-editions.md) project plans to use
[custom options](protobuf-editions-design-features.md) to model features and
encourage language bindings to build custom features off options as well.

This design proposed the specific addition of `target` and `retention`
attributes for options as well as their suggested meaning.

Both `target` and `retention` attributes are no-ops when applied to fields that
are not options (either from descriptor.proto or custom options).

## Target Attributes

Historically, options have only applied to specific entities, but features will
be available on most entities. To allow language specific extensions to restrict
the places where options can bind, we will allow features to explicitly specify
the targets they apply to (similar in concept to the "target" attribute on Java
annotations). `TARGET_TYPE_UNKNOWN` will be treated as absent.

```
message FieldOptions {
  ...
  optional OptionTargetType target = 17;

  enum OptionTargetType {
    TARGET_TYPE_UNKNOWN = 0;
    TARGET_TYPE_FILE = 1;
    TARGET_TYPE_EXTENSION_RANGE = 2;
    TARGET_TYPE_MESSAGE = 3;
    TARGET_TYPE_FIELD = 4;
    TARGET_TYPE_ONEOF = 5;
    TARGET_TYPE_ENUM = 6;
    TARGET_TYPE_ENUM_VALUE = 7;
    TARGET_TYPE_SERVICE = 8;
    TARGET_TYPE_METHOD = 9;
  };
}
```

If no target is provided, `protoc` will permit the target to apply to any
entity. Otherwise, `protoc` will allow an option to be applied at either the
file level or to its target entity (and will produce a compile error for any
other placement). For example

```
message Features {
  ...

  enum EnumType {
    OPEN = 0;
    CLOSED = 1;
  }
  optional EnumType enum = 2 [
      target = TARGET_TYPE_ENUM
  ];
}
```

would allow usage of

```
// foo.proto
edition = "tbd"

option features.enum = OPEN;  // allowed at FILE scope

enum Foo {
  option features.enum = CLOSED;  // allowed at ENUM scope
  A = 2;
  B = 4;
}

message Bar {
  option features.enum = CLOSED;  // disallowed at Message scope

  enum Baz {
    C = 8;
  }
}
```

## Retention

To reduce the size of descriptors in protobuf runtimes, features will be
permitted to specify retention rules (again similar in concept to "retention"
attributes on Java annotations).

```
enum FeatureRetention {
  RETENTION_UNKNOWN = 0;
  RETENTION_RUNTIME = 1;
  RETENTION_SOURCE = 2;
}
```

Options intended to inform code generators or `protoc` itself can be annotated
with `SOURCE` retention. The default retention will be `RUNTIME` as that is the
current behavior for all options. **Code generators that emit generated
descriptors will be required to omit/strip options with `SOURCE` retention from
their generated descriptors.** For example:

```
message Cpp {
  enum StringType {
    STRING = 1;
    STRING_VIEW = 0;
    CORD = 2;
  }

  optional string namespace = 2 [
      retention = RETENTION_SOURCE,
      target = TARGET_TYPE_FILE
  ];
}
```

## Motivation

While the proximal motivation for these options is for use with "features" in
"editions", I believe they provide sufficient general utility that adding them
directly to `FieldDescriptorOptions` is warranted. For example, significant
savings in binary sizes could be realized if `ExtensionRangeOptions::Metadata`
had only `SOURCE` retention. Previously, we have specifically special-cased this
behavior on a per-field basis, which does work but does not provide good
extensibility.

## Discussion

In the initial design `target` was serving the dual purpose of identifying the
semantic entity, and also the granularity of inheritance for features. After
discussion about concerns around over use of inheritance, we decided for a
slightly refined definition that decouples these concerns. `target` **only**
specifies the semantic entity to which an option can apply. Features will be
able to be set on both the `FILE` level and their semantic entity. Everything in
between will be refused in the initial release. This allows us a clean
forward-compatible way to allow arbitrary feature inheritance, but doesn't
commit us to doing that until we need it.

Similarly, we will start with `optional` target, because we can safely move to
`repeated` later should the need arise.

The naming for `target` and `retention` are directly modeled after Java
annotations. Other names were considered, but no better name was found and the
similarity to an existing thing won the day.

## Alternatives

### Use a repeated `target` proposed

This is the proposed alternative.

#### Pros

*   Allows fine-grained control of target applicability.

#### Cons

*   Harder to generalize for users (every feature's specification is potentially
    unique).

### Allow hierarchy based on `target` semantic location.

Rather than having a repeated `target` that specifies all locations, we allow
only the level at which it semantically applies to be specified. The protoc
compiler will implicitly allow the field to be used on entities that can
lexically group that type of entry. For this `target` can be either singular or
repeated.

#### Pros

*   Enables tooling that understands when a feature is used for grouping vs when
    it has semantic value (helpful for minimizing churn in large-scale changes).
*   Easier to generalize for users (any `FIELD` feature can apply to a message
    as opposed to only the `FIELD` features that explicitly specified an
    additional `target`).

#### Cons

*   Forces all `target` applications to be permitted on scoping entities.

### Use Custom Options (aka "We Must Go Deeper")

Rather than building `retention` and `target` directly as fields of
`FieldOptions`, we could use custom options to define an equivalent thing. This
option was rejected because it pushes extra syntax onto users for a fundamental
feature.

#### Pros

*   Doesn't require modifying `descriptor.proto`.

#### Cons

*   Requires a less-intuitive spelling in user code.
*   Requires an additional import for users.
*   Language-level features would have to have a magic syntax or a side table
    instead of using the same consistent option as user per code gen features.

### Hard Code Behaviors in `protoc`

Rather than building a generic mechanism we could simply hard code the behavior
of protoc and document it.

#### Pros

*   Can't be misused.

#### Cons

*   Not extensible for users.
*   Requires more special cases users need to learn.

### Original Approved Proposal

The proposal as originally approved had some slight differences from what was
ultimately implemented:

*   The retention enum did not have an `UNKNOWN` type.
*   The enums were defined at the top level instead of nested inside
    `FieldOptions`.
*   The enum values did not have a scoping prefix.
*   The target enum had a `STREAM` entry, but this turned out to be unnecessary
    since the syntax that it applied to was removed.

### Do Nothing

We could omit this entirely and get ice cream instead. This was rejected because
the proliferation of features on entities they do not apply to is considered too
high a cost.

#### Pros

*   Ice cream is awesome.

#### Cons

*   Doesn't address any of the problems that caused this to come up.
*   Some people are lactose intolerant.
