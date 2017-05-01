module google.protobuf.field_mask;

import std.exception : enforce;
import google.protobuf;

struct FieldMask
{
    @Proto(1) string[] paths = defaultValue!(string[]);

    auto toJSONValue()
    {
        import std.algorithm : map, joiner;
        import std.conv : to;
        import google.protobuf.json_encoding;

        return paths.map!(a => a.toCamelCase).joiner(",").to!string.toJSONValue;
    }
}

unittest
{
    import std.json : JSONValue;

    assert(FieldMask(["foo"]).toJSONValue == JSONValue("foo"));
    assert(FieldMask(["foo", "bar_baz"]).toJSONValue == JSONValue("foo,barBaz"));
    assert(FieldMask(["foo", "bar_baz.qux"]).toJSONValue == JSONValue("foo,barBaz.qux"));
}

string toCamelCase(string snakeCase) pure
{
    import std.array : Appender;
    import std.ascii : isLower, isUpper, toUpper;

    Appender!string result;
    bool capitalizeNext;
    bool wordStart = true;
    foreach (c; snakeCase)
    {
        enforce!ProtobufException(!c.isUpper, "Invalid field mask " ~ snakeCase);
        enforce!ProtobufException(!wordStart || c.isLower || c == '_' || c == '.', "Invalid field mask " ~ snakeCase);
        wordStart = (c == '_' || c == '.');
        if (c == '_')
        {
            enforce!ProtobufException(!capitalizeNext, "Invalid field mask " ~ snakeCase);
            capitalizeNext = true;
            continue;
        }
        if (capitalizeNext)
        {
            result ~= c.toUpper;
            capitalizeNext = false;
            continue;
        }
        result ~= c;
    }

    return result.data;
}

unittest
{
    import std.exception : assertThrown;

    assert("foo".toCamelCase == "foo");
    assert("foo1".toCamelCase == "foo1");
    assert("foo_bar".toCamelCase == "fooBar");
    assert("_foo_bar".toCamelCase == "FooBar");
    assert("foo_bar_baz_qux".toCamelCase == "fooBarBazQux");
    assertThrown!ProtobufException("__".toCamelCase);
    assertThrown!ProtobufException("foo__bar".toCamelCase);
    assertThrown!ProtobufException("fooBar".toCamelCase);
    assertThrown!ProtobufException("foo_1".toCamelCase);
    assertThrown!ProtobufException("1_foo".toCamelCase);
    assertThrown!ProtobufException("_1_foo".toCamelCase);
    
    assert("foo.bar".toCamelCase == "foo.bar");
    assert(".foo..bar.".toCamelCase == ".foo..bar.");
    assert("foo_bar.baz_qux".toCamelCase == "fooBar.bazQux");
}

string toSnakeCase(string camelCase) pure
{
    import std.array : Appender;
    import std.ascii : isUpper, toLower;

    Appender!string result;

    foreach (c; camelCase)
    {
        enforce!ProtobufException(c != '_', "Invalid field mask " ~ camelCase);
        if (c.isUpper)
        {
            result ~= '_';
            result ~= c.toLower;
        }
        else
        {
            result ~= c;
        }
    }

    return result.data;
}

unittest
{
    import std.exception : assertThrown;

    assert("".toSnakeCase == "");
    assert("fooBar".toSnakeCase == "foo_bar");
    assert("foo1".toSnakeCase == "foo1");
    assert("FooBar".toSnakeCase == "_foo_bar");
    assert("fooBarBazQux".toSnakeCase == "foo_bar_baz_qux");
    assertThrown!ProtobufException("foo_Bar".toSnakeCase);
    assertThrown!ProtobufException("foo_Bar".toSnakeCase);
    
    assert("foo.bar".toSnakeCase == "foo.bar");
    assert(".foo..bar.".toSnakeCase == ".foo..bar.");
    assert("fooBar.bazQux".toSnakeCase == "foo_bar.baz_qux");
}
