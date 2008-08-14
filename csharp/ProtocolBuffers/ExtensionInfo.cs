using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
  public sealed class ExtensionInfo {
    /// <summary>
    /// The extension's descriptor
    /// </summary>
    public FieldDescriptor Descriptor { get; private set; }

    /// <summary>
    /// A default instance of the extensions's type, if it has a message type,
    /// or null otherwise.
    /// </summary>
    public IMessage DefaultInstance { get; private set; }

    internal ExtensionInfo(FieldDescriptor descriptor) : this(descriptor, null) {
    }

    internal ExtensionInfo(FieldDescriptor descriptor, IMessage defaultInstance) {
      Descriptor = descriptor;
      DefaultInstance = defaultInstance;
    }
  }
}