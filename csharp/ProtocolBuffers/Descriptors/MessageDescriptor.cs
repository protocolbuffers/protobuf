
using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {
  public class MessageDescriptor : DescriptorBase<DescriptorProto, MessageOptions> {
    public IList<FieldDescriptor> Fields;

    internal MessageDescriptor(DescriptorProto proto, FileDescriptor file) : base(proto, file) {
    }

    internal bool IsExtensionNumber(int fieldNumber) {
      throw new System.NotImplementedException();
    }

    internal FieldDescriptor FindFieldByNumber(int fieldNumber) {
      throw new System.NotImplementedException();
    }

    /// <value>
    /// List of message types nested within this one.
    /// </value>
    public IList<MessageDescriptor> NestedTypes {
      get { throw new System.NotImplementedException(); }
    }
  }
}
