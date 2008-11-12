using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;
using System.IO;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.Collections;

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
        ExtensionRegistry extensionRegistry = ExtensionRegistry.CreateInstance();
        extensionRegistry.Add(CSharpFileOptions.CSharpOptions);
        extensionRegistry.Add(CSharpFieldOptions.CSharpOptions);
        using (Stream inputStream = File.OpenRead(inputFile)) {
          descriptorProtos = FileDescriptorSet.ParseFrom(inputStream, extensionRegistry);
        }
        IList<FileDescriptor> descriptors = ConvertDescriptors(descriptorProtos);

        foreach (FileDescriptor descriptor in descriptors) {
          Generate(descriptor);
        }
      }
    }

    /// <summary>
    /// Generates code for a particular file. All dependencies must
    /// already have been resolved.
    /// </summary>
    private void Generate(FileDescriptor descriptor) {
      UmbrellaClassGenerator ucg = new UmbrellaClassGenerator(descriptor);
      using (TextWriter textWriter = File.CreateText(Path.Combine(options.OutputDirectory, descriptor.CSharpOptions.UmbrellaClassname + ".cs"))) {
        TextGenerator writer = new TextGenerator(textWriter);        
        ucg.Generate(writer);
        /*
        GenerateSiblings(umbrellaSource, descriptor, descriptor.MessageTypes);
        GenerateSiblings(umbrellaSource, descriptor, descriptor.EnumTypes);
        GenerateSiblings(umbrellaSource, descriptor, descriptor.Services);*/
      }
    }

    private static void GenerateSiblings<T>(SourceFileGenerator parentSourceGenerator, FileDescriptor file, IEnumerable<T> siblings)
        where T : IDescriptor {
    }

    /// <summary>
    /// Resolves any dependencies and converts FileDescriptorProtos into FileDescriptors.
    /// The list returned is in the same order as the protos are listed in the descriptor set.
    /// Note: this method is internal rather than private to allow testing.
    /// </summary>
    /// <exception cref="DependencyResolutionException">Not all dependencies could be resolved.</exception>
    internal static IList<FileDescriptor> ConvertDescriptors(FileDescriptorSet descriptorProtos) {
      // Simple strategy: Keep going through the list of protos to convert, only doing ones where
      // we've already converted all the dependencies, until we get to a stalemate
      IList<FileDescriptorProto> fileList = descriptorProtos.FileList;
      FileDescriptor[] converted = new FileDescriptor[fileList.Count];

      Dictionary<string, FileDescriptor> convertedMap = new Dictionary<string, FileDescriptor>();

      int totalConverted = 0;

      bool madeProgress = true;
      while (madeProgress && totalConverted < converted.Length) {
        madeProgress = false;
        for (int i = 0; i < converted.Length; i++) {
          if (converted[i] != null) {
            // Already done this one
            continue;
          }
          FileDescriptorProto candidate = fileList[i];
          FileDescriptor[] dependencies = new FileDescriptor[candidate.DependencyList.Count];
          bool foundAllDependencies = true;
          for (int j = 0; j < dependencies.Length; j++) {
            if (!convertedMap.TryGetValue(candidate.DependencyList[j], out dependencies[j])) {
              foundAllDependencies = false;
              break;
            }
          }
          if (!foundAllDependencies) {
            continue;
          }
          madeProgress = true;
          totalConverted++;
          converted[i] = FileDescriptor.BuildFrom(candidate, dependencies);
          convertedMap[candidate.Name] = converted[i];
        }
      }
      if (!madeProgress) {
        StringBuilder remaining = new StringBuilder();
        for (int i = 0; i < converted.Length; i++) {
          if (converted[i] == null) {
            if (remaining.Length != 0) {
              remaining.Append(", ");
            }
            FileDescriptorProto failure = fileList[i];
            remaining.Append(failure.Name);
            remaining.Append(":");
            foreach (string dependency in failure.DependencyList) {
              if (!convertedMap.ContainsKey(dependency)) {
                remaining.Append(" ");
                remaining.Append(dependency);
              }
            }
            remaining.Append(";");
          }
        }
        throw new DependencyResolutionException("Unable to resolve all dependencies: " + remaining);
      }
      return Lists.AsReadOnly(converted);
    }
  }
}
