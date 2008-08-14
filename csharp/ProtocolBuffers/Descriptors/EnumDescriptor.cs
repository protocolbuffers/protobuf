
using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;
namespace Google.ProtocolBuffers.Descriptors {

  public class EnumDescriptor : IndexedDescriptorBase<EnumDescriptorProto, EnumOptions> {

    private readonly MessageDescriptor containingType;
    private readonly IList<EnumValueDescriptor> values;

    internal EnumDescriptor(EnumDescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int index)
        : base(proto, file, ComputeFullName(file, parent, proto.Name), index) {
      containingType = parent;

      if (proto.ValueCount == 0) {
        // We cannot allow enums with no values because this would mean there
        // would be no valid default value for fields of this type.
        throw new DescriptorValidationException(this, "Enums must contain at least one value.");
      }
      
      values = DescriptorUtil.ConvertAndMakeReadOnly(proto.ValueList,
          (value, i) => new EnumValueDescriptor(value, file, this, i));

      File.DescriptorPool.AddSymbol(this);
    }

    /// <value>
    /// If this is a nested type, get the outer descriptor, otherwise null.
    /// </value>
    public MessageDescriptor ContainingType {
      get { return containingType; }
    }

    /// <value>
    /// An unmodifiable list of defined value descriptors for this enum.
    /// </value>
    public IList<EnumValueDescriptor> Values {
      get { return values; }
    }

    /// <summary>
    /// Finds an enum value by number. If multiple enum values have the
    /// same number, this returns the first defined value with that number.
    /// </summary>
    // TODO(jonskeet): Make internal and use InternalsVisibleTo?
    public EnumValueDescriptor FindValueByNumber(int number) {
      return File.DescriptorPool.FindEnumValueByNumber(this, number);
    }

    /// <summary>
    /// Finds an enum value by name.
    /// </summary>
    /// <param name="name">The unqualified name of the value (e.g. "FOO").</param>
    /// <returns>The value's descriptor, or null if not found.</returns>
    // TODO(jonskeet): Make internal and use InternalsVisibleTo?
    public EnumValueDescriptor FindValueByName(string name) {
      return File.DescriptorPool.FindSymbol<EnumValueDescriptor>(FullName + "." + name);
    }
  }
}
