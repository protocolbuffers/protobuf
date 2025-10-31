using System.IO;
using Google.Protobuf.Buffers;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf;

public class MessageExtensionsTest
{
    [Test]
    public void TestWriteLengthPrefixedToWithBufferWriter()
    {
        TestWellKnownTypes message = new() { StringField = "abcd" };

        TestArrayBufferWriter<byte> buffer = new();
        message.WriteLengthPrefixedTo(buffer);

        Assert.That(
            buffer.WrittenSpan.ToArray(),
            Is.EqualTo(new byte[]
            {
                9, // length prefix
                138, 1, // tag
                6, // message length
                10, // tag
                4, // string length
                97, 98, 99, 100, // abcd
            }));
    }

    [Test]
    public void TestWriteLengthPrefixedToRoundTripWithBufferWriter()
    {
        TestWellKnownTypes message = new() { Int32Field = 25 };

        TestArrayBufferWriter<byte> buffer = new();
        message.WriteLengthPrefixedTo(buffer);

        Assert.That(
            TestWellKnownTypes.Parser.ParseDelimitedFrom(new MemoryStream(buffer.WrittenSpan.ToArray())),
            Is.EqualTo(message));
    }
}