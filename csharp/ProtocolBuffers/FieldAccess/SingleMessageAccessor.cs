using System;
using System.Reflection;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {
  /// <summary>
  /// Accessor for fields representing a non-repeated message value.
  /// </summary>
  internal sealed class SingleMessageAccessor : SinglePrimitiveAccessor {

    /// <summary>
    /// The static method to create a builder for the property type. For example,
    /// in a message type "Foo", a field called "bar" might be of type "Baz". This
    /// method is Baz.CreateBuilder.
    /// </summary>
    private readonly MethodInfo createBuilderMethod;


    internal SingleMessageAccessor(string name, Type messageType, Type builderType) 
        : base(name, messageType, builderType) {
      
      createBuilderMethod = ClrType.GetMethod("CreateBuilder", new Type[0]);//BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly);
      if (createBuilderMethod == null) {
        throw new ArgumentException("No public static CreateBuilder method declared in " + ClrType.Name);
      }
    }

    /// <summary>
    /// Creates a message of the appropriate CLR type from the given value,
    /// which may already be of the right type or may be a dynamic message.
    /// </summary>
    private object CoerceType(object value) {

      // If it's already of the right type, we're done
      if (ClrType.IsInstanceOfType(value)) {
        return value;
      }
      
      // No... so let's create a builder of the right type, and merge the value in.
      IMessage message = (IMessage) value;
      return CreateBuilder().MergeFrom(message).Build();
    }

    public override void SetValue(IBuilder builder, object value) {
      base.SetValue(builder, CoerceType(value));
    }

    public override IBuilder CreateBuilder() {
      return (IBuilder) createBuilderMethod.Invoke(null, null);
    }
  }
}