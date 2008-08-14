using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Base class for descriptors which are also indexed. This is all of them other than
  /// <see cref="FileDescriptor" />.
  /// </summary>
  public abstract class IndexedDescriptorBase<TProto, TOptions> : DescriptorBase<TProto, TOptions>
      where TProto : IMessage<TProto>, IDescriptorProto<TOptions> {

    private readonly int index;

    protected IndexedDescriptorBase(TProto proto, FileDescriptor file, string fullName, int index)
        : base(proto, file, fullName) {
      this.index = index;
    }

    /// <value>
    /// The index of this descriptor within its parent descriptor. 
    /// </value>
    /// <remarks>
    /// This returns the index of this descriptor within its parent, for
    /// this descriptor's type. (There can be duplicate values for different
    /// types, e.g. one enum type with index 0 and one message type with index 0.)
    /// </remarks>
    public int Index {
      get { return index; }
    }
  }
}
