using System;

namespace Google.ProtocolBuffers {

  public interface IGeneratedExtensionLite {
    int Number { get; }
    object ContainingType { get; }
    IMessageLite MessageDefaultInstance { get; }
  }
}