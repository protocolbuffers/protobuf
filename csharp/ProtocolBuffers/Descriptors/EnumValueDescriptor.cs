using System;

namespace Google.ProtocolBuffers.Descriptors {
  public class EnumValueDescriptor {
    private EnumDescriptor enumDescriptor;

    public int Number {
      get { throw new NotImplementedException(); }
    }

    public EnumDescriptor EnumDescriptor {
      get { return enumDescriptor; }
    }
  }
}
