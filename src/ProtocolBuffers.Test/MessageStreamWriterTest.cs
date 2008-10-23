using System.IO;
using NUnit.Framework;
using NestedMessage = Google.ProtocolBuffers.TestProtos.TestAllTypes.Types.NestedMessage;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class MessageStreamWriterTest {

    internal static readonly byte[] ThreeMessageData = new byte[] {
        (1 << 3) | 2, 2, // Field 1, 2 bytes long (first message)
        (1 << 3) | 0, 5, // Field 1, value 5
        (1 << 3) | 2, 3, // Field 1, 3 bytes long (second message)
        (1 << 3) | 0, (1500 & 0x7f) | 0x80, 1500 >> 7, // Field 1, value 1500
        (1 << 3) | 2, 0, // Field 1, no data (third message)
    };

    [Test]
    public void ThreeMessages() {
      NestedMessage message1 = new NestedMessage.Builder { Bb = 5 }.Build();
      NestedMessage message2 = new NestedMessage.Builder { Bb = 1500 }.Build();
      NestedMessage message3 = new NestedMessage.Builder().Build();

      byte[] data;
      using (MemoryStream stream = new MemoryStream()) {
        MessageStreamWriter<NestedMessage> writer = new MessageStreamWriter<NestedMessage>(stream);
        writer.Write(message1);
        writer.Write(message2);
        writer.Write(message3);
        writer.Flush();
        data = stream.ToArray();
      }

      TestUtil.AssertEqualBytes(ThreeMessageData, data);
    }
  }
}
