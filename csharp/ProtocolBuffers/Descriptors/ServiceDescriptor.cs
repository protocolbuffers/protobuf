using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {
  public class ServiceDescriptor : IndexedDescriptorBase<ServiceDescriptorProto, ServiceOptions> {
    public ServiceDescriptor(ServiceDescriptorProto proto, FileDescriptor file, int index)
        : base(proto, file, index) {
    }
  }
}
