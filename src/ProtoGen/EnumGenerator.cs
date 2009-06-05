using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal class EnumGenerator : SourceGeneratorBase<EnumDescriptor>, ISourceGenerator {
    internal EnumGenerator(EnumDescriptor descriptor) : base(descriptor) {
    }

    // TODO(jonskeet): Write out enum descriptors? Can be retrieved from file...
    public void Generate(TextGenerator writer) {
      writer.WriteLine("{0} enum {1} {{", ClassAccessLevel, Descriptor.Name);
      writer.Indent();
      foreach (EnumValueDescriptor value in Descriptor.Values) {
        writer.WriteLine("{0} = {1},", value.Name, value.Number);
      }
      writer.Outdent();
      writer.WriteLine("}");
      writer.WriteLine();
    }
  }
}
