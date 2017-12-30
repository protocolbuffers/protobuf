module google.protobuf.any;

import std.json : JSONValue;
import std.typecons : Flag, No, Yes;
import google.protobuf;

enum defaultUrlPrefix = "type.googleapis.com";

struct Any
{
    private struct _Message
    {
        @Proto(1) string typeUrl = defaultValue!string;
        @Proto(2) bytes value = defaultValue!bytes;
    }

    string typeUrl;
    private bool valueIsJSON;
    private union {
        bytes protoValue;
        JSONValue jsonValue;
    }

    T to(T)(string urlPrefix = defaultUrlPrefix)
    {
        import std.exception : enforce;
        import std.format : format;

        enforce!ProtobufException(isMessageType!T(urlPrefix),
            "Incompatible target type `%s` for Any message fo type '%s'".format(messageTypeUrl!T(urlPrefix), typeUrl));

        if (valueIsJSON)
        {
            return jsonValue.fromJSONValue!T;
        }
        else
        {
            return protoValue.fromProtobuf!T;
        }
    }

    Any from(T)(T value, string urlPrefix = defaultUrlPrefix)
    {
        import std.array : array;

        typeUrl = messageTypeUrl!T(urlPrefix);
        protoValue = value.toProtobuf.array;
        valueIsJSON = false;

        return this;
    }

    bool isMessageType(T)(string urlPrefix = defaultUrlPrefix)
    {
        return typeUrl == messageTypeUrl!T(urlPrefix);
    }

    auto toProtobuf()
    {
        import std.array : array;
        import std.exception : enforce;
        import std.format : format;
        import std.json : JSON_TYPE;

        if (valueIsJSON) {
            enforce!ProtobufException(typeUrl in messageTypes,
                    "Cannot handle 'Any' message: type '%s' is not registered".format(typeUrl));
            enforce!ProtobufException(jsonValue.type == JSON_TYPE.OBJECT,
                    "'Any' message JSON encoding must be an object");

            JSONValue jsonMapping;

            if (messageTypes[typeUrl].hasSpecialJSONMapping)
            {
                enforce!ProtobufException("value" in jsonValue.object,
                        "'Any' message with special JSON mapping must have a 'value' entry");
                jsonMapping = jsonValue.object["value"];
            }
            else
            {
                jsonMapping = jsonValue;
            }

            return _Message(typeUrl, messageTypes[typeUrl].JSONValueToProtobuf(jsonMapping).array).toProtobuf;
        }

        return _Message(typeUrl, protoValue).toProtobuf;
    }

    Any fromProtobuf(R)(ref R inputRange)
    {
        auto message = inputRange.fromProtobuf!_Message;

        typeUrl = message.typeUrl;
        protoValue = message.value;
        valueIsJSON = false;

        return this;
    }

    JSONValue toJSONValue()()
    {
        import std.format : format;
        import std.exception : enforce;
        import std.json : JSON_TYPE;

        if (!valueIsJSON) {
            enforce!ProtobufException(typeUrl in messageTypes,
                    "Cannot handle 'Any' message: type '%s' is not registered".format(typeUrl));

            auto result = messageTypes[typeUrl].protobufToJSONValue(protoValue);

            if (messageTypes[typeUrl].hasSpecialJSONMapping)
            {
                return JSONValue([
                    "@type": JSONValue(typeUrl),
                    "value": result,
                ]);
            }

            enforce!ProtobufException(result.type == JSON_TYPE.OBJECT,
                    "'Any' message of type '%s' with no special JSON mapping is not an JSON object".format(typeUrl));
            result.object["@type"] = typeUrl;
            return result;
        }

        enforce!ProtobufException(jsonValue.type == JSON_TYPE.OBJECT, "'Any' message JSON encoding must be an object");
        jsonValue.object["@type"] = typeUrl;

        return jsonValue;
    }

    Any fromJSONValue()(JSONValue value)
    {
        import std.exception : enforce;
        import std.json : JSON_TYPE;

        if (value.type == JSON_TYPE.NULL)
        {
            typeUrl = "";
            protoValue = [];
            valueIsJSON = false;

            return this;
        }

        enforce!ProtobufException(value.type == JSON_TYPE.OBJECT, "Invalid 'Any' JSON encoding");
        enforce!ProtobufException("@type" in value.object, "No type specified for 'Any' JSON encoding");
        enforce!ProtobufException(value.object["@type"].type == JSON_TYPE.STRING, "Any typeUrl should be a string");

        typeUrl = value.object["@type"].str;
        jsonValue = value;
        valueIsJSON = true;

        return this;
    }

    static EncodingInfo[string] messageTypes;

    static registerMessageType(T, Flag!"hasSpecialJSONMapping" hasSpecialJSONMapping = No.hasSpecialJSONMapping)
            (string urlPrefix = defaultUrlPrefix)
    {
        import std.array : array;
        import std.exception : enforce;
        import std.format : format;

        string url = messageTypeUrl!T(urlPrefix);
        enforce!ProtobufException(url !in messageTypes, "Message type '%s' already registered".format(url));

        messageTypes[url] = EncodingInfo(function(bytes input) => input.fromProtobuf!T.toJSONValue,
            function(JSONValue input) => input.fromJSONValue!T.toProtobuf.array, hasSpecialJSONMapping);
    }

    struct EncodingInfo
    {
        JSONValue function(bytes) protobufToJSONValue;
        bytes function(JSONValue) JSONValueToProtobuf;
        bool hasSpecialJSONMapping;
    }
}

static this()
{
    import google.protobuf.duration : Duration;
    import google.protobuf.empty : Empty;
    import google.protobuf.field_mask : FieldMask;
    import google.protobuf.struct_ : ListValue, NullValue, Struct, Value;
    import google.protobuf.timestamp : Timestamp;
    import google.protobuf.wrappers : BoolValue, BytesValue, DoubleValue, FloatValue, Int32Value, Int64Value,
        StringValue, UInt32Value, UInt64Value;

    Any.registerMessageType!(Any, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(BoolValue, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(BytesValue, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(DoubleValue, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(Duration, Yes.hasSpecialJSONMapping);
    //Any.registerMessageType!(Empty, Yes.hasSpecialJSONMapping); // TODO: Somehow it causes a linkage error
    Any.registerMessageType!(FieldMask, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(FloatValue, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(Int32Value, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(Int64Value, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(ListValue, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(NullValue, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(StringValue, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(Struct, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(Timestamp, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(UInt32Value, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(UInt64Value, Yes.hasSpecialJSONMapping);
    Any.registerMessageType!(Value, Yes.hasSpecialJSONMapping);
}

unittest
{
    struct Foo
    {
        enum messageTypeFullName = "Foo";

        @Proto(1) int intField = defaultValue!int;
        @Proto(2) string stringField = defaultValue!string;
    }
    auto foo1 = Foo(42, "abc");
    Any anyFoo;

    assert(!anyFoo.isMessageType!Foo("my.prefix"));

    anyFoo.from(foo1, "my.prefix");
    auto foo2 = anyFoo.to!Foo("my.prefix");

    assert(anyFoo.isMessageType!Foo("my.prefix"));
    assert(anyFoo.typeUrl == "my.prefix/Foo");
    assert(foo1 == foo2);
}


enum messageTypeFullName(T) = {
    import std.algorithm : splitter;
    import std.array : join, split;
    import std.traits : fullyQualifiedName, hasMember;

    static if (hasMember!(T, "messageTypeFullName"))
    {
        return T.messageTypeFullName;
    }

    enum splitName = fullyQualifiedName!T.split(".");

    return (splitName[0..$-2] ~ splitName[$-1]).join(".");
}();

unittest
{
    import google.protobuf.duration : Duration;
    import google.protobuf.empty : Empty;
    import google.protobuf.field_mask : FieldMask;
    import google.protobuf.struct_ : ListValue, NullValue, Struct, Value;
    import google.protobuf.timestamp : Timestamp;
    import google.protobuf.wrappers : BoolValue, BytesValue, DoubleValue, FloatValue, Int32Value, Int64Value,
        StringValue, UInt32Value, UInt64Value;

    static assert(messageTypeFullName!Any == "google.protobuf.Any");
    static assert(messageTypeFullName!BoolValue == "google.protobuf.BoolValue");
    static assert(messageTypeFullName!BytesValue == "google.protobuf.BytesValue");
    static assert(messageTypeFullName!DoubleValue == "google.protobuf.DoubleValue");
    static assert(messageTypeFullName!Duration == "google.protobuf.Duration");
    static assert(messageTypeFullName!Empty == "google.protobuf.Empty");
    static assert(messageTypeFullName!FieldMask == "google.protobuf.FieldMask");
    static assert(messageTypeFullName!FloatValue == "google.protobuf.FloatValue");
    static assert(messageTypeFullName!Int32Value == "google.protobuf.Int32Value");
    static assert(messageTypeFullName!Int64Value == "google.protobuf.Int64Value");
    static assert(messageTypeFullName!ListValue == "google.protobuf.ListValue");
    static assert(messageTypeFullName!NullValue == "google.protobuf.NullValue");
    static assert(messageTypeFullName!StringValue == "google.protobuf.StringValue");
    static assert(messageTypeFullName!Struct == "google.protobuf.Struct");
    static assert(messageTypeFullName!Timestamp == "google.protobuf.Timestamp");
    static assert(messageTypeFullName!UInt32Value == "google.protobuf.UInt32Value");
    static assert(messageTypeFullName!UInt64Value == "google.protobuf.UInt64Value");
    static assert(messageTypeFullName!Value == "google.protobuf.Value");
}

string messageTypeUrl(T)(string urlPrefix = defaultUrlPrefix)
{
    return urlPrefix ~ "/" ~ messageTypeFullName!T;
}
