using System;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {
  public class EnumValueDescriptor : IndexedDescriptorBase<EnumValueDescriptorProto, EnumValueOptions> {

    private readonly EnumDescriptor enumDescriptor;

    internal EnumValueDescriptor(EnumValueDescriptorProto proto, FileDescriptor file,
        EnumDescriptor parent, int index) 
        : base (proto, file, parent.FullName + "." + proto.Name, index) {
      enumDescriptor = parent;
      file.DescriptorPool.AddSymbol(this);
      file.DescriptorPool.AddEnumValueByNumber(this);
    }

    public int Number {
      get { return Proto.Number; }
    }

    public EnumDescriptor EnumDescriptor {
      get { return enumDescriptor; }
    }
  }
}
