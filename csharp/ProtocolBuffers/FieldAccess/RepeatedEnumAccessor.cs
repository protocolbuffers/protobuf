using System;
using System.Collections;
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {

  /// <summary>
  /// Accessor for a repeated enum field.
  /// </summary>
  internal sealed class RepeatedEnumAccessor : RepeatedPrimitiveAccessor {

    private readonly EnumDescriptor enumDescriptor;

    internal RepeatedEnumAccessor(FieldDescriptor field, string name, Type messageType, Type builderType)
        : base(name, messageType, builderType) {

      enumDescriptor = field.EnumType;
    }

    public override object GetValue(IMessage message) {
      List<EnumValueDescriptor> ret = new List<EnumValueDescriptor>();
      foreach (int rawValue in (IEnumerable) base.GetValue(message)) {
        ret.Add(enumDescriptor.FindValueByNumber(rawValue));
      }
      return Lists.AsReadOnly(ret);
    }

    public override object GetRepeatedValue(IMessage message, int index) {
      // Note: This relies on the fact that the CLR allows unboxing from an enum to
      // its underlying value
      int rawValue = (int) base.GetRepeatedValue(message, index);
      return enumDescriptor.FindValueByNumber(rawValue);
    }

    public override void AddRepeated(IBuilder builder, object value) {
      base.AddRepeated(builder, ((EnumValueDescriptor) value).Number);
    }

    public override void SetRepeated(IBuilder builder, int index, object value) {
      base.SetRepeated(builder, index, ((EnumValueDescriptor) value).Number);
    }
  }
}
