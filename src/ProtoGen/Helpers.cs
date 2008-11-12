using System;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;
namespace Google.ProtocolBuffers.ProtoGen {

  /// <summary>
  /// Helpers to resolve class names etc.
  /// </summary>
  internal static class Helpers {
    internal static void WriteNamespaces(TextGenerator writer) {
      writer.WriteLine("using pb = global::Google.ProtocolBuffers;");
      writer.WriteLine("using pbc = global::Google.ProtocolBuffers.Collections;");
      writer.WriteLine("using pbd = global::Google.ProtocolBuffers.Descriptors;");
      writer.WriteLine("using scg = global::System.Collections.Generic;");
    }
  }
}
