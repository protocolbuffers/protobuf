module google.protobuf.wrappers;

import std.json : JSONValue;
import google.protobuf;

struct WrappedValue(T, string messageTypeFullName_)
{
    import std.typecons : Nullable;

    enum messageTypeFullName = messageTypeFullName_;
    static assert(messageTypeFullName_.length != 0);

    private struct _Message
    {
        @Proto(1) T value = defaultValue!T;
    }

    Nullable!T value;

    alias value this;

    inout this(inout T value)
    {
        this.value = value;
    }

    auto toProtobuf()
    {
        return _Message(value).toProtobuf;
    }

    auto fromProtobuf(R)(ref R inputRange)
    {
        value = inputRange.fromProtobuf!_Message.value;
        return this;
    }

    JSONValue toJSONValue()()
    {
        import google.protobuf.json_encoding;

        return value.isNull ? JSONValue(null) : value.get.toJSONValue;
    }

    auto fromJSONValue()(JSONValue jsonValue)
    {
        import google.protobuf.json_encoding;

        if (jsonValue.isNull)
            value.nullify;
        else
            value = jsonValue.fromJSONValue!T;
        return this;
    }
}

alias DoubleValue = WrappedValue!(double, "google.protobuf.DoubleValue");
alias FloatValue = WrappedValue!(float, "google.protobuf.FloatValue");
alias Int64Value = WrappedValue!(long, "google.protobuf.Int64Value");
alias UInt64Value = WrappedValue!(ulong, "google.protobuf.UInt64Value");
alias Int32Value = WrappedValue!(int, "google.protobuf.Int32Value");
alias UInt32Value = WrappedValue!(uint, "google.protobuf.UInt32Value");
alias BoolValue = WrappedValue!(bool, "google.protobuf.BoolValue");
alias StringValue = WrappedValue!(string, "google.protobuf.StringValue");
alias BytesValue = WrappedValue!(bytes, "google.protobuf.BytesValue");

unittest
{
    import std.algorithm.comparison : equal;
    import std.array : array, empty;
    import std.meta : AliasSeq;

    auto buffer = Int64Value(10).toProtobuf.array;
    assert(equal(buffer, [0x08, 0x0a]));
    assert(buffer.fromProtobuf!UInt32Value == 10);

    assert(buffer.empty);
    foreach (WrappedValue; AliasSeq!(DoubleValue, FloatValue, Int64Value, UInt64Value, Int32Value, UInt32Value,
            BoolValue, StringValue, BytesValue))
    {
        static assert(defaultValue!WrappedValue.isNull);
        assert(buffer.fromProtobuf!WrappedValue == defaultValue!(typeof(WrappedValue.value.get)));
    }
}
