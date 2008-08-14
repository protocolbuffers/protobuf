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
   
    /// <summary>
    /// Finds a method by name.
    /// </summary>
    /// <param name="name">The unqualified name of the method (e.g. "Foo").</param>
    /// <returns>The method's decsriptor, or null if not found.</returns>
    public MethodDescriptor FindMethodByName(String name) {
      return File.DescriptorPool.FindSymbol<MethodDescriptor>(FullName + "." + name);
    }

    internal void CrossLink() {
      foreach (MethodDescriptor method in methods) {
        method.CrossLink();
      }
    }
  }
}
