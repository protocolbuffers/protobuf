using Google.ProtocolBuffers.ProtoGen;
using NUnit.Framework;

namespace Google.ProtocolBuffers.ProtoGen {
  [TestFixture]
  public class HelpersTest {

    [Test]
    public void UnderscoresToPascalCase() {
      Assert.AreEqual("FooBar", Helpers.UnderscoresToPascalCase("Foo_bar"));
      Assert.AreEqual("FooBar", Helpers.UnderscoresToPascalCase("foo_bar"));
      Assert.AreEqual("Foo0Bar", Helpers.UnderscoresToPascalCase("Foo0bar"));
      Assert.AreEqual("FooBar", Helpers.UnderscoresToPascalCase("Foo_+_Bar"));
    }

    [Test]
    public void UnderscoresToCamelCase() {
      Assert.AreEqual("fooBar", Helpers.UnderscoresToCamelCase("Foo_bar"));
      Assert.AreEqual("fooBar", Helpers.UnderscoresToCamelCase("foo_bar"));
      Assert.AreEqual("foo0Bar", Helpers.UnderscoresToCamelCase("Foo0bar"));
      Assert.AreEqual("fooBar", Helpers.UnderscoresToCamelCase("Foo_+_Bar"));
    }

    [Test]
    public void StripSuffix() {
      string text = "FooBar";
      Assert.IsFalse(Helpers.StripSuffix(ref text, "Foo"));
      Assert.AreEqual("FooBar", text);
      Assert.IsTrue(Helpers.StripSuffix(ref text, "Bar"));
      Assert.AreEqual("Foo", text);
    }
  }
}