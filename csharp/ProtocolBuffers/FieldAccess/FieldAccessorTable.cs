using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {
  public class FieldAccessorTable<TMessage, TBuilder> 
      where TMessage : IMessage<TMessage> 
      where TBuilder : IBuilder<TMessage> {

    readonly MessageDescriptor descriptor;

    public MessageDescriptor Descriptor { 
      get { return descriptor; }
    }

    public FieldAccessorTable(MessageDescriptor descriptor, String[] pascalCaseNames) {
      this.descriptor = descriptor;
    }

    internal IFieldAccessor<TMessage, TBuilder> this[FieldDescriptor field] {
      get { return null; }
    }
  }
}
