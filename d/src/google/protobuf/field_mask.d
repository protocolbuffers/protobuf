module google.protobuf.field_mask;

import std.json : JSONValue;
import google.protobuf;

struct FieldMask
{
    @Proto(1) string[] paths = defaultValue!(string[]);

    JSONValue toJSONValue()()
    {
        import std.algorithm : map;
        import std.array : join;
        import google.protobuf.json_encoding : toJSONValue;

        return paths.map!(a => a.toCamelCase).join(",").toJSONValue;
    }

    FieldMask fromJSONValue()(JSONValue value)
    {
        import std.algorithm : map, splitter;
        import std.array : array;
        import std.exception : enforce;
        import std.json : JSON_TYPE;

        if (value.type == JSON_TYPE.NULL)
        {
            paths = defaultValue!(string[]);
            return this;
        }

        enforce!ProtobufException(value.type == JSON_TYPE.STRING, "FieldMask JSON encoding must be a string");

        paths = value.str.splitter(",").map!(a => a.toSnakeCase).array;

        return this;
    }
}

unittest
{
    assert(FieldMask(["foo"]).toJSONValue == JSONValue("foo"));
    assert(FieldMask(["foo", "bar_baz"]).toJSONValue == JSONValue("foo,barBaz"));
    assert(FieldMask(["foo", "bar_baz.qux"]).toJSONValue == JSONValue("foo,barBaz.qux"));
}

unittest
{
    FieldMask foo;
    assert(FieldMask(["foo"]) == foo.fromJSONValue(JSONValue("foo")));
    assert(FieldMask(["foo", "bar_baz"]) == foo.fromJSONValue(JSONValue("foo,barBaz")));
    assert(FieldMask(["foo", "bar_baz.qux"]) == foo.fromJSONValue(JSONValue("foo,barBaz.qux")));
}

string toCamelCase(string snakeCase) pure
{
    import std.array : Appender;
    import std.ascii : isLower, isUpper, toUpper;
    import std.exception : enforce;

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
    import std.exception : enforce;

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
