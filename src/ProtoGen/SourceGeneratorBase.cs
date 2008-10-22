using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal abstract class SourceGeneratorBase<T> where T : IDescriptor {

    private readonly T descriptor;

    protected SourceGeneratorBase(T descriptor) {
      this.descriptor = descriptor;
    }

    protected T Descriptor {
      get { return descriptor; }
    }

    protected string ClassAccessLevel {
      get { 
        // Default to public
        return !descriptor.File.Options.HasExtension(CSharpOptions.CSharpPublicClasses)
            || descriptor.File.Options.GetExtension(CSharpOptions.CSharpPublicClasses) ? "public" : "internal";
      }
    }

    public bool MultipleFiles {
      get { return descriptor.File.Options.GetExtension(CSharpOptions.CSharpMultipleFiles); }
    }

    protected static void WriteChildren<TChild>(TextGenerator writer, string region, IEnumerable<TChild> children) 
        where TChild : IDescriptor {
      // Copy the set of children; makes access easier
      List<TChild> copy = new List<TChild>(children);
      if (copy.Count == 0) {
        return;
      }

      if (region != null) {
        writer.WriteLine("#region {0}", region);
      }
      foreach (TChild child in children) {
        SourceGenerators.CreateGenerator(child).Generate(writer);
      }
      if (region != null) {
        writer.WriteLine("#endregion");
        writer.WriteLine();
      }
    }
  }
}
