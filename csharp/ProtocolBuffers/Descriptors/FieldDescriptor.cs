
namespace Google.ProtocolBuffers.Descriptors {
  public class FieldDescriptor {

    public bool IsRequired {
      get;
      set;
    }

    public MappedType MappedType { get; set; }

    public bool IsRepeated { get; set; }

    public FieldType FieldType { get; set; }
    public int FieldNumber { get; set; }

    public bool IsExtension { get; set; }

    public MessageDescriptor ContainingType { get; set; }

    public string FullName { get; set; }

    public bool IsOptional { get; set; }

    public MessageDescriptor MessageType { get; set; }

    public MessageDescriptor ExtensionScope { get; set; }
  }
}
