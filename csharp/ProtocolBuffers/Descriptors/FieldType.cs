
namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Enumeration of all the possible field types. The odd formatting is to make it very clear
  /// which attribute applies to which value, while maintaining a compact format.
  /// </summary>
  public enum FieldType {
    [FieldMapping(MappedType.Double, WireFormat.WireType.Fixed64)] Double,
    [FieldMapping(MappedType.Single, WireFormat.WireType.Fixed32)] Float,
    [FieldMapping(MappedType.Int64, WireFormat.WireType.Varint)] Int64,
    [FieldMapping(MappedType.UInt64, WireFormat.WireType.Varint)] UInt64,
    [FieldMapping(MappedType.Int32, WireFormat.WireType.Varint)] Int32,
    [FieldMapping(MappedType.UInt64, WireFormat.WireType.Fixed64)] Fixed64,
    [FieldMapping(MappedType.UInt32, WireFormat.WireType.Fixed32)] Fixed32,
    [FieldMapping(MappedType.Boolean, WireFormat.WireType.Varint)] Bool,
    [FieldMapping(MappedType.String, WireFormat.WireType.LengthDelimited)] String,
    [FieldMapping(MappedType.Message, WireFormat.WireType.StartGroup)] Group,
    [FieldMapping(MappedType.Message, WireFormat.WireType.LengthDelimited)] Message,
    [FieldMapping(MappedType.ByteString, WireFormat.WireType.LengthDelimited)] Bytes,
    [FieldMapping(MappedType.UInt32, WireFormat.WireType.Varint)] UInt32,
    [FieldMapping(MappedType.Int32, WireFormat.WireType.Fixed32)] SFixed32,
    [FieldMapping(MappedType.Int64, WireFormat.WireType.Fixed64)] SFixed64,
    [FieldMapping(MappedType.Int32, WireFormat.WireType.Varint)] SInt32,
    [FieldMapping(MappedType.Int64, WireFormat.WireType.Varint)] SInt64,
    [FieldMapping(MappedType.Enum, WireFormat.WireType.Varint)] Enum
  }
}
