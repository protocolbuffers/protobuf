using Google.ProtocolBuffers.DescriptorProtos;
namespace Google.ProtocolBuffers.Descriptors {
  public class FileDescriptor : DescriptorBase<FileDescriptorProto, FileOptions> {
    public FileDescriptor(FileDescriptorProto proto, FileDescriptor file) : base(proto, file) {
    }
  }
}
