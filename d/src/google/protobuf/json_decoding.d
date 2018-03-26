module google.protobuf.json_decoding;

import std.json : JSONValue, JSON_TYPE;
import std.traits : isArray, isAssociativeArray, isBoolean, isFloatingPoint, isIntegral, isSigned;
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
    import std.exception : enforce;
    import std.traits : OriginalType;

    try
    {
        switch (value.type)
        {
        case JSON_TYPE.NULL:
            return defaultValue!T;
        case JSON_TYPE.STRING:
            return value.str.to!T;
        case JSON_TYPE.INTEGER:
            return cast(T) value.integer.to!(OriginalType!T);
        case JSON_TYPE.UINTEGER:
            return cast(T) value.uinteger.to!(OriginalType!T);
        case JSON_TYPE.FLOAT:
        {
            import core.stdc.math : fabs, modf;

            double integral;
            double fractional = modf(value.floating, &integral);
            double epsilon = double.epsilon * fabs(integral);

            enforce!ProtobufException(fabs(fractional) <= epsilon, "JSON integer expected");

            return value.floating.to!T;
        }
        default:
            throw new ProtobufException("JSON integer expected");
        }
    }
    catch (ConvException ConvException)
    {
        throw new ProtobufException("JSON integer expected");
    }
}

T fromJSONValue(T)(JSONValue value)
if (isFloatingPoint!T)
{
    import std.conv : ConvException, to;
    import std.math : isInfinity, isNaN;

    try
    {
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
                return value.str.to!T;
            }
        case JSON_TYPE.INTEGER:
            return value.integer.to!T;
        case JSON_TYPE.UINTEGER:
            return value.uinteger.to!T;
        case JSON_TYPE.FLOAT:
            return value.floating;
        default:
            throw new ProtobufException("JSON float expected");
        }
    }
    catch (ConvException ConvException)
    {
        throw new ProtobufException("JSON float expected");
    }
}

T fromJSONValue(T)(JSONValue value)
if (is(T == string))
{
    import std.exception : enforce;

    if (value.isNull)
        return defaultValue!T;

    enforce!ProtobufException(value.type == JSON_TYPE.STRING, "JSON string expected");
    return value.str;
}

T fromJSONValue(T)(JSONValue value)
if (is(T == bytes))
{
    import std.base64 : Base64;
    import std.exception : enforce;
    import std.json : JSON_TYPE;

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
    import std.exception : enforce;
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
    import std.exception : enforce;
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

T fromJSONValue(T)(JSONValue value, T result = defaultValue!T)
if (is(T == class) || is(T == struct))
{
    import std.algorithm : findAmong;
    import std.exception : enforce;
    import std.meta : staticMap;
    import std.range : empty;
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
        enum string jsonName(string fieldName) = {
            import std.algorithm : skipOver;

            string result = fieldName;

            if (fieldName[$ - 1] == '_')
                result = fieldName[0 .. $ - 1];

            static if (isOneof!(mixin("T." ~ fieldName)))
                result.skipOver("_");

            return result;
        }();

        if (value.isNull)
            return defaultValue!T;

        enforce!ProtobufException(value.type == JSON_TYPE.OBJECT, "JSON object expected");

        JSONValue[string] members = value.object;

        foreach (fieldName; Message!T.fieldNames)
        {
            enum jsonFieldName = jsonName!fieldName;

            auto fieldValue = (jsonFieldName in members);
            if (fieldValue !is null)
            {
                static if (isOneof!(mixin("T." ~ fieldName)))
                {
                    alias otherFields = staticMap!(jsonName, otherOneofFieldNames!(T, mixin("T." ~ fieldName)));
                    enforce!ProtobufException(members.keys.findAmong([otherFields]).empty,
                            "More than one oneof field in JSON Message");
                }

                mixin("result." ~ fieldName) = fromJSONValue!(typeof(mixin("T." ~ fieldName)))(*fieldValue);
            }
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

        @Oneof("test")
        union
        {
            @Proto(5) int _d;
            @Proto(6) string _e;
        }
    }

    auto foo = Foo(10, "abc", false);

    assert(fromJSONValue!Foo(parseJSON(`{"a":10, "b":"abc"}`)) == Foo(10, "abc", false));
    assert(fromJSONValue!Foo(parseJSON(`{"a": 10, "b": "abc", "c": false}`)) == Foo(10, "abc", false));
    assertThrown!ProtobufException(fromJSONValue!Foo(parseJSON(`{"a":10, "b":100}`)));
    assertThrown!ProtobufException(fromJSONValue!Foo(parseJSON(`{"d":10, "e":"abc"}`)));
}

private template oneofs(T)
{
    import std.meta : NoDuplicates, staticMap;
    import std.traits : getSymbolsByUDA;

    private alias oneofs = NoDuplicates!(staticMap!(oneofByField, getSymbolsByUDA!(T, Oneof)));
}

private template oneofByField(alias field)
{
    import std.traits : getUDAs;

    enum Oneof oneofByField = getUDAs!(field, Oneof)[0];
}

private template otherOneofFieldNames(T, alias field)
{
    import std.meta : Erase, Filter, staticMap;

    static assert(is(typeof(__traits(parent, field).init) == T));
    static assert(isOneof!field);

    static enum hasSameOneofCase(alias field2) = oneofCaseFieldName!field == oneofCaseFieldName!field2;
    static enum fieldName(alias field) = __traits(identifier, field);

    enum otherOneofFieldNames = staticMap!(fieldName, Erase!(field, Filter!(hasSameOneofCase, Filter!(isOneof,
            Message!T.fields))));
}

unittest
{
    import std.meta : AliasSeq, staticMap;

    static struct Test
    {
        @Oneof("foo")
        union
        {
            @Proto(1) int foo1;
            @Proto(2) int foo2;
        }
        @Oneof("bar")
        union
        {
            @Proto(11) int bar1;
            @Proto(12) int bar2;
            @Proto(13) int bar3;
        }

        @Proto(20) int baz;
    }

    static assert([otherOneofFieldNames!(Test, Test.foo1)] == ["foo2"]);
    static assert([otherOneofFieldNames!(Test, Test.bar2)] == ["bar1", "bar3"]);
}
