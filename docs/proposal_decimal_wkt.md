# Protobuf "decimal" well known type


Created: | 2019-11-14

State:   | PROPOSED

Author:  | jtattermusch@


## Summary

Proposes a new "decimal" well known type as a standard representation for
accurate fractional (e.g. monetary) values and provide language bindings for
languages that have built-in language support for such a type.

## Motivation

"decimal" type is important for many applications (especially financial
applications) that need to express fractional values (such as monetary amounts)
accurately. While many programming languages offer a built-in support for
"decimal" type, protobuf currently doesn't offer an easy way to utilize those
built-in types and users are forced to resort to workarounds and to reinvent the
wheel.

Better support for decimal type has been requested at
https://github.com/protocolbuffers/protobuf/issues/4406 and is heavily upvoted
(but there isn't much progress on the issue).

## Detailed Explanation

### Requirements for the new decimal well known type

-   introduce the "decimal" type as a "well-known type" (as opposed to extending
    the protobuf wire protocol, which would be way too complicated). Protobuf
    library implementations in different languages will then provide
    language-specific bindings and helpers to integrate with the native
    "decimal" type supported by given language.
-   Provide message representation that is reasonably usable if the well known
    type doesn't have special handling in given language (= not all languages
    will have a special binding for the new well known type and the type must
    still be usable in its "raw form" as a protobuf message).
-   Match semantics of decimal type in popular programming languages (if
    possible, in different languages the built-in decimal type can have slightly
    different properties).
-   Stay consistent with other pre-existing commonly used .proto types so that
    the new WKT fits well into the existing ecosystem (e.g. there's a
    money.proto)

### Rollout plan

-   Decide which .proto definition of the "decimal" type approach to use (see
    below)
-   Check in the well known type .proto definition of the type
-   Language owners can provide their own bindings for the WKT
-   Because the "decimal" WKT will be designed to be usable even without special
    language bindings, initially the language-specific bindings for the WKT can
    be marked as "experimental" in each of the languages and they can be later
    stabilized once they prove themselves.

### Proposal: message with "units" and "nanos"

```
// file:fixed_decimal.proto
package google.protobuf;


message FixedDecimal {

    // Whole units part of the amount
    int64 units = 1;

    // Nano units of the amount (10^-9)
    // Must be same sign as units
    // Example: The value -1.25 is represented as units=-1 and nanos=-250000000
    sfixed32 nanos = 2;
}
```

Pros

-   simple concept, easy to understand for users
-   values are human readable and easy to understand without specialized support
-   Very similar to .proto definitions that have already been used internally at
    google
-   Consistent with other widely used .proto definitions (such as money.proto)
-   range of values can be safely expressed by "decimal" type in all languages
    that support such type.
-   representable value range should be good enough to vast majority of
    applications (e.g. for financial applications)
-   Currently this is recommended for C# users by Microsoft (= which gives some
    indication that this approach works well in the .NET ecosystem).
    https://docs.microsoft.com/en-us/dotnet/architecture/grpc-for-wcf-developers/protobuf-data-types#decimals

Cons

-   lower range and resolution than "decimal" types in all languages (the good
    news is all common decimal types can represent the values carried by the
    proto, but converting from lang-specific decimal values can lose precision /
    throw exception)

The type of "units" can be either `int64` or `sint64`. `sint64` is more
efficient if negative values are common, but
`money.proto`
uses `int64`. So the choice is either staying more consistent with `money.proto`
or aiming for the highest efficiency.

The type of "nanos" can be either `int32`, `sfixed32` or `sint32`. `int32` is
what `money.proto` uses, but the signed types will be more efficient for
negative values. One advantage of `sfixed32` over `sint32` is that the "nanos"
value is likely to be a large number (numerical values like 0.5, 0.25 etc. are
more common than e.g. 0.000000001), and so `sfixed32` has potential to be more
efficient that `sint32` or `int32` (we'd need to do some measurements).

The sign of "units" and "nanos" is required to be the same (money.proto uses the
same restriction). Nevertheless, there are other types (e.g. timestamp.proto),
where the "nanos" part is always positive. The former seems to make more sense
for representing decimal numbers, because e.g. -7.5 will be represented as
`units=-7` and `nanos=-500000000` and the represented value can always be
computed as `units + 0.000000001 * nanos` regardless of the sign.

TODO(jtattermusch): do the signs of units vs nanos influence conversion speed
between the message fields and the numeric representation? it might for decimal
-> FixedDecimal direction of conversion

The message name `FixedDecimal`(in `fixed_decimal.proto`) is chosen to make the
nature of the WKT explicit (and to express that this is not necessarily a 1:1 of
language's "decimal" type).

### API Changes

For languages that provide a built-in "decimal" type (see overview in Appendix),
language owners should add an API that allows easy conversion between
`FixedDecimal` WKT and the built-in "decimal" type in given language.

The API changes in each language should be purely additive. It's fine for
language implementations to provide the bindings independent of other languages
(e.g. C# might provide the bindings sooner than some other language).

The value range representable by the proposed FixedDecimal message (at most ~29
significant digits) is smaller than the representable range for all the
language-specific "decimal" types. Therefore, there should be no issue
converting FixedDecimal message to any of the decimal types. In the opposite
direction (setting a value from a "decimal" type into a FixedDecimal), there can
be an "out of range" error and language implentations should be consistent in
how this situation is handled.

Language specific-APIs should generally allow accessing the FixedDecimal values
in two modes: 1. access the raw FixedDecimal value (the "units" and "nanos"
fields) 2. access the language-specific value as a "decimal" type

#### C# bindings

```
// extra methods for FixedDecimal generated class
public partial class FixedDecimal
{
    private const decimal NanoFactor = 1_000_000_000;

    public decimal ToDecimal()
    {
        // IsNormalized checks for Units and Nanos having the same sign
        // and Nanos being from the right range.
        if (!IsNormalized(Units, Nanos))
        {
            throw new InvalidOperationException(@"Fixed decimal contains invalid values: Units={Units}; Nanos={Nanos}");
        }
        return Units + Nanos / NanoFactor;
    }

    public static FixedDecimal FromDecimal(decimal value)
    {
        // ToInt64() throws OverflowException if value is out of range of int64
        var units = decimal.ToInt64(value);
        var nanos = decimal.ToInt32((value - units) * NanoFactor);
        return new FixedDecimal { Units = units, Nanos = nanos };
    }
}

// TODO: we can also add extension methods for "decimal" type to convert into FixedDecimal values
// by invoking value.ToFixedDecimal();

// TODO: in C# we can also define implicit conversion operators between
// FixedDecimal and decimal, but other WKT binding in protobuf C# don't do that
```

inspired by
https://docs.microsoft.com/en-us/dotnet/architecture/grpc-for-wcf-developers/protobuf-data-types#creating-a-custom-decimal-type-for-protobuf
and the way bindings for other WKTs are currently implemented in protobuf C#.

#### Python bindings

TODO: add design

#### Java bindings

TODO: add design for conversion between FixedDecimal message and BigDecimal type

#### C++ bindings

No special bindings: C++ doesn't have a standard way to represent the "decimal"
type, and until such type exists, users can just access the raw message (the
"units" and "nanos" fields directly).

#### Go bindings

TODO: add design

#### Ruby bindings

TODO: add design

#### Obj-C bindings

TODO: add design

### JSON Representation of FixedDecimal

Many WKT types provide a custom JSON serialization format (e.g.
timestamp.proto).

One way to represent the decimal number in JSON is with a string (e.g.
`"123.25"`) that complies with the Java's
[decimal floating point literal](https://docs.oracle.com/javase/specs/jls/se8/html/jls-3.html#jls-DecimalFloatingPointLiteral).

On the other hand, just using the default JSON format (2 fields "units" and
"nanos") doesn't require a special JSON handling to be implemented in all
languages and is more consistent with TextFormat and the representation of the
message itself. 2 fields "units" and "nanos" seem to be human readable enough
for this to be an option.

### Open-Source Plan

Better support for decimal type has been requested externally at
https://github.com/protocolbuffers/protobuf/issues/4406 and is heavily upvoted
(but there isn't much progress on the issue).

This change should definitely be released to open-source.

## Drawbacks

Not much beyond the risk of coming with bad design that provided suboptimal
developer experience.

## Alternatives Considered

Several different .proto representations are proposed in this doc. Approach 1
seems to have the best pros/cons.

### Alternative 1: Use integer value and "scale/exponent"

```
message Decimal {
 int64 value = 1;
 int32 scale = 2;
}
```

-   slightly less human-readable than the approach proposed.
-   no protection against using too high/too low exponents which can lead to
    losing significant digits if not used carefully.

### Alternative 2: Low level representation

```
message Decimal {
    // 96-bit mantissa broken into two chunks
    // this representation matches exactly the C# decimal spec
    // but not such a good match for other languages
    uint64 mantissa_msb = 1;
    uint32 mantissa_lsb = 2;
    required sint32 exponent_and_sign = 3;
}
```

Pros

-   efficient on the wire
-   1:1 mapping between the protobuf message and the language type (for some
    languages)

Cons

-   matches spec of some implementations of "decimal" exactly, but doesn't match
    analogous type in other languages
-   too low level to be used as a raw message (=problem for cross-language
    interoperability)
-   not intelligible by humans unless good language bindings are provided

### Alternative 3: String representation

```
message DecimalValue {

 /* This string contains a decimal floating point literal in the format
  * defined by the Java Language Specification: https://docs.oracle.com/javase/specs/jls/se8/html/jls-3.html#jls-DecimalFloatingPointLiteral
  */

 string value = 1;

}
```

-   can represent numbers with arbitrary precision (some languages support that)
-   high processing overhead due to string <-> number conversions
-   bad efficiency on the wire

## Unresolved Questions

-   The request for this feature has been initiated by external users and it
    would be good to somehow enable an open review process (external
    contributors can bring useful insights and help prototyping the solution)

## Appendix 1: Overview of built-in "decimal" type in programming languages

The key features of "decimal" type are that its internal representation must be
base-10 (as opposed to base-2 used for floating point types) and that it
provides some level of rounding protection (in the sense that it forbids values
that would require storing more significant digits than what the type itself is
capable of - this is what can lead to imprecise representation of floating point
values).

C#: decimal 128bit in total, 96-bit bit mantissa, exponent -28..0 (28-29
significant digits).

Python: decimal 28 digits precision by default (can be adjusted)

Java: BigDecimal uses arbitrary precision, the type is slightly heavy-weight

Go: no officially recommended type, but there are community provided libraries
with decimal support

Ruby: BigDecimal uses arbitrary precision

ObjC: NSDecimal stores up to 38digits, exponent -128 - 127

Swift: Decimal essentially a value-type wrapper around NSDecimal, so same
constraints/bugs as ObjC

https://en.wikipedia.org/wiki/Decimal_data_type

## Appendix 2: Overview of "decimal" type in common databases

BigQuery NUMERIC type: "exact numeric value with 38 digits of precision and 9
decimal digits of scale"
https://cloud.google.com/bigquery/docs/reference/standard-sql/data-types#numeric-type

TODO(jtattermusch)
