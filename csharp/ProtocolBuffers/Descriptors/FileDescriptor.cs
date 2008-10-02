// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// Describes a .proto file, including everything defined within.
  /// IDescriptor is implemented such that the File property returns this descriptor,
  /// and the FullName is the same as the Name.
  /// </summary>
  public sealed class FileDescriptor : IDescriptor<FileDescriptorProto> {

    private readonly FileDescriptorProto proto;
    private readonly IList<MessageDescriptor> messageTypes;
    private readonly IList<EnumDescriptor> enumTypes;
    private readonly IList<ServiceDescriptor> services;
    private readonly IList<FieldDescriptor> extensions;
    private readonly IList<FileDescriptor> dependencies;
    private readonly DescriptorPool pool;
    
    private FileDescriptor(FileDescriptorProto proto, FileDescriptor[] dependencies, DescriptorPool pool) {
      this.pool = pool;
      this.proto = proto;
      this.dependencies = new ReadOnlyCollection<FileDescriptor>((FileDescriptor[]) dependencies.Clone());

      pool.AddPackage(Package, this);

      messageTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.MessageTypeList, 
          (message, index) => new MessageDescriptor(message, this, null, index));

      enumTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.EnumTypeList,
        (enumType, index) => new EnumDescriptor(enumType, this, null, index));

      services = DescriptorUtil.ConvertAndMakeReadOnly(proto.ServiceList,
        (service, index) => new ServiceDescriptor(service, this, index));

      extensions = DescriptorUtil.ConvertAndMakeReadOnly(proto.ExtensionList,
        (field, index) => new FieldDescriptor(field, this, null, index, true));
    }

    /// <value>
    /// The descriptor in its protocol message representation.
    /// </value>
    public FileDescriptorProto Proto {
      get { return proto; }
    }

    /// <value>
    /// The <see cref="FileOptions" /> defined in <c>descriptor.proto</c>.
    /// </value>
    public FileOptions Options {
      get { return proto.Options; }
    }

    /// <value>
    /// The file name.
    /// </value>
    public string Name {
      get { return proto.Name; }
    }

    /// <summary>
    /// The package as declared in the .proto file. This may or may not
    /// be equivalent to the .NET namespace of the generated classes.
    /// </summary>
    public string Package {
      get { return proto.Package; }
    }

    /// <value>
    /// Unmodifiable list of top-level message types declared in this file.
    /// </value>
    public IList<MessageDescriptor> MessageTypes {
      get { return messageTypes; }
    }

    /// <value>
    /// Unmodifiable list of top-level enum types declared in this file.
    /// </value>
    public IList<EnumDescriptor> EnumTypes {
      get { return enumTypes; }
    }

    /// <value>
    /// Unmodifiable list of top-level services declared in this file.
    /// </value>
    public IList<ServiceDescriptor> Services {
      get { return services; }
    }

    /// <value>
    /// Unmodifiable list of top-level extensions declared in this file.
    /// </value>
    public IList<FieldDescriptor> Extensions {
      get { return extensions; }
    }

    /// <value>
    /// Unmodifiable list of this file's dependencies (imports).
    /// </value>
    public IList<FileDescriptor> Dependencies {
      get { return dependencies; }
    }

    /// <value>
    /// Implementation of IDescriptor.FullName - just returns the same as Name.
    /// </value>
    string IDescriptor.FullName {
      get { return Name; }
    }

    /// <value>
    /// Implementation of IDescriptor.File - just returns this descriptor.
    /// </value>
    FileDescriptor IDescriptor.File {
      get { return this; }
    }

    /// <value>
    /// Protocol buffer describing this descriptor.
    /// </value>
    IMessage IDescriptor.Proto {
      get { return Proto; }
    }

    /// <value>
    /// Pool containing symbol descriptors.
    /// </value>
    internal DescriptorPool DescriptorPool {
      get { return pool; }
    }
    
    /// <summary>
    /// Finds a type (message, enum, service or extension) in the file by name. Does not find nested types.
    /// </summary>
    /// <param name="name">The unqualified type name to look for.</param>
    /// <typeparam name="T">The type of descriptor to look for (or ITypeDescriptor for any)</typeparam>
    /// <returns>The type's descriptor, or null if not found.</returns>
    public T FindTypeByName<T>(String name) 
        where T : class, IDescriptor {
      // Don't allow looking up nested types.  This will make optimization
      // easier later.
      if (name.IndexOf('.') != -1) {
        return null;
      }
      if (Package.Length > 0) {
        name = Package + "." + name;
      }
      T result = pool.FindSymbol<T>(name);
      if (result != null && result.File == this) {
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
    public static FileDescriptor BuildFrom(FileDescriptorProto proto, FileDescriptor[] dependencies) {
      // Building decsriptors involves two steps:  translating and linking.
      // In the translation step (implemented by FileDescriptor's
      // constructor), we build an object tree mirroring the
      // FileDescriptorProto's tree and put all of the descriptors into the
      // DescriptorPool's lookup tables.  In the linking step, we look up all
      // type references in the DescriptorPool, so that, for example, a
      // FieldDescriptor for an embedded message contains a pointer directly
      // to the Descriptor for that message's type.  We also detect undefined
      // types in the linking step.
      if (dependencies == null) {
        dependencies = new FileDescriptor[0];
      }

      DescriptorPool pool = new DescriptorPool(dependencies);
      FileDescriptor result = new FileDescriptor(proto, dependencies, pool);

      if (dependencies.Length != proto.DependencyCount) {
        throw new DescriptorValidationException(result,
          "Dependencies passed to FileDescriptor.BuildFrom() don't match " +
          "those listed in the FileDescriptorProto.");
      }
      for (int i = 0; i < proto.DependencyCount; i++) {
        if (dependencies[i].Name != proto.DependencyList[i]) {
          throw new DescriptorValidationException(result,
            "Dependencies passed to FileDescriptor.BuildFrom() don't match " +
            "those listed in the FileDescriptorProto.");
        }
      }

      result.CrossLink();
      return result;
    }                                 

    private void CrossLink() {
      foreach (MessageDescriptor message in messageTypes) {
        message.CrossLink();
      }

      foreach (ServiceDescriptor service in services) {
        service.CrossLink();
      }

      foreach (FieldDescriptor extension in extensions) {
        extension.CrossLink();
      }

      foreach (MessageDescriptor message in messageTypes) {
        message.CheckRequiredFields();
      }
    }
    
    /// <summary>
    /// This method is to be called by generated code only.  It is equivalent
    /// to BuilderFrom except that the FileDescriptorProto is encoded in
    /// protocol buffer wire format.
    /// </summary>
    public static FileDescriptor InternalBuildGeneratedFileFrom(byte[] descriptorData,
        FileDescriptor[] dependencies) {
      FileDescriptorProto proto = FileDescriptorProto.ParseFrom(descriptorData);
      return BuildFrom(proto, dependencies);      
    }
  }
}
