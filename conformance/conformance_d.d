import google.protobuf;
import conformance.conformance : ConformanceRequest, ConformanceResponse;

void doTest(ConformanceRequest request, ConformanceResponse response)
{
    import std.array : array, save;
    import std.base64 : Base64Exception;
    import std.json : JSONException, parseJSON, toJSON;
    import std.typecons : Yes;
    import conformance.conformance : WireFormat;
    import protobuf_test_messages.proto3.test_messages_proto3 : TestAllTypes;

    TestAllTypes testMessage;

    final switch (request.payloadCase)
    {
    case ConformanceRequest.PayloadCase.protobufPayload:
        try
        {
            auto payload = request.protobufPayload.save;
            testMessage = payload.fromProtobuf!TestAllTypes;
        }
        catch (ProtobufException decodeException)
        {
            response.parseError = decodeException.msg;
            return;
        }
        break;
    case ConformanceRequest.PayloadCase.jsonPayload:
        try
        {
            auto payload = request.jsonPayload.save;
            testMessage = parseJSON(payload).fromJSONValue!TestAllTypes;
        }
        catch (Base64Exception decodeException)
        {
            response.parseError = decodeException.msg;
            return;
        }
        catch (JSONException decodeException)
        {
            response.parseError = decodeException.msg;
            return;
        }
        catch (ProtobufException decodeException)
        {
            response.parseError = decodeException.msg;
            return;
        }
        break;
    case ConformanceRequest.PayloadCase.payloadNotSet:
        response.runtimeError = "Request has no payload.";
        return;
    }

    final switch (request.requestedOutputFormat)
    {
    case WireFormat.UNSPECIFIED:
        response.runtimeError = "Unspecified output format";
        return;
    case WireFormat.PROTOBUF:
        response.protobufPayload = testMessage.toProtobuf.array;
        break;
    case WireFormat.JSON:
        try
        {
            auto result = testMessage.toJSONValue;
            response.jsonPayload = toJSON(result);
        }
        catch (ProtobufException serializeException)
        {
            response.serializeError = serializeException.msg;
            return;
        }
        break;
    }
}

bool doTestIo()
{
    import std.array : array;
    import std.exception : enforce;
    import std.stdio : stdin, stdout;

    static ubyte[4] lengthBuffer;
    ubyte[] lengthData;
    size_t length;
    static ubyte[] testBuffer;
    ubyte[] testData;

    lengthData = stdin.rawRead(lengthBuffer[]);
    if (lengthData.length == 0)
        return false;

    enforce(lengthData.length == 4, "Truncate test length");
    length = *cast(uint*) lengthData.ptr;

    if (testBuffer.length < length)
        testBuffer.length = length;

    testData = stdin.rawRead(testBuffer[0 .. length]);
    enforce(testData.length == length, "Truncated test");

    auto request = testData.fromProtobuf!ConformanceRequest;
    auto response = new ConformanceResponse;

    doTest(request, response);

    auto encodedResult = toProtobuf(response);
    enforce(encodedResult.length < (1L << 32));
    *cast(uint*) lengthBuffer.ptr = cast(uint) encodedResult.length;

    stdout.rawWrite(lengthBuffer);
    stdout.rawWrite(encodedResult.array);
    stdout.flush;

    return true;
}

void main()
{
    import google.protobuf.any : Any;
    import protobuf_test_messages.proto3.test_messages_proto3 : TestAllTypes;

    Any.registerMessageType!TestAllTypes;

    while (doTestIo)
    {
    }
}
