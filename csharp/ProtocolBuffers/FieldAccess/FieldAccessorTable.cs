using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {
  public class FieldAccessorTable {

    readonly MessageDescriptor descriptor;

    public MessageDescriptor Descriptor { 
      get { return descriptor; }
    }

    public FieldAccessorTable(MessageDescriptor descriptor, String[] pascalCaseFieldNames, Type messageType, Type builderType) {
      this.descriptor = descriptor;
    }

    internal IFieldAccessor this[FieldDescriptor field] {
      get { return null; }
    }
  }
}
