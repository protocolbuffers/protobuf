using System;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {
  public class EnumValueDescriptor : IndexedDescriptorBase<EnumValueDescriptorProto, EnumValueOptions> {

    private readonly EnumDescriptor enumDescriptor;

    internal EnumValueDescriptor(EnumValueDescriptorProto proto,
                                 FileDescriptor file,
                                 EnumDescriptor parent,
                                 int index) : base (proto, file, index) {
      enumDescriptor = parent;
    }

    public int Number {
      get { throw new NotImplementedException(); }
    }

    public EnumDescriptor EnumDescriptor {
      get { return enumDescriptor; }
    }
  }
}
