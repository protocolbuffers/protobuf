using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Generated extension for a singular field.
  /// </remarks>
  public class GeneratedSingleExtension<TExtension> : GeneratedExtensionBase<TExtension>  {
    internal GeneratedSingleExtension(FieldDescriptor descriptor) : base(descriptor, typeof(TExtension)) {
    }

    public static GeneratedSingleExtension<TExtension> CreateInstance(FieldDescriptor descriptor) {
      if (descriptor.IsRepeated) {
        throw new ArgumentException("Must call GeneratedRepeateExtension.CreateInstance() for repeated types.");
      }
      return new GeneratedSingleExtension<TExtension>(descriptor);
    }

    public override object FromReflectionType(object value) {
      return base.SingularFromReflectionType(value);
    }
  }
}
