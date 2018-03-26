module google.protobuf.empty;

import std.json : JSONValue;
import google.protobuf;

struct Empty
{
    auto toProtobuf()
    {
        return cast(ubyte[]) null;
    }

    Empty fromProtobuf(R)(ref R inputRange)
    {
        import std.range : drop;

        inputRange = inputRange.drop(inputRange.length);
        return this;
    }

    JSONValue toJSONValue()()
    {
        return JSONValue(cast(JSONValue[string]) null);
    }

    Empty fromJSONValue()(JSONValue value)
    {
        import std.exception : enforce;
        import std.json : JSON_TYPE;
        import std.range : empty;

        if (value.type == JSON_TYPE.NULL)
        {
            return this;
        }

        enforce!ProtobufException(value.type == JSON_TYPE.OBJECT && value.object.empty,
            "Invalid google.protobuf.Empty JSON Encoding");

        return this;
    }
}

unittest
{
    import std.range : empty;

    assert(Empty().toProtobuf.empty);
    ubyte[] foo = [1, 2, 3];
    Empty().fromProtobuf(foo);
    assert(foo.empty);
}

unittest
{
    import std.json : JSON_TYPE;
    import std.range : empty;

    assert(Empty().toJSONValue.type == JSON_TYPE.OBJECT);
    assert(Empty().toJSONValue.object.empty);
}
