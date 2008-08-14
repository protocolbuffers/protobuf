using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// The non-generic form of the IDescriptor interface. Useful for describing a general
  /// descriptor.
  /// </summary>
  public interface IDescriptor {
    string Name { get; }
    string FullName { get; }
    FileDescriptor File { get; }
    IMessage Proto { get; }
  }

  /// <summary>
  /// Strongly-typed form of the IDescriptor interface.
  /// </summary>
  /// <typeparam name="TProto">Protocol buffer type underlying this descriptor type</typeparam>
  public interface IDescriptor<TProto> : IDescriptor where TProto : IMessage {
    new TProto Proto { get; }
  }
}
