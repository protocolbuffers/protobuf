module google.protobuf.empty;

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

    auto toJSONValue()
    {
        import std.json : JSONValue;

        return JSONValue(cast(JSONValue[string]) null);
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
    assert(Empty().toJSONValue.object.length == 0);
}
