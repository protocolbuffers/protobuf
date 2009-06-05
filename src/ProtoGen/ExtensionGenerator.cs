using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal class ExtensionGenerator : SourceGeneratorBase<FieldDescriptor>, ISourceGenerator {

    private readonly string scope;
    private readonly string type;
    private readonly string name;

    internal ExtensionGenerator(FieldDescriptor descriptor) : base(descriptor) {
      if (Descriptor.ExtensionScope != null) {
        scope = GetClassName(Descriptor.ExtensionScope);
      } else {
        scope = DescriptorUtil.GetFullUmbrellaClassName(Descriptor.File);
      }
      switch (Descriptor.MappedType) {
        case MappedType.Message:
          type = GetClassName(Descriptor.MessageType);
          break;
        case MappedType.Enum:
          type = GetClassName(Descriptor.EnumType);
          break;
        default:
          type = DescriptorUtil.GetMappedTypeName(Descriptor.MappedType);
          break;
      }
      name = Descriptor.CSharpOptions.PropertyName;
    }

    public void Generate(TextGenerator writer) {
      writer.WriteLine ("public const int {0} = {1};", GetFieldConstantName(Descriptor), Descriptor.FieldNumber);
      if (Descriptor.IsRepeated) {
        writer.WriteLine("{0} static pb::GeneratedExtensionBase<scg::IList<{1}>> {2};", ClassAccessLevel, type, name);
      } else {
        writer.WriteLine("{0} static pb::GeneratedExtensionBase<{1}> {2};", ClassAccessLevel, type, name);
      }
    }

    internal void GenerateStaticVariableInitializers(TextGenerator writer) {
      if (Descriptor.IsRepeated) {
        writer.WriteLine("{0}.{1} = pb::GeneratedRepeatExtension<{2}>.CreateInstance({0}.Descriptor.Extensions[{3}]);", scope, name, type, Descriptor.Index);
      } else {
        writer.WriteLine("{0}.{1} = pb::GeneratedSingleExtension<{2}>.CreateInstance({0}.Descriptor.Extensions[{3}]);", scope, name, type, Descriptor.Index);
      }
    }

    internal void GenerateExtensionRegistrationCode(TextGenerator writer) {
      writer.WriteLine("registry.Add({0}.{1});", scope, name);
    }
  }
}
