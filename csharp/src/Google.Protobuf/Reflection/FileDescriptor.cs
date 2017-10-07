#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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
using System.Collections.ObjectModel;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Describes a .proto file, including everything defined within.
    /// IDescriptor is implemented such that the File property returns this descriptor,
    /// and the FullName is the same as the Name.
    /// </summary>
    public sealed class FileDescriptor : IDescriptor
    {
        private FileDescriptor(ByteString descriptorData, FileDescriptorProto proto, FileDescriptor[] dependencies, DescriptorPool pool, bool allowUnknownDependencies, GeneratedClrTypeInfo generatedCodeInfo)
        {
            SerializedData = descriptorData;
            DescriptorPool = pool;
            Proto = proto;
            Dependencies = new ReadOnlyCollection<FileDescriptor>((FileDescriptor[]) dependencies.Clone());

            PublicDependencies = DeterminePublicDependencies(this, proto, dependencies, allowUnknownDependencies);

            pool.AddPackage(Package, this);

            MessageTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.MessageType,
                                                                 (message, index) =>
                                                                 new MessageDescriptor(message, this, null, index, generatedCodeInfo.NestedTypes[index]));

            EnumTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.EnumType,
                                                              (enumType, index) =>
                                                              new EnumDescriptor(enumType, this, null, index, generatedCodeInfo.NestedEnums[index]));

            Services = DescriptorUtil.ConvertAndMakeReadOnly(proto.Service,
                                                             (service, index) =>
                                                             new ServiceDescriptor(service, this, index));
        }

        /// <summary>
        /// Computes the full name of a descriptor within this file, with an optional parent message.
        /// </summary>
        internal string ComputeFullName(MessageDescriptor parent, string name)
        {
            if (parent != null)
            {
                return parent.FullName + "." + name;
            }
            if (Package.Length > 0)
            {
                return Package + "." + name;
            }
            return name;
        }

        /// <summary>
        /// Extracts public dependencies from direct dependencies. This is a static method despite its
        /// first parameter, as the value we're in the middle of constructing is only used for exceptions.
        /// </summary>
        private static IList<FileDescriptor> DeterminePublicDependencies(FileDescriptor @this, FileDescriptorProto proto, FileDescriptor[] dependencies, bool allowUnknownDependencies)
        {
            var nameToFileMap = new Dictionary<string, FileDescriptor>();
            foreach (var file in dependencies)
            {
                nameToFileMap[file.Name] = file;
            }
            var publicDependencies = new List<FileDescriptor>();
            for (int i = 0; i < proto.PublicDependency.Count; i++)
            {
                int index = proto.PublicDependency[i];
                if (index < 0 || index >= proto.Dependency.Count)
                {
                    throw new DescriptorValidationException(@this, "Invalid public dependency index.");
                }
                string name = proto.Dependency[index];
                FileDescriptor file = nameToFileMap[name];
                if (file == null)
                {
                    if (!allowUnknownDependencies)
                    {
                        throw new DescriptorValidationException(@this, "Invalid public dependency: " + name);
                    }
                    // Ignore unknown dependencies.
                }
                else
                {
                    publicDependencies.Add(file);
                }
            }
            return new ReadOnlyCollection<FileDescriptor>(publicDependencies);
        }

        /// <value>
        /// The descriptor in its protocol message representation.
        /// </value>
        internal FileDescriptorProto Proto { get; }

        /// <value>
        /// The file name.
        /// </value>
        public string Name => Proto.Name;

        /// <summary>
        /// The package as declared in the .proto file. This may or may not
        /// be equivalent to the .NET namespace of the generated classes.
        /// </summary>
        public string Package => Proto.Package;

        /// <value>
        /// Unmodifiable list of top-level message types declared in this file.
        /// </value>
        public IList<MessageDescriptor> MessageTypes { get; }

        /// <value>
        /// Unmodifiable list of top-level enum types declared in this file.
        /// </value>
        public IList<EnumDescriptor> EnumTypes { get; }

        /// <value>
        /// Unmodifiable list of top-level services declared in this file.
        /// </value>
        public IList<ServiceDescriptor> Services { get; }

        /// <value>
        /// Unmodifiable list of this file's dependencies (imports).
        /// </value>
        public IList<FileDescriptor> Dependencies { get; }

        /// <value>
        /// Unmodifiable list of this file's public dependencies (public imports).
        /// </value>
        public IList<FileDescriptor> PublicDependencies { get; }

        /// <value>
        /// The original serialized binary form of this descriptor.
        /// </value>
        public ByteString SerializedData { get; }

        /// <value>
        /// Implementation of IDescriptor.FullName - just returns the same as Name.
        /// </value>
        string IDescriptor.FullName => Name;

        /// <value>
        /// Implementation of IDescriptor.File - just returns this descriptor.
        /// </value>
        FileDescriptor IDescriptor.File => this;

        /// <value>
        /// Pool containing symbol descriptors.
        /// </value>
        internal DescriptorPool DescriptorPool { get; }

        /// <summary>
        /// Finds a type (message, enum, service or extension) in the file by name. Does not find nested types.
        /// </summary>
        /// <param name="name">The unqualified type name to look for.</param>
        /// <typeparam name="T">The type of descriptor to look for</typeparam>
        /// <returns>The type's descriptor, or null if not found.</returns>
        public T FindTypeByName<T>(String name)
            where T : class, IDescriptor
        {
            // Don't allow looking up nested types.  This will make optimization
            // easier later.
            if (name.IndexOf('.') != -1)
            {
                return null;
            }
            if (Package.Length > 0)
            {
                name = Package + "." + name;
            }
            T result = DescriptorPool.FindSymbol<T>(name);
            if (result != null && result.File == this)
            {
                return result;
            }
            return null;
        }

        /// <summary>
        /// Builds a FileDescriptor from its protocol buffer representation.
        /// </summary>
        /// <param name="descriptorData">The original serialized descriptor data.
        /// We have only limited proto2 support, so serializing FileDescriptorProto
        /// would not necessarily give us this.</param>
        /// <param name="proto">The protocol message form of the FileDescriptor.</param>
        /// <param name="dependencies">FileDescriptors corresponding to all of the
        /// file's dependencies, in the exact order listed in the .proto file. May be null,
        /// in which case it is treated as an empty array.</param>
        /// <param name="allowUnknownDependencies">Whether unknown dependencies are ignored (true) or cause an exception to be thrown (false).</param>
        /// <param name="generatedCodeInfo">Details about generated code, for the purposes of reflection.</param>
        /// <exception cref="DescriptorValidationException">If <paramref name="proto"/> is not
        /// a valid descriptor. This can occur for a number of reasons, such as a field
        /// having an undefined type or because two messages were defined with the same name.</exception>
        private static FileDescriptor BuildFrom(ByteString descriptorData, FileDescriptorProto proto, FileDescriptor[] dependencies, bool allowUnknownDependencies, GeneratedClrTypeInfo generatedCodeInfo)
        {
            // Building descriptors involves two steps: translating and linking.
            // In the translation step (implemented by FileDescriptor's
            // constructor), we build an object tree mirroring the
            // FileDescriptorProto's tree and put all of the descriptors into the
            // DescriptorPool's lookup tables.  In the linking step, we look up all
            // type references in the DescriptorPool, so that, for example, a
            // FieldDescriptor for an embedded message contains a pointer directly
            // to the Descriptor for that message's type.  We also detect undefined
            // types in the linking step.
            if (dependencies == null)
            {
                dependencies = new FileDescriptor[0];
            }

            DescriptorPool pool = new DescriptorPool(dependencies);
            FileDescriptor result = new FileDescriptor(descriptorData, proto, dependencies, pool, allowUnknownDependencies, generatedCodeInfo);

            // Validate that the dependencies we've been passed (as FileDescriptors) are actually the ones we
            // need.
            if (dependencies.Length != proto.Dependency.Count)
            {
                throw new DescriptorValidationException(
                    result,
                    "Dependencies passed to FileDescriptor.BuildFrom() don't match " +
                    "those listed in the FileDescriptorProto.");
            }

            result.CrossLink();
            return result;
        }

        private void CrossLink()
        {
            foreach (MessageDescriptor message in MessageTypes)
            {
                message.CrossLink();
            }

            foreach (ServiceDescriptor service in Services)
            {
                service.CrossLink();
            }
        }

        /// <summary>
        /// Creates a descriptor for generated code.
        /// </summary>
        /// <remarks>
        /// This method is only designed to be used by the results of generating code with protoc,
        /// which creates the appropriate dependencies etc. It has to be public because the generated
        /// code is "external", but should not be called directly by end users.
        /// </remarks>
        public static FileDescriptor FromGeneratedCode(
            byte[] descriptorData,
            FileDescriptor[] dependencies,
            GeneratedClrTypeInfo generatedCodeInfo)
        {
            FileDescriptorProto proto;
            try
            {
                proto = FileDescriptorProto.Parser.ParseFrom(descriptorData);
            }
            catch (InvalidProtocolBufferException e)
            {
                throw new ArgumentException("Failed to parse protocol buffer descriptor for generated code.", e);
            }

            try
            {
                // When building descriptors for generated code, we allow unknown
                // dependencies by default.
                return BuildFrom(ByteString.CopyFrom(descriptorData), proto, dependencies, true, generatedCodeInfo);
            }
            catch (DescriptorValidationException e)
            {
                throw new ArgumentException($"Invalid embedded descriptor for \"{proto.Name}\".", e);
            }
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>
        /// A <see cref="System.String" /> that represents this instance.
        /// </returns>
        public override string ToString()
        {
            return $"FileDescriptor for {Name}";
        }

        /// <summary>
        /// Returns the file descriptor for descriptor.proto.
        /// </summary>
        /// <remarks>
        /// This is used for protos which take a direct dependency on <c>descriptor.proto</c>, typically for
        /// annotations. While <c>descriptor.proto</c> is a proto2 file, it is built into the Google.Protobuf
        /// runtime for reflection purposes. The messages are internal to the runtime as they would require
        /// proto2 semantics for full support, but the file descriptor is available via this property. The
        /// C# codegen in protoc automatically uses this property when it detects a dependency on <c>descriptor.proto</c>.
        /// </remarks>
        /// <value>
        /// The file descriptor for <c>descriptor.proto</c>.
        /// </value>
        public static FileDescriptor DescriptorProtoFileDescriptor { get { return DescriptorReflection.Descriptor; } }

        /// <summary>
        /// The (possibly empty) set of custom options for this file.
        /// </summary>
        public CustomOptions CustomOptions => Proto.Options?.CustomOptions ?? CustomOptions.Empty;
    }
}
