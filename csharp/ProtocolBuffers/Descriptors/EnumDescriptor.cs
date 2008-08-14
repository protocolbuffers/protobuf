
using Google.ProtocolBuffers.DescriptorProtos;
namespace Google.ProtocolBuffers.Descriptors {
  public class EnumDescriptor : IndexedDescriptorBase<EnumDescriptorProto, EnumOptions> {
    internal EnumDescriptor(EnumDescriptorProto proto, EnumOptions options, FileDescriptor file, int index)
        : base(proto, file, index) {
    }

    internal EnumValueDescriptor FindValueByNumber(int rawValue) {
      throw new System.NotImplementedException();
    }
  }
}
