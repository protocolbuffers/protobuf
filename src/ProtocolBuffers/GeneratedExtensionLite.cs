using System;

namespace Google.ProtocolBuffers {

  public interface IGeneratedExtensionLite {
    int Number { get; }
    object ContainingType { get; }
    IMessageLite MessageDefaultInstance { get; }
  }

  public class GeneratedExtensionLite : IGeneratedExtensionLite {
    public int Number {
      get { throw new NotImplementedException(); }
    }

    public object ContainingType {
      get { throw new NotImplementedException(); }
    }

    public IMessageLite MessageDefaultInstance {
      get { throw new NotImplementedException(); }
    }
  }
}