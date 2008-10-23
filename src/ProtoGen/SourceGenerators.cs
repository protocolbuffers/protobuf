using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal static class SourceGenerators {

    private static readonly Dictionary<Type, Func<IDescriptor, ISourceGenerator>> GeneratorFactories = new Dictionary<Type, Func<IDescriptor, ISourceGenerator>> {
      { typeof(FileDescriptor), descriptor => new UmbrellaClassGenerator((FileDescriptor) descriptor) },
      { typeof(EnumDescriptor), descriptor => new EnumGenerator((EnumDescriptor) descriptor) },
      { typeof(ServiceDescriptor), descriptor => new ServiceGenerator((ServiceDescriptor) descriptor) },
      { typeof(MessageDescriptor), descriptor => new MessageGenerator((MessageDescriptor) descriptor) },
      // For other fields, we have IFieldSourceGenerators.
      { typeof(FieldDescriptor), descriptor => new ExtensionGenerator((FieldDescriptor) descriptor) }
    };

    public static IFieldSourceGenerator CreateFieldGenerator(FieldDescriptor field) {
      switch (field.MappedType) {
        case MappedType.Message :
          return field.IsRepeated 
              ? (IFieldSourceGenerator) new RepeatedMessageFieldGenerator(field)
              : new MessageFieldGenerator(field);
        case MappedType.Enum:
          return field.IsRepeated
              ? (IFieldSourceGenerator)new RepeatedEnumFieldGenerator(field)
              : new EnumFieldGenerator(field);
        default:
          return field.IsRepeated
              ? (IFieldSourceGenerator)new RepeatedPrimitiveFieldGenerator(field)
              : new PrimitiveFieldGenerator(field);
      }
    }

    public static ISourceGenerator CreateGenerator<T>(T descriptor) where T : IDescriptor {
      Func<IDescriptor, ISourceGenerator> factory;
      if (!GeneratorFactories.TryGetValue(typeof(T), out factory)) {
        throw new ArgumentException("No generator registered for " + typeof(T).Name);
      }
      return factory(descriptor);
    }
  }
}
