using System.Collections;
using System.Collections.Generic;
using System.IO;
using NUnit.Framework;
using NestedMessage = Google.ProtocolBuffers.TestProtos.TestAllTypes.Types.NestedMessage;
using Google.ProtocolBuffers.TestProtos;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class MessageStreamIteratorTest {

    [Test]
    public void ThreeMessagesInMemory() {
      MemoryStream stream = new MemoryStream(MessageStreamWriterTest.ThreeMessageData);      
      IEnumerable<NestedMessage> iterator = MessageStreamIterator<NestedMessage>.FromStreamProvider(() => stream);
      List<NestedMessage> messages = new List<NestedMessage>(iterator);

      Assert.AreEqual(3, messages.Count);
      Assert.AreEqual(5, messages[0].Bb);
      Assert.AreEqual(1500, messages[1].Bb);
      Assert.IsFalse(messages[2].HasBb);
    }

    [Test]
    public void ManyMessagesShouldNotTriggerSizeAlert() {
      int messageSize = TestUtil.GetAllSet().SerializedSize;
      // Enough messages to trigger the alert unless we've reset the size
      // Note that currently we need to make this big enough to copy two whole buffers,
      // as otherwise when we refill the buffer the second type, the alert triggers instantly.
      int correctCount = (CodedInputStream.BufferSize * 2) / messageSize + 1;
      using (MemoryStream stream = new MemoryStream()) {
        MessageStreamWriter<TestAllTypes> writer = new MessageStreamWriter<TestAllTypes>(stream);
        for (int i = 0; i < correctCount; i++) {
          writer.Write(TestUtil.GetAllSet());
        }
        writer.Flush();

        stream.Position = 0;

        int count = 0;
        foreach (var message in MessageStreamIterator<TestAllTypes>.FromStreamProvider(() => stream)
          .WithSizeLimit(CodedInputStream.BufferSize * 2)) {
          count++;
          TestUtil.AssertAllFieldsSet(message);
        }
        Assert.AreEqual(correctCount, count);
      }
    }
  }
}
