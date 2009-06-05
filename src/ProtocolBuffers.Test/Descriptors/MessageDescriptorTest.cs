using NUnit.Framework;
using Google.ProtocolBuffers.TestProtos;

namespace Google.ProtocolBuffers.Descriptors {

  [TestFixture]
  public class MessageDescriptorTest {
    [Test]
    public void FindPropertyWithDefaultName() {
      Assert.AreSame(OptionsMessage.Descriptor.FindFieldByNumber(OptionsMessage.NormalFieldNumber),
          OptionsMessage.Descriptor.FindFieldByPropertyName("Normal"));
    }

    [Test]
    public void FindPropertyWithAutoModifiedName() {
      Assert.AreSame(OptionsMessage.Descriptor.FindFieldByNumber(OptionsMessage.OptionsMessage_FieldNumber),
          OptionsMessage.Descriptor.FindFieldByPropertyName("OptionsMessage_"));
    }

    [Test]
    public void FindPropertyWithCustomizedName() {
      Assert.AreSame(OptionsMessage.Descriptor.FindFieldByNumber(OptionsMessage.CustomNameFieldNumber),
          OptionsMessage.Descriptor.FindFieldByPropertyName("CustomName"));
    }

    [Test]
    public void FindPropertyWithInvalidName() {
      Assert.IsNull(OptionsMessage.Descriptor.FindFieldByPropertyName("Bogus"));
    }
  }
}
