using System.IO;
using Google.Protobuf.Buffers;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf;

public class MessageExtensionsTest
{
    [Test]
    public void TestWriteDelimitedToRoundTripWithBufferWriter()
    {
        TestWellKnownTypes message = new() { Int32Field = 25 };

        TestArrayBufferWriter<byte> buffer = new();
        message.WriteDelimitedTo(buffer);

        Assert.That(
            TestWellKnownTypes.Parser.ParseDelimitedFrom(new MemoryStream(buffer.WrittenSpan.ToArray())),
            Is.EqualTo(message));
    }
}