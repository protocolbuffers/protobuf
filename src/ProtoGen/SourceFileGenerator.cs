using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  /// <summary>
  /// Generator to hold a TextGenerator, generate namespace aliases etc.
  /// Each source file created uses one of these, and it can be used to create
  /// multiple classes within the same file.
  /// </summary>
  internal class SourceFileGenerator  {

    private readonly TextGenerator output;

    private SourceFileGenerator(TextWriter writer) {
      output = new TextGenerator(writer);
    }

    /// <summary>
    /// Creates a ClassFileGenerator for the given writer, which will be closed
    /// when the instance is disposed. The specified namespace is created, if it's non-null.
    /// </summary>
    internal static SourceFileGenerator ForWriter(TextWriter writer) {
      return new SourceFileGenerator(writer);
    }
  }
}
