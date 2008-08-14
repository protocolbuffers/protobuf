using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Generated extension for a singular field.
  /// </remarks>
  public class GeneratedSingleExtension<TContainer, TExtension> : GeneratedExtensionBase<TContainer, TExtension>    
      where TContainer : IMessage<TContainer> {

    internal GeneratedSingleExtension(FieldDescriptor descriptor) : base(descriptor) {
    }

    public static GeneratedSingleExtension<TContainer, TExtension> CreateInstance(FieldDescriptor descriptor) {
      if (descriptor.IsRepeated) {
        throw new ArgumentException("Must call GeneratedRepeateExtension.CreateInstance() for repeated types.");
      }
      return new GeneratedSingleExtension<TContainer, TExtension>(descriptor);
    }

    public override object FromReflectionType(object value) {
      return base.SingularFromReflectionType(value);
    }
  }
}
