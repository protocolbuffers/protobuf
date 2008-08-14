
using System.Collections.Generic;

namespace Google.ProtocolBuffers.Descriptors {
  public class MessageDescriptor {
    public IList<FieldDescriptor> Fields;
    public DescriptorProtos.MessageOptions Options;
    public string FullName;
  }
}
