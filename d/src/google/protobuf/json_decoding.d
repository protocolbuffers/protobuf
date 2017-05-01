module google.protobuf.json_decoding;

import std.exception : enforce;
import std.json : JSON_TYPE, JSONValue;
import std.traits : isAggregateType, isArray, isAssociativeArray, isBoolean, isFloatingPoint, isIntegral, isSigned;
import std.typecons : Flag, No, Yes;
import google.protobuf.common;

T fromJSONValue(T)(JSONValue value)
if (isBoolean!T)
{
    switch (value.type)
    {
    case JSON_TYPE.NULL:
        return defaultValue!T;
    case JSON_TYPE.TRUE:
        return true;
    case JSON_TYPE.FALSE:
        return false;
    default:
        throw new ProtobufException("JSON boolean expected");
    }
}

T fromJSONValue(T)(JSONValue value)
if (isIntegral!T)
{
    import std.conv : ConvException, to;

    switch (value.type)
    {
    case JSON_TYPE.NULL:
        return defaultValue!T;
    case JSON_TYPE.STRING:
        try
        {
            return value.str.to!T;
        }
        catch (ConvException ConvException)
        {
            throw new ProtobufException("JSON integer expected");
        }
    case JSON_TYPE.INTEGER:
        return cast(T) value.integer;
    case JSON_TYPE.UINTEGER:
        return cast(T) value.uinteger;
    default:
        throw new ProtobufException("JSON integer expected");
    }
}

T fromJSONValue(T)(JSONValue value)
if (isFloatingPoint!T)
{
    import std.conv : ConvException, to;
    import std.math : isInfinity, isNaN;

    switch (value.type)
    {
    case JSON_TYPE.NULL:
        return defaultValue!T;
    case JSON_TYPE.STRING:
        switch (value.str)
        {
        case "NaN":
            return T.nan;
        case "Infinity":
            return T.infinity;
        case "-Infinity":
            return -T.infinity;
        default:
            try
            {
                return value.str.to!T;
            }
            catch (ConvException ConvException)
            {
                throw new ProtobufException("JSON float expected");
            }
        }
    case JSON_TYPE.INTEGER:
        return cast(T) value.integer;
    case JSON_TYPE.UINTEGER:
        return cast(T) value.uinteger;
    case JSON_TYPE.FLOAT:
        return value.floating;
    default:
        throw new ProtobufException("JSON float expected");
    }
}

T fromJSONValue(T)(JSONValue value)
if (is(T == string))
{
    if (value.isNull)
        return defaultValue!T;

    enforce!ProtobufException(value.type == JSON_TYPE.STRING, "JSON string expected");
    return value.str;
}

T fromJSONValue(T)(JSONValue value)
if (is(T == bytes))
{
    import std.base64 : Base64;

    if (value.isNull)
        return defaultValue!T;

    enforce!ProtobufException(value.type == JSON_TYPE.STRING, "JSON base64 encoded binary expected");
    return Base64.decode(value.str);
}

T fromJSONValue(T)(JSONValue value)
if (isArray!T && !is(T == string) && !is(T == bytes))
{
    import std.algorithm : map;
    import std.array : array;
    import std.range : ElementType;

    if (value.isNull)
        return defaultValue!T;

    enforce!ProtobufException(value.type == JSON_TYPE.ARRAY, "JSON array expected");
    return value.array.map!(a => a.fromJSONValue!(ElementType!T)).array;
}

T fromJSONValue(T)(JSONValue value, T result = null)
if (isAssociativeArray!T)
{
    import std.conv : ConvException, to;
    import std.traits : KeyType, ValueType;

    if (value.isNull)
        return defaultValue!T;

    enforce!ProtobufException(value.type == JSON_TYPE.OBJECT, "JSON object expected");
    foreach (k, v; value.object)
    {
        try
        {
            result[k.to!(KeyType!T)] = v.fromJSONValue!(ValueType!T);
        }
        catch (ConvException exception)
        {
            throw new ProtobufException(exception.msg);
        }
    }

    return result;
}

unittest
{
    import std.exception : assertThrown;
    import std.json : parseJSON;
    import std.math : isInfinity, isNaN;

    assert(fromJSONValue!bool(JSONValue(false)) == false);
    assert(fromJSONValue!bool(JSONValue(true)) == true);
    assertThrown!ProtobufException(fromJSONValue!bool(JSONValue(1)));

    assert(fromJSONValue!int(JSONValue(1)) == 1);
    assert(fromJSONValue!uint(JSONValue(1U)) == 1U);
    assert(fromJSONValue!long(JSONValue(1L)) == 1);
    assert(fromJSONValue!ulong(JSONValue(1UL)) == 1U);
    assertThrown!ProtobufException(fromJSONValue!int(JSONValue(false)));
    assertThrown!ProtobufException(fromJSONValue!ulong(JSONValue("foo")));

    assert(fromJSONValue!float(JSONValue(1.0f)) == 1.0);
    assert(fromJSONValue!double(JSONValue(1.0)) == 1.0);
    assert(fromJSONValue!float(JSONValue("NaN")).isNaN);
    assert(fromJSONValue!double(JSONValue("Infinity")).isInfinity);
    assert(fromJSONValue!double(JSONValue("-Infinity")).isInfinity);
    assertThrown!ProtobufException(fromJSONValue!float(JSONValue(false)));
    assertThrown!ProtobufException(fromJSONValue!double(JSONValue("foo")));

    assert(fromJSONValue!bytes(JSONValue("Zm9v")) == cast(bytes) "foo");
    assertThrown!ProtobufException(fromJSONValue!bytes(JSONValue(1)));

    assert(fromJSONValue!(int[])(parseJSON(`[1, 2, 3]`)) == [1, 2, 3]);
    assertThrown!ProtobufException(fromJSONValue!(int[])(JSONValue(`[1, 2, 3]`)));

    assert(fromJSONValue!(bool[int])(parseJSON(`{"1": false, "2": true}`)) == [1 : false, 2 : true]);
    assertThrown!ProtobufException(fromJSONValue!(bool[int])(JSONValue(`{"1": false, "2": true}`)));
    assertThrown!ProtobufException(fromJSONValue!(bool[int])(parseJSON(`{"foo": false, "2": true}`)));
}

T fromJSONValue(T)(JSONValue value, T result = T.init)
if (isAggregateType!T)
{
    import std.traits : hasMember;

    static if (is(T == class))
    {
        if (result is null)
            result = new T;
    }

    static if (hasMember!(T, "fromJSONValue"))
    {
        return result.fromJSONValue(value);
    }
    else
    {
        if (value.isNull)
            return defaultValue!T;

        enforce!ProtobufException(value.type == JSON_TYPE.OBJECT, "JSON object expected");

        JSONValue[string] members = value.object;

        foreach (field; Message!T.fieldNames)
        {
            static if (field[$ - 1] == '_')
                enum jsonName = field[0 .. $ - 1];
            else
                enum jsonName = field;

            auto fieldValue = (jsonName in members);
            if (fieldValue !is null)
                mixin("result." ~ field) = fromJSONValue!(typeof(mixin("T." ~ field)))(*fieldValue);
        }
        return result;
    }
}

unittest
{
    import std.exception : assertThrown;
    import std.json : parseJSON;

    struct Foo
    {
        @Proto(1) int a;
        @Proto(3) string b;
        @Proto(4) bool c;
    }

    auto foo = Foo(10, "abc", false);

    assert(fromJSONValue!Foo(parseJSON(`{"a":10, "b":"abc"}`)) == Foo(10, "abc", false));
    assert(fromJSONValue!Foo(parseJSON(`{"a": 10, "b": "abc", "c": false}`)) == Foo(10, "abc", false));
    assertThrown!ProtobufException(fromJSONValue!Foo(parseJSON(`{"a":10, "b":100}`)));
}
