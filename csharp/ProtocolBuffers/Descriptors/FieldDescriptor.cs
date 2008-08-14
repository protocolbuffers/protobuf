
using System;

namespace Google.ProtocolBuffers.Descriptors {
  public class FieldDescriptor {

    private EnumDescriptor enumType;

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

    /// <summary>
    /// For enum fields, returns the field's type.
    /// </summary>
    public EnumDescriptor EnumType {
      get {
        if (MappedType != MappedType.Enum) {
          throw new InvalidOperationException("EnumType is only valid for enum fields.");
        }
        return enumType;
      }
    }   

    /// <summary>
    /// The default value for this field. For repeated fields
    /// this will always be an empty list. For message fields it will
    /// always be null. For singular values, it will depend on the descriptor.
    /// </summary>
    public object DefaultValue
    {
      get { throw new NotImplementedException(); }
    }

    public string Name
    {
      get { throw new NotImplementedException(); }
    }
  }
}
