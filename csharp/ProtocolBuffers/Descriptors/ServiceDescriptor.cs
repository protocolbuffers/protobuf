using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// Describes a service type.
  /// </summary>
  public class ServiceDescriptor : IndexedDescriptorBase<ServiceDescriptorProto, ServiceOptions> {

    private readonly IList<MethodDescriptor> methods;

    public ServiceDescriptor(ServiceDescriptorProto proto, FileDescriptor file, int index)
        : base(proto, file, ComputeFullName(file, null, proto.Name), index) {

      methods = DescriptorUtil.ConvertAndMakeReadOnly(proto.MethodList,
        (method, i) => new MethodDescriptor(method, file, this, i));

      file.DescriptorPool.AddSymbol(this);
    }

    /// <value>
    /// An unmodifiable list of methods in this service.
    /// </value>
    public IList<MethodDescriptor> Methods {
      get { return methods; }
    }

    internal void CrossLink() {
      foreach (MethodDescriptor method in methods) {
        method.CrossLink();
      }
    }
  }
}
