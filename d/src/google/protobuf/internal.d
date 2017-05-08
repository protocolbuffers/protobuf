module google.protobuf.internal;

import std.range : ElementType, hasLength, InputRange, InputRangeObject, isInputRange;
import google.protobuf.common;

struct Varint
{
    private long value;
    private ubyte index;
    private ubyte _length;

    this(long value)
    out { assert(_length > 0); }
    body
    {
        size_t encodingLength(long value)
        {
            import core.bitop : bsr;

            if (value == 0)
                return 1;

            static if (long.sizeof <= size_t.sizeof)
            {
                return bsr(value) / 7 + 1;
            }
            else
            {
                if (value > 0 && value <= size_t.max)
                    return bsr(value) / 7 + 1;

                enum bsrShift = size_t.sizeof * 8;
                return (bsr(value >>> bsrShift) + bsrShift) / 7 + 1;
            }
        }

        this.value = value;
        this._length = cast(ubyte) encodingLength(value);
    }

    @property bool empty() const { return index >= _length; }
    @property ubyte front() const { return opIndex(index); }
    void popFront() { ++index; }

    ubyte opIndex(size_t index) const
    in { assert(index < _length); }
    body
    {
        auto result = value >>> (index * 7);

        if (result >>> 7)
            return result & 0x7F | 0x80;
        else
            return result & 0x7F;
    }

    @property size_t length() const
    in { assert(index <= _length); }
    body
    {
        return _length - index;
    }
}

unittest
{
    import std.array : array;

    assert(Varint(0).array == [0x00]);
    assert(Varint(1).array == [0x01]);
    assert(Varint(-1).array == [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01]);
    assert(Varint(-1L).array == [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01]);
    assert(Varint(int.max).array == [0xff, 0xff, 0xff, 0xff, 0x07]);
    assert(Varint(int.min).array == [0x80, 0x80, 0x80, 0x80, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x01]);
    assert(Varint(long.max).array == [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f]);
    assert(Varint(long.min).array == [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01]);
}

long fromVarint(R)(ref R inputRange)
if (isInputRange!R)
{
    import std.exception : enforce;
    import std.range : empty, front, popFront;
    import std.traits : Unqual, Unsigned;

    static assert(is(ElementType!R : ubyte));

    alias E = Unqual!(Unsigned!(ElementType!R));

    size_t i = 0;
    long result = 0;
    E data;

    do
    {
        enforce!ProtobufException(!inputRange.empty, "Truncated message");
        data = cast(E) inputRange.front;
        inputRange.popFront;

        if (i == 9)
            enforce!ProtobufException(!(data & 0xfe), "Malformed varint encoding");

        result |= cast(long) (data & 0x7f) << (i++ * 7);
    } while (data & 0x80);

    return result;
}

unittest
{
    auto foo = Varint(0);
    assert(fromVarint(foo) == 0);
    assert(foo.empty);

    foo = Varint(1);
    assert(fromVarint(foo) == 1);
    assert(foo.empty);

    foo = Varint(int.max);
    assert(fromVarint(foo) == int.max);
    assert(foo.empty);

    foo = Varint(int.min);
    assert(fromVarint(foo) == int.min);
    assert(foo.empty);

    foo = Varint(long.max);
    assert(fromVarint(foo) == long.max);
    assert(foo.empty);

    foo = Varint(long.min);
    assert(fromVarint(foo) == long.min);
    assert(foo.empty);
}

T zigZag(T)(T value)
if (is(T == int) || is(T == long))
{
    return (value << 1) ^ (value >> (T.sizeof * 8 - 1));
}

unittest
{
    assert(zigZag(0) == 0);
    assert(zigZag(-1) == 1);
    assert(zigZag(1L) == 2L);
    assert(zigZag(int.max) == 0xffff_fffe);
    assert(zigZag(int.min) == 0xffff_ffff);
    assert(zigZag(long.max) == 0xffff_ffff_ffff_fffe);
    assert(zigZag(long.min) == 0xffff_ffff_ffff_ffff);
}

T zagZig(T)(T value)
if (is(T == int) || is(T == long))
{
    return (value >>> 1) ^ -(value & 1);
}

unittest
{
    assert(zagZig(zigZag(0)) == 0);
    assert(zagZig(zigZag(-1)) == -1);
    assert(zagZig(zigZag(1L)) == 1L);
    assert(zagZig(zigZag(int.max)) == int.max);
    assert(zagZig(zigZag(int.min)) == int.min);
    assert(zagZig(zigZag(long.min)) == long.min);
}

auto encodeTag(Proto proto, T)()
{
    static assert(validateProto!(proto, T));

    return Varint(proto.tag << 3 | wireType!(proto, T));
}

auto decodeTag(R)(ref R inputRange)
if (isInputRange!R)
{
    import std.algorithm : canFind;
    import std.exception : enforce;
    import std.traits : EnumMembers;
    import std.typecons : tuple;

    static assert(is(ElementType!R : ubyte));

    long tagWire = fromVarint(inputRange);

    WireType wireType = cast(WireType) (tagWire & 0x07);
    enforce!ProtobufException([EnumMembers!WireType].canFind(wireType), "Unknown encoded wire format");
    tagWire >>>= 3;
    enforce!ProtobufException(tagWire > 0 && tagWire < (1<<29), "Tag value out of range");
    return tuple!("tag", "wireType")(cast(uint) tagWire, wireType);
}

enum WireType : ubyte
{
    varint = 0,
    bits64 = 1,
    withLength = 2,
    bits32 = 5,
}

WireType wireType(Proto proto, T)()
{
    import std.traits : isAggregateType, isArray, isAssociativeArray, isBoolean, isIntegral;

    static assert(validateProto!(proto, T));

    static if (is(T == string) || is(T == bytes) || (isArray!T && proto.packed) || isAssociativeArray!T ||
        isAggregateType!T)
    {
        return WireType.withLength;
    }
    else static if (isArray!T && !proto.packed)
    {
        return wireType!(proto, ElementType!T);
    }
    else static if (((is(T == long) || is(T == ulong)) && proto.wire == "fixed") || is(T == double))
    {
        return WireType.bits64;
    }
    else static if (((is(T == int) || is(T == uint)) && proto.wire == "fixed") || is(T == float))
    {
        return WireType.bits32;
    }
    else static if (isBoolean!T || isIntegral!T)
    {
        return WireType.varint;
    }
    else
    {
        static assert(0, "Internal error");
    }
}

template CollectTypes(M, T...)
{
    import std.meta : AliasSeq, Filter, NoDuplicates, staticIndexOf, staticMap, Unqual;
    import std.range : ElementType;
    import std.traits : getSymbolsByUDA, hasMember, isAggregateType, isArray, isAssociativeArray, KeyType, ValueType;

    static if (isAggregateType!M)
    {
        static template BaseTypeOf(alias S)
        {
            alias BaseTypeOf = BaseType!(typeof(S));
        }

        static template BaseType(T)
        {
            static if (isArray!T && !is(T == string) && !is(T == bytes))
            {
                alias BaseType = BaseType!(ElementType!T);
            }
            else static if (isAssociativeArray!T)
            {
                alias BaseType = AliasSeq!(BaseType!(KeyType!T), BaseType!(ValueType!T));
            }
            else
            {
                alias BaseType = Unqual!T;
            }
        }
        alias Types = NoDuplicates!(staticMap!(BaseTypeOf, getSymbolsByUDA!(M, Proto)));

        enum isNewType(S) = staticIndexOf!(S, T) < 0;
        alias NewTypes = Filter!(isNewType, Types);

        alias CollectMemberTypes(Member) = CollectTypes!(Member, NewTypes, T);
        alias NewMemberTypes = NoDuplicates!(staticMap!(CollectMemberTypes, NewTypes));

        static if (hasMember!(M, "toProtobuf"))
        {
            alias CollectTypes = AliasSeq!();
        }
        else
        {
            alias CollectTypes = NoDuplicates!(T, NewTypes, NewMemberTypes);
        }
    }
    else
    {
        alias CollectTypes = AliasSeq!();
    }
}

template isRecursive(T)
{
    import std.meta : staticIndexOf;

    enum isRecursive = staticIndexOf!(T, CollectTypes!T) >= 0;
}

version(unittest)
{
    class Foo
    {
        @Proto int i;
        @Proto Bar[] f;
    }

    class Bar
    {
        @Proto string s;
        @Proto Baz1[long] b;
    }

    class Baz1
    {
        @Proto string s;
        @Proto Baz2 b;
    }

    class Baz2
    {
        @Proto string s;
        @Proto Baz1 b;
    }
}

unittest
{
    import std.meta : AliasSeq;

    static assert(is(CollectTypes!Foo == AliasSeq!(int, Bar, string, long, Baz1, Baz2)));
    static assert(!isRecursive!Foo);
    static assert(!isRecursive!Bar);
    static assert(isRecursive!Baz1);
    static assert(isRecursive!Baz2);
}

auto joiner(RoR)(RoR ranges)
if (isInputRange!RoR && isInputRange!(ElementType!RoR) && hasLength!(ElementType!RoR))
{
    import std.algorithm : stdJoiner = joiner, map, sum;
    alias StdJoinerResult = typeof(stdJoiner(ranges));

    static struct Result
    {
        StdJoinerResult result;
        size_t _length;

        alias result this;

        this(RoR r)
        {
            result = StdJoinerResult(r);
            _length = r.map!(a => a.length).sum;
        }

        void popFront()
        {
            result.popFront;
            --_length;
        }

        @property size_t length()
        {
            return _length;
        }
    }

    return Result(ranges);
}

unittest
{
    import std.array : array;

    auto a = [[1, 2, 3], [], [4, 5]].joiner;

    assert(a.length == 5);
    a.popFront;
    assert(a.length == 4);
    assert(a.array == [2, 3, 4, 5]);
}

interface SizedRange(E) : InputRange!E
{
    @property size_t length();
}

class SizedRangeObject(R) : InputRangeObject!R, SizedRange!(ElementType!R)
if (isInputRange!R && hasLength!R)
{
    size_t _length;

    this(R range)
    {
        super(range);
        _length = range.length;
    }

    override void popFront()
    {
        super.popFront;
        --_length;
    }

    override @property size_t length()
    {
        return _length;
    }
}

SizedRangeObject!R sizedRangeObject(R)(R range)
if (isInputRange!R && hasLength!R)
{
    static if (is(R : SizedRange!(ElementType!R)))
        return range;
    else
        return new SizedRangeObject!R(range);
}

template isSizedRange(T)
{
    enum isSizedRange = isInputRange!T && is(typeof(T.init.length));
}

auto emptyRange(T)()
{
    static if (is(T == SizedRange!ubyte))
    {
        return sizedRangeObject(cast(ubyte[]) null);
    }
    else static if (is(T == struct))
    {
        static assert(isSizedRange!T);
        return T.init;
    }
    else
    {
        static assert(0, "No empty range for " ~ T.stringof);
    }
}

R takeLengthPrefixed(R)(ref R inputRange)
{
    import std.exception : enforce;

    long size = fromVarint(inputRange);
    enforce!ProtobufException(size >= 0, "Negative field length");
    return inputRange.takeN(size);
}

R takeN(R)(ref R inputRange, size_t size)
{
    import std.exception : enforce;
    import std.range : dropExactly, take;

    R result = inputRange.take(size);
    enforce!ProtobufException(result.length == size, "Truncated message");
    inputRange = inputRange.dropExactly(size);
    return result;
}
