#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#endregion

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen
{
    /// <summary>
    /// Code generator for protocol buffers. Only C# is supported at the moment.
    /// </summary>
    public sealed class Generator
    {
        private readonly GeneratorOptions options;

        private Generator(GeneratorOptions options)
        {
            options.Validate();
            this.options = options;
        }

        /// <summary>
        /// Returns a generator configured with the specified options.
        /// </summary>
        public static Generator CreateGenerator(GeneratorOptions options)
        {
            return new Generator(options);
        }

        public void Generate()
        {
            List<FileDescriptorSet> descriptorProtos = new List<FileDescriptorSet>();
            foreach (string inputFile in options.InputFiles)
            {
                ExtensionRegistry extensionRegistry = ExtensionRegistry.CreateInstance();
                CSharpOptions.RegisterAllExtensions(extensionRegistry);
                using (Stream inputStream = File.OpenRead(inputFile))
                {
                    descriptorProtos.Add(FileDescriptorSet.ParseFrom(inputStream, extensionRegistry));
                }
            }

            IList<FileDescriptor> descriptors = ConvertDescriptors(options.FileOptions, descriptorProtos.ToArray());

            // Combine with options from command line
            foreach (FileDescriptor descriptor in descriptors)
            {
                descriptor.ConfigureWithDefaultOptions(options.FileOptions);
            }

            bool duplicates = false;
            Dictionary<string, bool> names = new Dictionary<string, bool>(StringComparer.OrdinalIgnoreCase);
            foreach (FileDescriptor descriptor in descriptors)
            {
                string file = GetOutputFile(descriptor, false);
                if (names.ContainsKey(file))
                {
                    duplicates = true;
                    break;
                }
                names.Add(file, true);
            }

            foreach (FileDescriptor descriptor in descriptors)
            {
                // Optionally exclude descriptors in google.protobuf
                if (descriptor.CSharpOptions.IgnoreGoogleProtobuf && descriptor.Package == "google.protobuf")
                {
                    continue;
                }
                Generate(descriptor, duplicates);
            }
        }

        /// <summary>
        /// Generates code for a particular file. All dependencies must
        /// already have been resolved.
        /// </summary>
        private void Generate(FileDescriptor descriptor, bool duplicates)
        {
            UmbrellaClassGenerator ucg = new UmbrellaClassGenerator(descriptor);
            using (TextWriter textWriter = File.CreateText(GetOutputFile(descriptor, duplicates)))
            {
                TextGenerator writer = new TextGenerator(textWriter, options.LineBreak);
                ucg.Generate(writer);
            }
        }

        private string GetOutputFile(FileDescriptor descriptor, bool duplicates)
        {
            CSharpFileOptions fileOptions = descriptor.CSharpOptions;

            string filename = descriptor.CSharpOptions.UmbrellaClassname + descriptor.CSharpOptions.FileExtension;
            if (duplicates)
            {
                string namepart;
                if (String.IsNullOrEmpty(descriptor.Name) || String.IsNullOrEmpty(namepart = Path.GetFileNameWithoutExtension(descriptor.Name)))
                    throw new ApplicationException("Duplicate UmbrellaClassname options created a file name collision.");

                filename = namepart + descriptor.CSharpOptions.FileExtension;
            }

            string outputDirectory = descriptor.CSharpOptions.OutputDirectory;
            if (fileOptions.ExpandNamespaceDirectories)
            {
                string package = fileOptions.Namespace;
                if (!string.IsNullOrEmpty(package))
                {
                    string[] bits = package.Split('.');
                    foreach (string bit in bits)
                    {
                        outputDirectory = Path.Combine(outputDirectory, bit);
                    }
                }
            }

            // As the directory can be explicitly specified in options, we need to make sure it exists
            Directory.CreateDirectory(outputDirectory);
            return Path.Combine(outputDirectory, filename);
        }

        /// <summary>
        /// Resolves any dependencies and converts FileDescriptorProtos into FileDescriptors.
        /// The list returned is in the same order as the protos are listed in the descriptor set.
        /// Note: this method is internal rather than private to allow testing.
        /// </summary>
        /// <exception cref="DependencyResolutionException">Not all dependencies could be resolved.</exception>
        public static IList<FileDescriptor> ConvertDescriptors(CSharpFileOptions options,
                                                               params FileDescriptorSet[] descriptorProtos)
        {
            // Simple strategy: Keep going through the list of protos to convert, only doing ones where
            // we've already converted all the dependencies, until we get to a stalemate
            List<FileDescriptorProto> fileList = new List<FileDescriptorProto>();
            foreach (FileDescriptorSet set in descriptorProtos)
            {
                fileList.AddRange(set.FileList);
            }

            FileDescriptor[] converted = new FileDescriptor[fileList.Count];

            Dictionary<string, FileDescriptor> convertedMap = new Dictionary<string, FileDescriptor>();

            int totalConverted = 0;

            bool madeProgress = true;
            while (madeProgress && totalConverted < converted.Length)
            {
                madeProgress = false;
                for (int i = 0; i < converted.Length; i++)
                {
                    if (converted[i] != null)
                    {
                        // Already done this one
                        continue;
                    }
                    FileDescriptorProto candidate = fileList[i];
                    FileDescriptor[] dependencies = new FileDescriptor[candidate.DependencyList.Count];


                    CSharpFileOptions.Builder builder = options.ToBuilder();
                    if (candidate.Options.HasExtension(CSharpOptions.CSharpFileOptions))
                    {
                        builder.MergeFrom(
                            candidate.Options.GetExtension(CSharpOptions.CSharpFileOptions));
                    }
                    CSharpFileOptions localOptions = builder.Build();

                    bool foundAllDependencies = true;
                    for (int j = 0; j < dependencies.Length; j++)
                    {
                        if (!convertedMap.TryGetValue(candidate.DependencyList[j], out dependencies[j]))
                        {
                            // We can auto-magically resolve these since we already have their description
                            // This way if the file is only referencing options it does not need to be built with the
                            // --include_imports definition.
                            if (localOptions.IgnoreGoogleProtobuf &&
                                (candidate.DependencyList[j] == "google/protobuf/csharp_options.proto"))
                            {
                                dependencies[j] = CSharpOptions.Descriptor;
                                continue;
                            }
                            if (localOptions.IgnoreGoogleProtobuf &&
                                (candidate.DependencyList[j] == "google/protobuf/descriptor.proto"))
                            {
                                dependencies[j] = DescriptorProtoFile.Descriptor;
                                continue;
                            }
                            foundAllDependencies = false;
                            break;
                        }
                    }
                    if (!foundAllDependencies)
                    {
                        continue;
                    }
                    madeProgress = true;
                    totalConverted++;
                    converted[i] = FileDescriptor.BuildFrom(candidate, dependencies);
                    convertedMap[candidate.Name] = converted[i];
                }
            }
            if (!madeProgress)
            {
                StringBuilder remaining = new StringBuilder();
                for (int i = 0; i < converted.Length; i++)
                {
                    if (converted[i] == null)
                    {
                        if (remaining.Length != 0)
                        {
                            remaining.Append(", ");
                        }
                        FileDescriptorProto failure = fileList[i];
                        remaining.Append(failure.Name);
                        remaining.Append(":");
                        foreach (string dependency in failure.DependencyList)
                        {
                            if (!convertedMap.ContainsKey(dependency))
                            {
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