using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// Defined specifically for the <see cref="FieldType" /> enumeration,
  /// this allows each field type to specify the mapped type and wire type.
  /// </summary>
  [AttributeUsage(AttributeTargets.Field)]
  internal class FieldMappingAttribute : Attribute {
    internal FieldMappingAttribute(MappedType mappedType, WireFormat.WireType wireType) {
      MappedType = mappedType;
      WireType = wireType;
    }

    internal MappedType MappedType { get; private set; }
    internal WireFormat.WireType WireType { get; private set; }
  }
}
