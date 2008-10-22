using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;
using NUnit.Framework;

namespace Google.ProtocolBuffers.ProtoGen {
  [TestFixture]
  public class DescriptorUtilTest {

    [Test]
    public void ExplicitNamespace() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder {
        Name = "x", Package = "pack", Options = new FileOptions.Builder().SetExtension(CSharpOptions.CSharpNamespace, "Foo.Bar").Build()
      }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("Foo.Bar", DescriptorUtil.GetNamespace(descriptor));
    }

    [Test]
    public void NoNamespaceFallsBackToPackage() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "x", Package = "pack" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("pack", DescriptorUtil.GetNamespace(descriptor));
    }

    [Test]
    public void NoNamespaceOrPackageFallsBackToEmptyString() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "x" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("", DescriptorUtil.GetNamespace(descriptor));
    }

    [Test]
    public void ExplicitlyNamedFileClass() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder {
        Name = "x", Options = new FileOptions.Builder().SetExtension(CSharpOptions.CSharpUmbrellaClassname, "Foo").Build()
      }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("Foo", DescriptorUtil.GetUmbrellaClassName(descriptor));
    }

    [Test]
    public void ImplicitFileClassWithProtoSuffix() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "foo_bar.proto" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("FooBar", DescriptorUtil.GetUmbrellaClassName(descriptor));
    }

    [Test]
    public void ImplicitFileClassWithProtoDevelSuffix() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "foo_bar.protodevel" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("FooBar", DescriptorUtil.GetUmbrellaClassName(descriptor));
    }

    [Test]
    public void ImplicitFileClassWithNoSuffix() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "foo_bar" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("FooBar", DescriptorUtil.GetUmbrellaClassName(descriptor));
    }

    [Test]
    public void ImplicitFileClassWithDirectoryStructure() {
      FileDescriptorProto proto = new FileDescriptorProto.Builder { Name = "x/y/foo_bar" }.Build();
      FileDescriptor descriptor = FileDescriptor.BuildFrom(proto, null);
      Assert.AreEqual("FooBar", DescriptorUtil.GetUmbrellaClassName(descriptor));
    }
  }
}
