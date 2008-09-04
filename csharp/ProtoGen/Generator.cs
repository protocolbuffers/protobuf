using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;
using System.IO;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  /// <summary>
  /// Code generator for protocol buffers. Only C# is supported at the moment.
  /// </summary>
  public sealed class Generator {

    readonly GeneratorOptions options;

    private Generator(GeneratorOptions options) {
      options.Validate();
      this.options = options;
    }

    /// <summary>
    /// Returns a generator configured with the specified options.
    /// </summary>
    public static Generator CreateGenerator(GeneratorOptions options) {
      return new Generator(options);
    }

    public void Generate() {
      foreach (string inputFile in options.InputFiles) {
        FileDescriptorSet descriptorProtos;
        using (Stream inputStream = File.OpenRead(inputFile)) {
          descriptorProtos = FileDescriptorSet.ParseFrom(inputStream);
        }
        List<FileDescriptor> descriptors = ConvertDescriptors(descriptorProtos);
      }
    }

    /// <summary>
    /// Resolves any dependencies and converts FileDescriptorProtos into FileDescriptors.
    /// The list returned is in the same order as the protos are listed in the descriptor set.
    /// Note: this method is internal rather than private to allow testing.
    /// </summary>
    /// <exception cref="DependencyResolutionException">Not all dependencies could be resolved.</exception>
    internal static List<FileDescriptor> ConvertDescriptors(FileDescriptorSet descriptorProtos) {
      return null;
    }
  }
}
