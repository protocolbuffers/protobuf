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
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using Google.ProtocolBuffers.DescriptorProtos;
using FileOptions = Google.ProtocolBuffers.DescriptorProtos.FileOptions;

namespace Google.ProtocolBuffers.Descriptors
{
    /// <summary>
    /// Describes a .proto file, including everything defined within.
    /// IDescriptor is implemented such that the File property returns this descriptor,
    /// and the FullName is the same as the Name.
    /// </summary>
    public sealed class FileDescriptor : IDescriptor<FileDescriptorProto>
    {
        private FileDescriptorProto proto;
        private readonly IList<MessageDescriptor> messageTypes;
        private readonly IList<EnumDescriptor> enumTypes;
        private readonly IList<ServiceDescriptor> services;
        private readonly IList<FieldDescriptor> extensions;
        private readonly IList<FileDescriptor> dependencies;
        private readonly DescriptorPool pool;
        private CSharpFileOptions csharpFileOptions;
        private readonly object optionsLock = new object();

        private FileDescriptor(FileDescriptorProto proto, FileDescriptor[] dependencies, DescriptorPool pool)
        {
            this.pool = pool;
            this.proto = proto;
            this.dependencies = new ReadOnlyCollection<FileDescriptor>((FileDescriptor[]) dependencies.Clone());

            pool.AddPackage(Package, this);

            messageTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.MessageTypeList,
                                                                 (message, index) =>
                                                                 new MessageDescriptor(message, this, null, index));

            enumTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.EnumTypeList,
                                                              (enumType, index) =>
                                                              new EnumDescriptor(enumType, this, null, index));

            services = DescriptorUtil.ConvertAndMakeReadOnly(proto.ServiceList,
                                                             (service, index) =>
                                                             new ServiceDescriptor(service, this, index));

            extensions = DescriptorUtil.ConvertAndMakeReadOnly(proto.ExtensionList,
                                                               (field, index) =>
                                                               new FieldDescriptor(field, this, null, index, true));
        }


        /// <summary>
        /// Allows a file descriptor to be configured with a set of external options, e.g. from the
        /// command-line arguments to protogen.
        /// </summary>
        public void ConfigureWithDefaultOptions(CSharpFileOptions options)
        {
            csharpFileOptions = BuildOrFakeWithDefaultOptions(options);
        }

        private CSharpFileOptions BuildOrFakeWithDefaultOptions(CSharpFileOptions defaultOptions)
        {
            // Fix for being able to relocate these files to any directory structure
            if (proto.Package == "google.protobuf")
            {
                string filename = Path.GetFileName(proto.Name);
                // TODO(jonskeet): Check if we could use FileDescriptorProto.Descriptor.Name - interesting bootstrap issues)
                if (filename == "descriptor.proto")
                {
                    return new CSharpFileOptions.Builder
                               {
                                   Namespace = "Google.ProtocolBuffers.DescriptorProtos",
                                   UmbrellaClassname = "DescriptorProtoFile",
                                   NestClasses = false,
                                   MultipleFiles = false,
                                   PublicClasses = true,
                                   OutputDirectory = defaultOptions.OutputDirectory,
                                   IgnoreGoogleProtobuf = defaultOptions.IgnoreGoogleProtobuf
                               }.Build();
                }
                if (filename == "csharp_options.proto")
                {
                    return new CSharpFileOptions.Builder
                               {
                                   Namespace = "Google.ProtocolBuffers.DescriptorProtos",
                                   UmbrellaClassname = "CSharpOptions",
                                   NestClasses = false,
                                   MultipleFiles = false,
                                   PublicClasses = true,
                                   OutputDirectory = defaultOptions.OutputDirectory,
                                   IgnoreGoogleProtobuf = defaultOptions.IgnoreGoogleProtobuf
                               }.Build();
                }
            }
            CSharpFileOptions.Builder builder = defaultOptions.ToBuilder();
            if (proto.Options.HasExtension(DescriptorProtos.CSharpOptions.CSharpFileOptions))
            {
                builder.MergeFrom(proto.Options.GetExtension(DescriptorProtos.CSharpOptions.CSharpFileOptions));
            }
            if (!builder.HasNamespace)
            {
                builder.Namespace = Package;
            }
            if (!builder.HasUmbrellaClassname)
            {
                int lastSlash = Name.LastIndexOf('/');
                string baseName = Name.Substring(lastSlash + 1);
                builder.UmbrellaClassname = NameHelpers.UnderscoresToPascalCase(NameHelpers.StripProto(baseName));
            }

            // Auto-fix for name collision by placing umbrella class into a new namespace.  This
            // still won't fix the collisions with nesting enabled; however, you have to turn that on explicitly anyway.
            if (!builder.NestClasses && !builder.HasUmbrellaNamespace)
            {
                bool collision = false;
                foreach (IDescriptor d in MessageTypes)
                {
                    collision |= d.Name == builder.UmbrellaClassname;
                }
                foreach (IDescriptor d in Services)
                {
                    collision |= d.Name == builder.UmbrellaClassname;
                }
                foreach (IDescriptor d in EnumTypes)
                {
                    collision |= d.Name == builder.UmbrellaClassname;
                }
                if (collision)
                {
                    builder.UmbrellaNamespace = "Proto";
                }
            }

            return builder.Build();
        }

        /// <value>
        /// The descriptor in its protocol message representation.
        /// </value>
        public FileDescriptorProto Proto
        {
            get { return proto; }
        }

        /// <value>
        /// The <see cref="DescriptorProtos.FileOptions" /> defined in <c>descriptor.proto</c>.
        /// </value>
        public FileOptions Options
        {
            get { return proto.Options; }
        }

        /// <summary>
        /// Returns the C#-specific options for this file descriptor. This will always be
        /// completely filled in.
        /// </summary>
        public CSharpFileOptions CSharpOptions
        {
            get
            {
                lock (optionsLock)
                {
                    if (csharpFileOptions == null)
                    {
                        csharpFileOptions = BuildOrFakeWithDefaultOptions(CSharpFileOptions.DefaultInstance);
                    }
                }
                return csharpFileOptions;
            }
        }

        /// <value>
        /// The file name.
        /// </value>
        public string Name
        {
            get { return proto.Name; }
        }

        /// <summary>
        /// The package as declared in the .proto file. This may or may not
        /// be equivalent to the .NET namespace of the generated classes.
        /// </summary>
        public string Package
        {
            get { return proto.Package; }
        }

        /// <value>
        /// Unmodifiable list of top-level message types declared in this file.
        /// </value>
        public IList<MessageDescriptor> MessageTypes
        {
            get { return messageTypes; }
        }

        /// <value>
        /// Unmodifiable list of top-level enum types declared in this file.
        /// </value>
        public IList<EnumDescriptor> EnumTypes
        {
            get { return enumTypes; }
        }

        /// <value>
        /// Unmodifiable list of top-level services declared in this file.
        /// </value>
        public IList<ServiceDescriptor> Services
        {
            get { return services; }
        }

        /// <value>
        /// Unmodifiable list of top-level extensions declared in this file.
        /// </value>
        public IList<FieldDescriptor> Extensions
        {
            get { return extensions; }
        }

        /// <value>
        /// Unmodifiable list of this file's dependencies (imports).
        /// </value>
        public IList<FileDescriptor> Dependencies
        {
            get { return dependencies; }
        }

        /// <value>
        /// Implementation of IDescriptor.FullName - just returns the same as Name.
        /// </value>
        string IDescriptor.FullName
        {
            get { return Name; }
        }

        /// <value>
        /// Implementation of IDescriptor.File - just returns this descriptor.
        /// </value>
        FileDescriptor IDescriptor.File
        {
            get { return this; }
        }

        /// <value>
        /// Protocol buffer describing this descriptor.
        /// </value>
        IMessage IDescriptor.Proto
        {
            get { return Proto; }
        }

        /// <value>
        /// Pool containing symbol descriptors.
        /// </value>
        internal DescriptorPool DescriptorPool
        {
            get { return pool; }
        }

        /// <summary>
        /// Finds a type (message, enum, service or extension) in the file by name. Does not find nested types.
        /// </summary>
        /// <param name="name">The unqualified type name to look for.</param>
        /// <typeparam name="T">The type of descriptor to look for (or ITypeDescriptor for any)</typeparam>
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
            T result = pool.FindSymbol<T>(name);
            if (result != null && result.File == this)
            {
                return result;
            }
            return null;
        }

        /// <summary>
        /// Builds a FileDescriptor from its protocol buffer representation.
        /// </summary>
        /// <param name="proto">The protocol message form of the FileDescriptor.</param>
        /// <param name="dependencies">FileDescriptors corresponding to all of the
        /// file's dependencies, in the exact order listed in the .proto file. May be null,
        /// in which case it is treated as an empty array.</param>
        /// <exception cref="DescriptorValidationException">If <paramref name="proto"/> is not
        /// a valid descriptor. This can occur for a number of reasons, such as a field
        /// having an undefined type or because two messages were defined with the same name.</exception>
        public static FileDescriptor BuildFrom(FileDescriptorProto proto, FileDescriptor[] dependencies)
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
            FileDescriptor result = new FileDescriptor(proto, dependencies, pool);

            if (dependencies.Length != proto.DependencyCount)
            {
                throw new DescriptorValidationException(result,
                                                        "Dependencies passed to FileDescriptor.BuildFrom() don't match " +
                                                        "those listed in the FileDescriptorProto.");
            }
            for (int i = 0; i < proto.DependencyCount; i++)
            {
                if (dependencies[i].Name != proto.DependencyList[i])
                {
                    throw new DescriptorValidationException(result,
                                                            "Dependencies passed to FileDescriptor.BuildFrom() don't match " +
                                                            "those listed in the FileDescriptorProto.");
                }
            }

            result.CrossLink();
            return result;
        }

        private void CrossLink()
        {
            foreach (MessageDescriptor message in messageTypes)
            {
                message.CrossLink();
            }

            foreach (ServiceDescriptor service in services)
            {
                service.CrossLink();
            }

            foreach (FieldDescriptor extension in extensions)
            {
                extension.CrossLink();
            }

            foreach (MessageDescriptor message in messageTypes)
            {
                message.CheckRequiredFields();
            }
        }

        /// <summary>
        /// This method is to be called by generated code only.  It is equivalent
        /// to BuildFrom except that the FileDescriptorProto is encoded in
        /// protocol buffer wire format. This overload is maintained for backward
        /// compatibility with source code generated before the custom options were available
        /// (and working).
        /// </summary>
        public static FileDescriptor InternalBuildGeneratedFileFrom(byte[] descriptorData, FileDescriptor[] dependencies)
        {
            return InternalBuildGeneratedFileFrom(descriptorData, dependencies, x => null);
        }

        /// <summary>
        /// This delegate should be used by generated code only. When calling
        /// FileDescriptor.InternalBuildGeneratedFileFrom, the caller can provide
        /// a callback which assigns the global variables defined in the generated code
        /// which point at parts of the FileDescriptor. The callback returns an
        /// Extension Registry which contains any extensions which might be used in
        /// the descriptor - that is, extensions of the various "Options" messages defined
        /// in descriptor.proto. The callback may also return null to indicate that
        /// no extensions are used in the descriptor.
        /// </summary>
        /// <param name="descriptor"></param>
        /// <returns></returns>
        public delegate ExtensionRegistry InternalDescriptorAssigner(FileDescriptor descriptor);

        public static FileDescriptor InternalBuildGeneratedFileFrom(byte[] descriptorData,
                                                                    FileDescriptor[] dependencies,
                                                                    InternalDescriptorAssigner descriptorAssigner)
        {
            FileDescriptorProto proto;
            try
            {
                proto = FileDescriptorProto.ParseFrom(descriptorData);
            }
            catch (InvalidProtocolBufferException e)
            {
                throw new ArgumentException("Failed to parse protocol buffer descriptor for generated code.", e);
            }

            FileDescriptor result;
            try
            {
                result = BuildFrom(proto, dependencies);
            }
            catch (DescriptorValidationException e)
            {
                throw new ArgumentException("Invalid embedded descriptor for \"" + proto.Name + "\".", e);
            }

            ExtensionRegistry registry = descriptorAssigner(result);

            if (registry != null)
            {
                // We must re-parse the proto using the registry.
                try
                {
                    proto = FileDescriptorProto.ParseFrom(descriptorData, registry);
                }
                catch (InvalidProtocolBufferException e)
                {
                    throw new ArgumentException("Failed to parse protocol buffer descriptor for generated code.", e);
                }

                result.ReplaceProto(proto);
            }
            return result;
        }

        /// <summary>
        /// Replace our FileDescriptorProto with the given one, which is
        /// identical except that it might contain extensions that weren't present
        /// in the original. This method is needed for bootstrapping when a file
        /// defines custom options. The options may be defined in the file itself,
        /// so we can't actually parse them until we've constructed the descriptors,
        /// but to construct the decsriptors we have to have parsed the descriptor
        /// protos. So, we have to parse the descriptor protos a second time after
        /// constructing the descriptors.
        /// </summary>
        private void ReplaceProto(FileDescriptorProto newProto)
        {
            proto = newProto;

            for (int i = 0; i < messageTypes.Count; i++)
            {
                messageTypes[i].ReplaceProto(proto.GetMessageType(i));
            }

            for (int i = 0; i < enumTypes.Count; i++)
            {
                enumTypes[i].ReplaceProto(proto.GetEnumType(i));
            }

            for (int i = 0; i < services.Count; i++)
            {
                services[i].ReplaceProto(proto.GetService(i));
            }

            for (int i = 0; i < extensions.Count; i++)
            {
                extensions[i].ReplaceProto(proto.GetExtension(i));
            }
        }
    }
}