using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {
  /// <summary>
  /// Accessor for fields representing a non-repeated enum value.
  /// </summary>
  internal sealed class SingleEnumAccessor : SinglePrimitiveAccessor {

    private readonly EnumDescriptor enumDescriptor;

    internal SingleEnumAccessor(FieldDescriptor field, string name, Type messageType, Type builderType) 
        : base(name, messageType, builderType) {
      enumDescriptor = field.EnumType;
    }

    /// <summary>
    /// Returns an EnumValueDescriptor representing the value in the builder.
    /// Note that if an enum has multiple values for the same number, the descriptor
    /// for the first value with that number will be returned.
    /// </summary>
    public override object GetValue(IMessage message) {
      // Note: This relies on the fact that the CLR allows unboxing from an enum to
      // its underlying value
      int rawValue = (int) base.GetValue(message);
      return enumDescriptor.FindValueByNumber(rawValue);
    }

    /// <summary>
    /// Sets the value as an enum (via an int) in the builder,
    /// from an EnumValueDescriptor parameter.
    /// </summary>
    public override void SetValue(IBuilder builder, object value) {
      EnumValueDescriptor valueDescriptor = (EnumValueDescriptor) value;
      base.SetValue(builder, valueDescriptor.Number);
    }
  }
}