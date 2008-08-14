using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Base class for nearly all descriptors, providing common functionality.
  /// </summary>
  /// <typeparam name="TProto">Type of the protocol buffer form of this descriptor</typeparam>
  /// <typeparam name="TOptions">Type of the options protocol buffer for this descriptor</typeparam>
  public abstract class DescriptorBase<TProto, TOptions> : IDescriptor<TProto>
      where TProto : IMessage, IDescriptorProto<TOptions> {

    private readonly TProto proto;
    private readonly FileDescriptor file;
    private readonly string fullName;

    protected DescriptorBase(TProto proto, FileDescriptor file, string fullName) {
      this.proto = proto;
      this.file = file;
      this.fullName = fullName;
    }

    protected static string ComputeFullName(FileDescriptor file, MessageDescriptor parent, string name) {
      if (parent != null) {
        return parent.FullName + "." + name;
      }
      if (file.Package.Length > 0) {
        return file.Package + "." + name;
      }
      return name;
    }

    IMessage IDescriptor.Proto {
      get { return proto; }
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
    /// </summary>
    public string FullName {
      get { return fullName; }
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
