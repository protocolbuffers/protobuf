using System;

namespace Google.ProtocolBuffers.Descriptors {
  public class EnumValueDescriptor {

    internal EnumValueDescriptor(DescriptorProtos.EnumValueDescriptorProto proto,
                                 FileDescriptor file,
                                 EnumDescriptor parent,
                                 int index) {
      enumDescriptor = parent;
    }

    private EnumDescriptor enumDescriptor;

    public int Number {
      get { throw new NotImplementedException(); }
    }

    public EnumDescriptor EnumDescriptor {
      get { return enumDescriptor; }
    }
  }
}
