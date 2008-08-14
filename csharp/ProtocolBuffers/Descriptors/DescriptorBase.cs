using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Base class for all descriptors, providing common functionality.
  /// </summary>
  /// <typeparam name="TProto">Type of the protocol buffer form of this descriptor</typeparam>
  /// <typeparam name="TOptions">Type of the options protocol buffer for this descriptor</typeparam>
  public abstract class DescriptorBase<TProto, TOptions> where TProto : IMessage<TProto>, IDescriptorProto<TOptions> {

    private readonly TProto proto;
    private readonly FileDescriptor file;

    protected DescriptorBase(TProto proto, FileDescriptor file) {
      this.proto = proto;
      this.file = file;
    }

    /// <summary>
    /// Returns the protocol buffer form of this descriptor
    /// </summary>
    public TProto Proto {
      get { return proto; }
    }

    public TOptions Options {
      get { return proto.Options; }
    }

    /// <summary>
    /// The fully qualified name of the descriptor's target.
    /// TODO(jonskeet): Implement!
    /// </summary>
    public string FullName {
      get { return null; }
    }

    /// <summary>
    /// The brief name of the descriptor's target.
    /// </summary>
    public string Name {
      get { return proto.Name; }
    }

    /// <value>
    /// The file this descriptor was declared in.
    /// </value>
    public FileDescriptor File {
      get { return file; }
    }
  }
}
