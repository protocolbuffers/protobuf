using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Writes multiple messages to the same stream. Each message is written
  /// as if it were an element of a repeated field 1 in a larger protocol buffer.
  /// This class takes no ownership of the stream it is given; it never closes the
  /// stream.
  /// </summary>
  public sealed class MessageStreamWriter<T> where T : IMessage<T> {

    private readonly CodedOutputStream codedOutput;

    /// <summary>
    /// Creates an instance which writes to the given stream.
    /// </summary>
    /// <param name="output">Stream to write messages to.</param>
    public MessageStreamWriter(Stream output) {
      codedOutput = CodedOutputStream.CreateInstance(output);
    }

    public void Write(T message) {
      codedOutput.WriteMessage(1, message);
    }

    public void Flush() {
      codedOutput.Flush();
    }
  }
}
