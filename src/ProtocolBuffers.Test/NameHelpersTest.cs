using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class NameHelpersTest {

    [Test]
    public void UnderscoresToPascalCase() {
      Assert.AreEqual("FooBar", NameHelpers.UnderscoresToPascalCase("Foo_bar"));
      Assert.AreEqual("FooBar", NameHelpers.UnderscoresToPascalCase("foo_bar"));
      Assert.AreEqual("Foo0Bar", NameHelpers.UnderscoresToPascalCase("Foo0bar"));
      Assert.AreEqual("FooBar", NameHelpers.UnderscoresToPascalCase("Foo_+_Bar"));
    }

    [Test]
    public void UnderscoresToCamelCase() {
      Assert.AreEqual("fooBar", NameHelpers.UnderscoresToCamelCase("Foo_bar"));
      Assert.AreEqual("fooBar", NameHelpers.UnderscoresToCamelCase("foo_bar"));
      Assert.AreEqual("foo0Bar", NameHelpers.UnderscoresToCamelCase("Foo0bar"));
      Assert.AreEqual("fooBar", NameHelpers.UnderscoresToCamelCase("Foo_+_Bar"));
    }

    [Test]
    public void StripSuffix() {
      string text = "FooBar";
      Assert.IsFalse(NameHelpers.StripSuffix(ref text, "Foo"));
      Assert.AreEqual("FooBar", text);
      Assert.IsTrue(NameHelpers.StripSuffix(ref text, "Bar"));
      Assert.AreEqual("Foo", text);
    }
  }
}