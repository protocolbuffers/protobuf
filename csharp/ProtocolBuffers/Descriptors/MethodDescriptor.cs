using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Describes a single method in a service.
  /// </summary>
  public class MethodDescriptor : IndexedDescriptorBase<MethodDescriptorProto, MethodOptions> {

    private readonly ServiceDescriptor service;
    private MessageDescriptor inputType;
    private MessageDescriptor outputType;

    /// <value>
    /// The service this method belongs to.
    /// </value>
    public ServiceDescriptor Service {
      get { return service; }
    }

    /// <value>
    /// The method's input type.
    /// </value>
    public MessageDescriptor InputType {
      get { return inputType; }
    }

    /// <value>
    /// The method's input type.
    /// </value>
    public MessageDescriptor OutputType {
      get { return outputType; }
    }

    internal MethodDescriptor(MethodDescriptorProto proto, FileDescriptor file,
        ServiceDescriptor parent, int index) 
        : base(proto, file, parent.FullName + "." + proto.Name, index) {
      service = parent;
      file.DescriptorPool.AddSymbol(this);
    }

    internal void CrossLink() {
      IDescriptor lookup = File.DescriptorPool.LookupSymbol(Proto.InputType, this);
      if (!(lookup is MessageDescriptor)) {
        throw new DescriptorValidationException(this, "\"" + Proto.InputType + "\" is not a message type.");
      }
      inputType = (MessageDescriptor) lookup;

      lookup = File.DescriptorPool.LookupSymbol(Proto.OutputType, this);
      if (!(lookup is MessageDescriptor)) {
        throw new DescriptorValidationException(this, "\"" + Proto.OutputType + "\" is not a message type.");
      }
      outputType = (MessageDescriptor) lookup;
    }
  }
}
