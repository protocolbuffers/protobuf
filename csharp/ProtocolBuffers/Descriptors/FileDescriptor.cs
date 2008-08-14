using System;
using Google.ProtocolBuffers.DescriptorProtos;
using System.Collections.Generic;
namespace Google.ProtocolBuffers.Descriptors {
  public class FileDescriptor : DescriptorBase<FileDescriptorProto, FileOptions> {

    private readonly IList<MessageDescriptor> messageTypes;
    private readonly IList<EnumDescriptor> enumTypes;
    private readonly IList<ServiceDescriptor> services;
    private readonly IList<FieldDescriptor> extensions;
    private readonly IList<FileDescriptor> dependencies;
    private readonly DescriptorPool pool;

    public FileDescriptor(FileDescriptorProto proto, FileDescriptor file) : base(proto, file) {
    }

    /// <summary>
    /// The package as declared in the .proto file. This may or may not
    /// be equivalent to the .NET namespace of the generated classes.
    /// </summary>
    public string Package {
      get { return Proto.Package; }
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

    public static FileDescriptor BuildFrom(FileDescriptorProto proto,
        FileDescriptor[] dependencies) {
      throw new NotImplementedException();
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
