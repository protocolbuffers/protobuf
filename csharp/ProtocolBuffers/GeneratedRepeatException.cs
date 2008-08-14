using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;
using System.Collections;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Class used to represent repeat extensions in generated classes.
  /// </summary>
  public class GeneratedRepeatExtension<TExtensionElement> : GeneratedExtensionBase<IList<TExtensionElement>> {
    private GeneratedRepeatExtension(FieldDescriptor field) : base(field, typeof(TExtensionElement)) {
    }

    public static GeneratedExtensionBase<IList<TExtensionElement>> CreateInstance(FieldDescriptor descriptor) {
      if (!descriptor.IsRepeated) {
        throw new ArgumentException("Must call GeneratedRepeatExtension.CreateInstance() for repeated types.");
      }
      return new GeneratedRepeatExtension<TExtensionElement>(descriptor);
    }

    /// <summary>
    /// Converts the list to the right type.
    /// TODO(jonskeet): Check where this is used, and whether we need to convert
    /// for primitive types.
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    public override object FromReflectionType(object value) {
      if (Descriptor.MappedType == MappedType.Message ||
          Descriptor.MappedType == MappedType.Enum) {
        // Must convert the whole list.
        List<TExtensionElement> result = new List<TExtensionElement>();
        foreach (object element in (IEnumerable) value) {
          ((IList) result).Add(SingularFromReflectionType(element));
        }
        return result;
      } else {
        return value;
      }
    }
  }
}
