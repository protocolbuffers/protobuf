using Google.ProtocolBuffers.Descriptors;
using System;
using System.Collections.Generic;
using System.Reflection;

namespace Google.ProtocolBuffers {

  public static class GeneratedExtension {

    public static GeneratedExtension<TContainer, TExtension> CreateExtension<TContainer, TExtension>(FieldDescriptor descriptor)
        where TContainer : IMessage<TContainer> {
      if (descriptor.IsRepeated) {
        throw new ArgumentException("Must call CreateRepeatedGeneratedExtension() for repeated types.");
      }
      return new GeneratedExtension<TContainer, TExtension>(descriptor);
    }

    public static GeneratedExtension<TContainer, IList<TExtension>> CreateRepeatedExtension<TContainer, TExtension>(FieldDescriptor descriptor) 
        where TContainer : IMessage<TContainer> {
      if (descriptor.IsRepeated) {
        throw new ArgumentException("Must call CreateRepeatedGeneratedExtension() for repeated types.");
      }
      return new GeneratedExtension<TContainer, IList<TExtension>>(descriptor);
    }
  }

  /// <summary>
  /// Base class for all generated extensions.
  /// </summary>
  /// <remarks>
  /// The protocol compiler generates a static singleton instance of this
  /// class for each extension. For exmaple, imagine a .proto file with:
  /// <code>
  /// message Foo {
  ///   extensions 1000 to max
  /// }
  /// 
  /// extend Foo {
  ///   optional int32 bar;
  /// }
  /// </code>
  /// Then MyProto.Foo.Bar has type GeneratedExtension&lt;MyProto.Foo,int&gt;.
  /// <para />
  /// In general, users should ignore the details of this type, and
  /// simply use the static singletons as parmaeters to the extension accessors
  /// in ExtendableMessage and ExtendableBuilder.
  /// </remarks>
  public class GeneratedExtension<TContainer, TExtension> where TContainer : IMessage<TContainer> {
    private readonly IMessage messageDefaultInstance;
    private readonly FieldDescriptor descriptor;

    internal GeneratedExtension(FieldDescriptor descriptor) {
      if (!descriptor.IsExtension) {
        throw new ArgumentException("GeneratedExtension given a regular (non-extension) field.");
      }

      this.descriptor = descriptor;

      switch (descriptor.MappedType) {
        case MappedType.Message:
          PropertyInfo defaultInstanceProperty = typeof(TExtension)
              .GetProperty("DefaultInstance", BindingFlags.Static | BindingFlags.Public);
          if (defaultInstanceProperty == null) {
            throw new ArgumentException("No public static DefaultInstance property for type " + typeof(TExtension).Name);
          }
          messageDefaultInstance = (IMessage) defaultInstanceProperty.GetValue(null, null);
          break;
        case MappedType.Enum:
          // FIXME(jonskeet): May not need this
          //enumValueOf = getMethodOrDie(type, "valueOf",
            //                           EnumValueDescriptor.class);
          //enumGetValueDescriptor = getMethodOrDie(type, "getValueDescriptor");
          messageDefaultInstance = null;
          break;
        default:
          messageDefaultInstance = null;
          break;
      }
    }

    public FieldDescriptor Descriptor {
      get { return descriptor; }
    }

    public IMessage MessageDefaultInstance {
      get { return messageDefaultInstance; }
    }

    internal object SingularFromReflectionType(object p) {
      throw new System.NotImplementedException();
    }

    internal object FromReflectionType(object value) {
      throw new System.NotImplementedException();
    }
  }
}
