using System.Collections;
using System.Collections.Generic;
using System.IO;
using NUnit.Framework;
using NestedMessage = Google.ProtocolBuffers.TestProtos.TestAllTypes.Types.NestedMessage;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class MessageStreamIteratorTest {

    [Test]
    public void ThreeMessagesInMemory() {
      MemoryStream stream = new MemoryStream(MessageStreamWriterTest.ThreeMessageData);      
      IEnumerable<NestedMessage> iterator = MessageStreamIterator.FromStreamProvider<NestedMessage>(() => stream);
      List<NestedMessage> messages = new List<NestedMessage>(iterator);

      Assert.AreEqual(3, messages.Count);
      Assert.AreEqual(5, messages[0].Bb);
      Assert.AreEqual(1500, messages[1].Bb);
      Assert.IsFalse(messages[2].HasBb);
    }
  }
}
