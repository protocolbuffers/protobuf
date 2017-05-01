module google.protobuf.encoding;

import std.algorithm : map;
import std.range : chain, ElementType, empty;
import std.traits : isAggregateType, isArray, isAssociativeArray, isBoolean, isFloatingPoint, isIntegral, ValueType;
import google.protobuf.common;
import google.protobuf.internal;

auto toProtobuf(T)(T value)
if (isBoolean!T)
{
    return value ? [cast(ubyte) 0x01] : [cast(ubyte) 0x00];
}

unittest
{
    import std.array : array;

    assert(true.toProtobuf.array == [0x01]);
    assert(false.toProtobuf.array == [0x00]);
}

auto toProtobuf(string wire = "", T)(T value)
if (isIntegral!T)
{
    static if (wire == "fixed")
    {
        import std.bitmanip : nativeToLittleEndian;

        return nativeToLittleEndian(value).dup;
    }
    else static if (wire == "zigzag")
    {
        return Varint(zigZag(value));
    }
    else
    {
        return Varint(value);
    }
}

unittest
{
    import std.array : array;

    assert(10.toProtobuf.array == [0x0a]);
    assert((-1).toProtobuf.array == [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01]);
    assert((-1L).toProtobuf.array == [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01]);
    assert(0xffffffffffffffffUL.toProtobuf.array == [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01]);

    assert(1L.toProtobuf!"fixed".array == [0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
    assert(1.toProtobuf!"fixed".array == [0x01, 0x00, 0x00, 0x00]);
    assert((-1).toProtobuf!"fixed".array == [0xff, 0xff, 0xff, 0xff]);
    assert(0xffffffffU.toProtobuf!"fixed".array == [0xff, 0xff, 0xff, 0xff]);

    assert(1.toProtobuf!"zigzag".array == [0x02]);
    assert((-1).toProtobuf!"zigzag".array == [0x01]);
    assert(1L.toProtobuf!"zigzag".array == [0x02]);
    assert((-1L).toProtobuf!"zigzag".array == [0x01]);
}

auto toProtobuf(T)(T value)
if (isFloatingPoint!T)
{
    import std.bitmanip : nativeToLittleEndian;

    return nativeToLittleEndian(value).dup;
}

unittest
{
    import std.array : array;

    assert((0.0).toProtobuf.array == [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
    assert((0.0f).toProtobuf.array == [0x00, 0x00, 0x00, 0x00]);
}

auto toProtobuf(T)(T value)
if (is(T == string) || is(T == bytes))
{
    return chain(Varint(value.length), cast(ubyte[]) value);
}

unittest
{
    import std.array : array;

    assert("abc".toProtobuf.array == [0x03, 'a', 'b', 'c']);
    assert("".toProtobuf.array == [0x00]);
    assert((cast(bytes) [1, 2, 3]).toProtobuf.array == [0x03, 1, 2, 3]);
    assert((cast(bytes) []).toProtobuf.array == [0x00]);
}

auto toProtobuf(string wire = "", T)(T value)
if (isArray!T && !is(T == string) && !is(T == bytes))
{
    import std.range : hasLength;

    static assert(hasLength!T, "Cannot encode array with unknown length");

    static if (wire.empty)
    {
        auto result = value.map!(a => a.toProtobuf).joiner;
    }
    else
    {
        static assert(isIntegral!(ElementType!T), "Cannot specify wire format for non-integral arrays");

        auto result = value.map!(a => a.toProtobuf!wire).joiner;
    }

    return chain(Varint(result.length), result);
}

unittest
{
    import std.array : array;

    assert([false, false, true].toProtobuf.array == [0x03, 0x00, 0x00, 0x01]);
    assert([1, 2].toProtobuf!"fixed".array == [0x08, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00]);
    assert([1, 2].toProtobuf.array == [0x02, 0x01, 0x02]);
}

auto toProtobuf(T)(T value)
if (isAggregateType!T)
{
    import std.traits : hasMember;

    static if (hasMember!(T, "toProtobuf"))
    {
        return value.toProtobuf;
    }
    else
    {
        import std.algorithm : stdJoiner = joiner;
        import std.array : array;

        enum fieldExpressions = [Message!T.fieldNames]
            .map!(a => "value.toProtobufByField!(T." ~ a ~ ")")
            .stdJoiner(", ")
            .array;
        enum resultExpression = "chain(" ~ fieldExpressions ~ ")";

        static if (isRecursive!T)
        {
            static if (is(T == class))
            {
                if (value is null)
                    return cast(SizedRange!ubyte) sizedRangeObject(cast(ubyte[]) null);
            }

            return cast(SizedRange!ubyte) mixin(resultExpression).sizedRangeObject;
        }
        else
        {
            static if (is(T == class))
            {
                if (value is null)
                    return typeof(mixin(resultExpression)).init;
            }

            return mixin(resultExpression);
        }
    }
}

unittest
{
    import std.array : array;

    class Foo
    {
        @Proto(1) int bar = defaultValue!int;
        @Proto(3) bool qux = defaultValue!bool;
        @Proto(2, "fixed") long baz = defaultValue!long;
        @Proto(4) string quux = defaultValue!string;

        @Proto(5) Foo recursion = defaultValue!Foo;
    }

    Foo foo;
    assert(foo.toProtobuf.empty);
    foo = new Foo;
    assert(foo.toProtobuf.empty);
    foo.bar = 5;
    foo.baz = 1;
    foo.qux = true;
    assert(foo.toProtobuf.array == [0x08, 0x05, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x01]);
}

unittest
{
    struct Foo
    {
        @Proto(1) int bar = defaultValue!int;

        auto toProtobuf()
        {
            return [0x08, 0x00];
        }
    }

    Foo foo;
    assert(foo.toProtobuf == [0x08, 0x00]);
}

private static auto toProtobufByField(alias field, T)(T message)
{
    import std.meta : Alias;

    static assert(is(Alias!(__traits(parent, field)) == T));
    static assert(validateProto!(protoByField!field, typeof(field)));
    enum proto = protoByField!field;
    enum fieldName = __traits(identifier, field);
    enum fieldInstanceName = "message." ~ fieldName;

    static if (isOneof!field)
    {
        auto oneofCase = __traits(getMember, message, oneofCaseFieldName!field);
        enum fieldCase = "T." ~ typeof(oneofCase).stringof ~ "." ~ oneofAccessorName!field;

        if (oneofCase != mixin(fieldCase))
            return emptyRange!(typeof(mixin(fieldInstanceName).toProtobufByProto!proto));
    }
    else
    {
        if (mixin(fieldInstanceName) == defaultValue!(typeof(field)))
            return emptyRange!(typeof(mixin(fieldInstanceName).toProtobufByProto!proto));
    }

    return mixin(fieldInstanceName).toProtobufByProto!proto;
}

unittest
{
    import std.array : array;
    import std.typecons : Yes;

    struct Foo
    {
        @Proto(1) int f10 = 10;
        @Proto(16) int f11 = 10;
        @Proto(2048) bool f12 = true;
        @Proto(262144) bool f13 = true;

        @Proto(20) bool f20 = false;
        @Proto(21) int f21 = 0;
        @Proto(22, "fixed") int f22 = 0;
        @Proto(23, "zigzag") int f23 = 0;
        @Proto(24) long f24 = 0L;
        @Proto(25) double f25 = 0.0;
        @Proto(26) string f26 = "";
        @Proto(27) bytes f27 = [];

        @Proto(30) int[] f30 = [1, 2];
        @Proto(31, "", Yes.packed) int[] f31 = [1, 2];
        @Proto(32, "", Yes.packed) int[] f32 = [128, 2];
    }

    Foo foo;

    assert(foo.toProtobufByField!(Foo.f10).array == [0x08, 0x0a]);
    assert(foo.toProtobufByField!(Foo.f11).array == [0x80, 0x01, 0x0a]);
    assert(foo.toProtobufByField!(Foo.f12).array == [0x80, 0x80, 0x01, 0x01]);
    assert(foo.toProtobufByField!(Foo.f13).array == [0x80, 0x80, 0x80, 0x01, 0x01]);

    assert(foo.toProtobufByField!(Foo.f20).empty);
    assert(foo.toProtobufByField!(Foo.f21).empty);
    assert(foo.toProtobufByField!(Foo.f22).empty);
    assert(foo.toProtobufByField!(Foo.f23).empty);
    assert(foo.toProtobufByField!(Foo.f24).empty);
    assert(foo.toProtobufByField!(Foo.f25).empty);
    assert(foo.toProtobufByField!(Foo.f26).empty);
    assert(foo.toProtobufByField!(Foo.f27).empty);

    assert(foo.toProtobufByField!(Foo.f30).array == [0xf0, 0x01, 0x01, 0xf0, 0x01, 0x02]);
    assert(foo.toProtobufByField!(Foo.f31).array == [0xfa, 0x01, 0x02, 0x01, 0x02]);
    assert(foo.toProtobufByField!(Foo.f32).array == [0x82, 0x02, 0x03, 0x80, 0x01, 0x02]);
}

private auto toProtobufByProto(Proto proto, T)(T value)
if (isBoolean!T ||
    isFloatingPoint!T ||
    is(T == string) ||
    is(T == bytes) ||
    (isArray!T && proto.packed))
{
    static assert(validateProto!(proto, T));

    return chain(encodeTag!(proto, T), value.toProtobuf);
}

private auto toProtobufByProto(Proto proto, T)(T value)
if (isIntegral!T)
{
    static assert(validateProto!(proto, T));

    enum wire = proto.wire;
    return chain(encodeTag!(proto, T), value.toProtobuf!wire);
}

private auto toProtobufByProto(Proto proto, T)(T value)
if (isArray!T && !proto.packed && !is(T == string) && !is(T == bytes))
{
    static assert(validateProto!(proto, T));

    enum elementProto = Proto(proto.tag, proto.wire);
    return value
        .map!(a => a.toProtobufByProto!elementProto)
        .joiner;
}

private auto toProtobufByProto(Proto proto, T)(T value)
if (isAssociativeArray!T)
{
    import std.algorithm : findSplit;

    static assert(validateProto!(proto, T));

    enum wires = proto.wire.findSplit(",");
    enum keyProto = Proto(1, wires[0]);
    enum valueProto = Proto(2, wires[2]);

    return value
        .byKeyValue
        .map!(a => chain(a.key.toProtobufByProto!keyProto, a.value.toProtobufByProto!valueProto))
        .map!(a => chain(encodeTag!(proto, T), Varint(a.length), a))
        .joiner;
}

unittest
{
    import std.array : array;
    import std.typecons : Yes;

    assert([1, 2].toProtobufByProto!(Proto(1)).array == [0x08, 0x01, 0x08, 0x02]);
    assert((int[]).init.toProtobufByProto!(Proto(1)).empty);
    assert([1, 2].toProtobufByProto!(Proto(1, "", Yes.packed)).array == [0x0a, 0x02, 0x01, 0x02]);
    assert([128, 2].toProtobufByProto!(Proto(1, "", Yes.packed)).array == [0x0a, 0x03, 0x80, 0x01, 0x02]);
    assert((int[]).init.toProtobufByProto!(Proto(1, "", Yes.packed)).array == [0x0a, 0x00]);

    assert((int[int]).init.toProtobufByProto!(Proto(1)).empty);
    assert([1: 2].toProtobufByProto!(Proto(1)).array == [0x0a, 0x04, 0x08, 0x01, 0x10, 0x02]);
    assert([1: 2].toProtobufByProto!(Proto(1, ",fixed")).array ==
        [0x0a, 0x07, 0x08, 0x01, 0x15, 0x02, 0x00, 0x00, 0x00]);
}

private auto toProtobufByProto(Proto proto, T)(T value)
if (isAggregateType!T)
{
    static assert(validateProto!(proto, T));

    auto encodedValue = value.toProtobuf;
    auto result = chain(encodeTag!(proto, T), Varint(encodedValue.length), encodedValue);

    static if (isRecursive!T)
    {
        return cast(SizedRange!ubyte) result.sizedRangeObject;
    }
    else
    {
        return result;
    }
}
