module google.protobuf.duration;

import core.time : StdDuration = Duration;
import std.exception : enforce;
import std.json : JSONValue;
import std.range : empty;
import google.protobuf;

struct Duration
{
    private struct _Message
    {
        @Proto(1) long seconds = defaultValue!long;
        @Proto(2) int nanos = defaultValue!int;
    }

    StdDuration duration;

    alias duration this;

    auto toProtobuf()
    {
        validateDuration;

        auto splitedDuration = duration.split!("seconds", "nsecs");
        auto protobufMessage = _Message(splitedDuration.seconds, cast(int) splitedDuration.nsecs);

        return protobufMessage.toProtobuf;
    }

    Duration fromProtobuf(R)(ref R inputRange)
    {
        import core.time : dur;

        auto protobufMessage = inputRange.fromProtobuf!_Message;
        duration = dur!"seconds"(protobufMessage.seconds) + dur!"nsecs"(protobufMessage.nanos);

        return this;
    }

    auto toJSONValue()()
    {
        import std.format : format;
        import google.protobuf.json_encoding : toJSONValue;

        validateDuration;

        auto splitedDuration = duration.split!("seconds", "nsecs");
        auto seconds = splitedDuration.seconds >= 0 ? splitedDuration.seconds : -splitedDuration.seconds;
        auto fractionalDigits = splitedDuration.nsecs >= 0 ? splitedDuration.nsecs : -splitedDuration.nsecs;
        auto fractionalLength = 9;

        foreach (i; 0 .. 3)
        {
            if (fractionalDigits % 1000 != 0)
                break;
            fractionalDigits /= 1000;
            fractionalLength -= 3;
        }

        if (fractionalDigits)
        {
            return "%s%d.%0*ds"
                .format(duration.isNegative ? "-" : "", seconds, fractionalLength, fractionalDigits)
                .toJSONValue;
        }
        else
        {
            return "%s%ds".format(duration.isNegative ? "-" : "", seconds).toJSONValue;
        }
    }

    Duration fromJSONValue()(JSONValue value)
    {
        import core.time : dur;
        import std.algorithm : skipOver;
        import std.conv : ConvException, to;
        import std.regex : matchAll, regex;
        import std.string : leftJustify;
        import google.protobuf.json_decoding : fromJSONValue;

        auto match = value.fromJSONValue!string.matchAll(`^(-)?(\d+)([.]\d*)?s$`);
        enforce!ProtobufException(match, "Invalid duration JSON encoding");

        bool negative = !match.front[1].empty;
        auto secondsPart = match.front[2];
        auto nsecsPart = match.front[3];
        nsecsPart.skipOver('.');

        try
        {
            duration = dur!"seconds"(secondsPart.to!ulong) + dur!"nsecs"(nsecsPart.leftJustify(9, '0').to!uint);
            if (negative)
                duration = -duration;

            validateDuration;
            return this;
        }
        catch (ConvException exception)
        {
            throw new ProtobufException(exception.msg);
        }
    }

    private void validateDuration()
    {
        import std.exception : enforce;

        auto seconds = duration.total!"seconds";
        enforce!ProtobufException(-315_576_000_001L < seconds && seconds < 315_576_000_001,
            "Duration is out of range approximately +-10_000 years.");
    }
}

unittest
{
    import std.algorithm.comparison : equal;
    import std.array : array;
    import std.datetime : msecs, seconds;

    assert(equal(Duration(5.seconds + 5.msecs).toProtobuf, [0x08, 0x05, 0x10, 0xc0, 0x96, 0xb1, 0x02]));
    assert(equal(Duration(5.msecs).toProtobuf, [0x10, 0xc0, 0x96, 0xb1, 0x02]));
    assert(equal(Duration((-5).msecs).toProtobuf, [0x10, 0xc0, 0xe9, 0xce, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01]));
    assert(equal(Duration((-5).seconds + (-5).msecs).toProtobuf, [
        0x08, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01,
        0x10, 0xc0, 0xe9, 0xce, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01]));

    assert(equal(Duration(5.msecs).toProtobuf, [0x10, 0xc0, 0x96, 0xb1, 0x02]));

    auto buffer = Duration(5.seconds + 5.msecs).toProtobuf.array;
    assert(buffer.fromProtobuf!Duration == Duration(5.seconds + 5.msecs));
    buffer = Duration(5.msecs).toProtobuf.array;
    assert(buffer.fromProtobuf!Duration == Duration(5.msecs));
    buffer = Duration((-5).msecs).toProtobuf.array;
    assert(buffer.fromProtobuf!Duration == Duration((-5).msecs));
    buffer = Duration((-5).seconds + (-5).msecs).toProtobuf.array;
    assert(buffer.fromProtobuf!Duration == Duration((-5).seconds + (-5).msecs));

    buffer = Duration(StdDuration.zero).toProtobuf.array;
    assert(buffer.empty);
    assert(buffer.fromProtobuf!Duration == Duration.zero);
}

unittest
{
    import std.datetime : msecs, nsecs, seconds, weeks;
    import std.exception : assertThrown;
    import std.json : JSONValue;

    assert(Duration(0.seconds).toJSONValue == JSONValue("0s"));
    assert(Duration(1.seconds).toJSONValue == JSONValue("1s"));
    assert(Duration((-1).seconds).toJSONValue == JSONValue("-1s"));
    assert(Duration(0.seconds + 50.msecs).toJSONValue == JSONValue("0.050s"));
    assert(Duration(0.seconds - 50.msecs).toJSONValue == JSONValue("-0.050s"));
    assert(Duration(0.seconds - 300.nsecs).toJSONValue == JSONValue("-0.000000300s"));
    assert(Duration(-100.seconds - 300.nsecs).toJSONValue == JSONValue("-100.000000300s"));

    assertThrown!ProtobufException(Duration(530_000.weeks).toJSONValue);
    assertThrown!ProtobufException(Duration(-530_000.weeks).toJSONValue);

    Duration foo;
    assert(Duration(0.seconds) == foo.fromJSONValue(JSONValue("0s")));
    assert(Duration(1.seconds) == foo.fromJSONValue(JSONValue("1s")));
    assert(Duration((-1).seconds) == foo.fromJSONValue(JSONValue("-1s")));
    assert(Duration(0.seconds + 50.msecs) == foo.fromJSONValue(JSONValue("0.050s")));
    assert(Duration(0.seconds - 50.msecs) == foo.fromJSONValue(JSONValue("-0.050s")));
    assert(Duration(0.seconds - 300.nsecs) == foo.fromJSONValue(JSONValue("-0.000000300s")));
    assert(Duration(-100.seconds - 300.nsecs) == foo.fromJSONValue(JSONValue("-100.000000300s")));

    assertThrown!ProtobufException(foo.fromJSONValue(JSONValue("315576000001s")));
    assertThrown!ProtobufException(foo.fromJSONValue(JSONValue("315576000001")));
}
