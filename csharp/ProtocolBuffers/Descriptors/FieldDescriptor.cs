
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
  }
}
