
using System.Collections.Generic;

namespace Google.ProtocolBuffers.Descriptors {
  public class MessageDescriptor {
    public IList<FieldDescriptor> Fields;
    public DescriptorProtos.MessageOptions Options;
    public string FullName;

    internal bool IsExtensionNumber(int fieldNumber) {
      throw new System.NotImplementedException();
    }

    internal FieldDescriptor FindFieldByNumber(int fieldNumber) {
      throw new System.NotImplementedException();
    }
  }
}
