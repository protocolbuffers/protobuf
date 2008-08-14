using System;
using System.Reflection;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {

  /// <summary>
  /// Accessor for a repeated message field.
  /// 
  /// TODO(jonskeet): Try to extract the commonality between this and SingleMessageAccessor.
  /// We almost want multiple inheritance...
  /// </summary>
  internal sealed class RepeatedMessageAccessor : RepeatedPrimitiveAccessor {

    /// <summary>
    /// The static method to create a builder for the property type. For example,
    /// in a message type "Foo", a field called "bar" might be of type "Baz". This
    /// method is Baz.CreateBuilder.
    /// </summary>
    private readonly MethodInfo createBuilderMethod;

    internal RepeatedMessageAccessor(string name, Type messageType, Type builderType)
        : base(name, messageType, builderType) {
      createBuilderMethod = ClrType.GetMethod("CreateBuilder", BindingFlags.Public | BindingFlags.Static);
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

    public override void SetRepeated(IBuilder builder, int index, object value) {
      base.SetRepeated(builder, index, CoerceType(value));
    }

    public override IBuilder CreateBuilder() {
      return (IBuilder) createBuilderMethod.Invoke(null, null);
    }

    public override void AddRepeated(IBuilder builder, object value) {
      base.AddRepeated(builder, CoerceType(value));
    }
  }
}
