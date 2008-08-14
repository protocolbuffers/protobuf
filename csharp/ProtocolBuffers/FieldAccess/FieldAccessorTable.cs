using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {
  /// <summary>
  /// Provides access to fields in generated messages via reflection.
  /// This type is public to allow it to be used by generated messages, which
  /// create appropriate instances in the .proto file description class.
  /// TODO(jonskeet): See if we can hide it somewhere...
  /// </summary>
  public class FieldAccessorTable {

    readonly IFieldAccessor[] accessors;

    readonly MessageDescriptor descriptor;

    public MessageDescriptor Descriptor { 
      get { return descriptor; }
    }

   /// <summary>
    /// Constructs a FieldAccessorTable for a particular message class.
    /// Only one FieldAccessorTable should be constructed per class.
    /// </summary>
    /// <param name="descriptor">The type's descriptor</param>
    /// <param name="propertyNames">The Pascal-case names of all the field-based properties in the message.</param>
    /// <param name="messageType">The .NET type representing the message</param>
    /// <param name="builderType">The .NET type representing the message's builder type</param>
    public FieldAccessorTable(MessageDescriptor descriptor, String[] propertyNames, Type messageType, Type builderType) {
      this.descriptor = descriptor;
      accessors = new IFieldAccessor[descriptor.Fields.Count];
      for (int i=0; i < accessors.Length; i++) {
        accessors[i] = CreateAccessor(descriptor.Fields[i], propertyNames[i], messageType, builderType);
      }
    }

     /// <summary>
     /// Creates an accessor for a single field
     /// </summary>   
    private static IFieldAccessor CreateAccessor(FieldDescriptor field, string name, Type messageType, Type builderType) {
      if (field.IsRepeated) {
        switch (field.MappedType) {
          case MappedType.Message: return new RepeatedMessageAccessor(name, messageType, builderType);
          case MappedType.Enum: return new RepeatedEnumAccessor(field, name, messageType, builderType);
          default: return new RepeatedPrimitiveAccessor(name, messageType, builderType);
        }
      } else {
        switch (field.MappedType) {
          case MappedType.Message: return new SingleMessageAccessor(name, messageType, builderType);
          case MappedType.Enum: return new SingleEnumAccessor(field, name, messageType, builderType);
          default: return new SinglePrimitiveAccessor(name, messageType, builderType);
        }
      }
    }

    internal IFieldAccessor this[FieldDescriptor field] {
      get {
        if (field.ContainingType != descriptor) {
          throw new ArgumentException("FieldDescriptor does not match message type.");
        } else if (field.IsExtension) {
          // If this type had extensions, it would subclass ExtendableMessage,
          // which overrides the reflection interface to handle extensions.
          throw new ArgumentException("This type does not have extensions.");
        }
        return accessors[field.Index];
      }
    }
  }
}
