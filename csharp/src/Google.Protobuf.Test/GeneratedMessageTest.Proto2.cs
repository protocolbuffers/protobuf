using Google.Protobuf.TestProtos.Proto2;
using NUnit.Framework;

namespace Google.Protobuf
{
    /// <summary>
    /// Tests around the generated TestAllTypes message in unittest.proto
    /// </summary>
    public partial class GeneratedMessageTest
    {
        [Test]
        public void DefaultProto2Values()
        {
            var message = new TestAllTypes();
            Assert.AreEqual(false, message.OptionalBool);
            Assert.AreEqual(ByteString.Empty, message.OptionalBytes);
            Assert.AreEqual(0.0, message.OptionalDouble);
            Assert.AreEqual(0, message.OptionalFixed32);
            Assert.AreEqual(0L, message.OptionalFixed64);
            Assert.AreEqual(0.0f, message.OptionalFloat);
            Assert.AreEqual(ForeignEnum.ForeignFoo, message.OptionalForeignEnum);
            Assert.IsNull(message.OptionalForeignMessage);
            Assert.AreEqual(ImportEnum.ImportFoo, message.OptionalImportEnum);
            Assert.IsNull(message.OptionalImportMessage);
            Assert.AreEqual(0, message.OptionalInt32);
            Assert.AreEqual(0L, message.OptionalInt64);
            Assert.AreEqual(TestAllTypes.Types.NestedEnum.Foo, message.OptionalNestedEnum);
            Assert.IsNull(message.OptionalNestedMessage);
            Assert.IsNull(message.OptionalPublicImportMessage);
            Assert.AreEqual(0, message.OptionalSfixed32);
            Assert.AreEqual(0L, message.OptionalSfixed64);
            Assert.AreEqual(0, message.OptionalSint32);
            Assert.AreEqual(0L, message.OptionalSint64);
            Assert.AreEqual("", message.OptionalString);
            Assert.AreEqual(0U, message.OptionalUint32);
            Assert.AreEqual(0UL, message.OptionalUint64);

            // Repeated fields
            Assert.AreEqual(0, message.RepeatedBool.Count);
            Assert.AreEqual(0, message.RepeatedBytes.Count);
            Assert.AreEqual(0, message.RepeatedDouble.Count);
            Assert.AreEqual(0, message.RepeatedFixed32.Count);
            Assert.AreEqual(0, message.RepeatedFixed64.Count);
            Assert.AreEqual(0, message.RepeatedFloat.Count);
            Assert.AreEqual(0, message.RepeatedForeignEnum.Count);
            Assert.AreEqual(0, message.RepeatedForeignMessage.Count);
            Assert.AreEqual(0, message.RepeatedImportEnum.Count);
            Assert.AreEqual(0, message.RepeatedImportMessage.Count);
            Assert.AreEqual(0, message.RepeatedNestedEnum.Count);
            Assert.AreEqual(0, message.RepeatedNestedMessage.Count);
            Assert.AreEqual(0, message.RepeatedSfixed32.Count);
            Assert.AreEqual(0, message.RepeatedSfixed64.Count);
            Assert.AreEqual(0, message.RepeatedSint32.Count);
            Assert.AreEqual(0, message.RepeatedSint64.Count);
            Assert.AreEqual(0, message.RepeatedString.Count);
            Assert.AreEqual(0, message.RepeatedUint32.Count);
            Assert.AreEqual(0, message.RepeatedUint64.Count);

            // Oneof fields
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.None, message.OneofFieldCase);
            Assert.AreEqual(0, message.OneofUint32);
            Assert.AreEqual("", message.OneofString);
            Assert.AreEqual(ByteString.Empty, message.OneofBytes);
            Assert.IsNull(message.OneofNestedMessage);

            Assert.AreEqual(true, message.DefaultBool);
            Assert.AreEqual(ByteString.CopyFromUtf8("world"), message.DefaultBytes);
            Assert.AreEqual("123", message.DefaultCord);
            Assert.AreEqual(52e3, message.DefaultDouble);
            Assert.AreEqual(47, message.DefaultFixed32);
            Assert.AreEqual(48, message.DefaultFixed64);
            Assert.AreEqual(51.5, message.DefaultFloat);
            Assert.AreEqual(ForeignEnum.ForeignBar, message.DefaultForeignEnum);
            Assert.AreEqual(ImportEnum.ImportBar, message.DefaultImportEnum);
            Assert.AreEqual(41, message.DefaultInt32);
            Assert.AreEqual(42, message.DefaultInt64);
            Assert.AreEqual(TestAllTypes.Types.NestedEnum.Bar, message.DefaultNestedEnum);
            Assert.AreEqual(49, message.DefaultSfixed32);
            Assert.AreEqual(-50, message.DefaultSfixed64);
            Assert.AreEqual(-45, message.DefaultSint32);
            Assert.AreEqual(46, message.DefaultSint64);
            Assert.AreEqual("hello", message.DefaultString);
            Assert.AreEqual("abc", message.DefaultStringPiece);
            Assert.AreEqual(43, message.DefaultUint32);
            Assert.AreEqual(44, message.DefaultUint64);

            Assert.False(message.HasDefaultBool);
            Assert.False(message.HasDefaultBytes);
            Assert.False(message.HasDefaultCord);
            Assert.False(message.HasDefaultDouble);
            Assert.False(message.HasDefaultFixed32);
            Assert.False(message.HasDefaultFixed64);
            Assert.False(message.HasDefaultFloat);
            Assert.False(message.HasDefaultForeignEnum);
            Assert.False(message.HasDefaultImportEnum);
            Assert.False(message.HasDefaultInt32);
            Assert.False(message.HasDefaultInt64);
            Assert.False(message.HasDefaultNestedEnum);
            Assert.False(message.HasDefaultSfixed32);
            Assert.False(message.HasDefaultSfixed64);
            Assert.False(message.HasDefaultSint32);
            Assert.False(message.HasDefaultSint64);
            Assert.False(message.HasDefaultString);
            Assert.False(message.HasDefaultStringPiece);
            Assert.False(message.HasDefaultUint32);
            Assert.False(message.HasDefaultUint64);
        }

        [Test]
        public void FieldPresence()
        {
            var message = new TestAllTypes();

            Assert.False(message.HasOptionalBool);
            Assert.False(message.OptionalBool);

            message.OptionalBool = true;

            Assert.True(message.HasOptionalBool);
            Assert.True(message.OptionalBool);

            message.OptionalBool = false;

            Assert.True(message.HasOptionalBool);
            Assert.False(message.OptionalBool);

            message.ClearOptionalBool();

            Assert.False(message.HasOptionalBool);
            Assert.False(message.OptionalBool);

            Assert.False(message.HasDefaultBool);
            Assert.True(message.DefaultBool);

            message.DefaultBool = false;

            Assert.True(message.HasDefaultBool);
            Assert.False(message.DefaultBool);

            message.DefaultBool = true;

            Assert.True(message.HasDefaultBool);
            Assert.True(message.DefaultBool);

            message.ClearDefaultBool();

            Assert.False(message.HasDefaultBool);
            Assert.True(message.DefaultBool);
        }

        [Test]
        public void RequiredFields()
        {
            var message = new TestRequired();
            Assert.False(message.IsInitialized());

            message.A = 1;
            message.B = 2;
            message.C = 3;

            Assert.True(message.IsInitialized());
        }

        [Test]
        public void RequiredFieldsInExtensions()
        {
            var message = new TestAllExtensions();
            Assert.True(message.IsInitialized());

            message.SetExtension(TestRequired.Extensions.Single, new TestRequired());

            Assert.False(message.IsInitialized());

            var extensionMessage = message.GetExtension(TestRequired.Extensions.Single);
            extensionMessage.A = 1;
            extensionMessage.B = 2;
            extensionMessage.C = 3;

            Assert.True(message.IsInitialized());

            message.GetOrRegisterExtension(TestRequired.Extensions.Multi);

            Assert.True(message.IsInitialized());

            message.GetExtension(TestRequired.Extensions.Multi).Add(new TestRequired());

            Assert.False(message.IsInitialized());

            extensionMessage = message.GetExtension(TestRequired.Extensions.Multi)[0];
            extensionMessage.A = 1;
            extensionMessage.B = 2;
            extensionMessage.C = 3;

            Assert.True(message.IsInitialized());

            message.SetExtension(UnittestExtensions.OptionalBoolExtension, true);

            Assert.True(message.IsInitialized());

            message.GetOrRegisterExtension(UnittestExtensions.RepeatedBoolExtension).Add(true);

            Assert.True(message.IsInitialized());
        }

        [Test]
        public void RequiredFieldInNestedMessageMapValue()
        {
            var message = new TestRequiredMap();
            message.Foo.Add(0, new TestRequiredMap.Types.NestedMessage());

            Assert.False(message.IsInitialized());

            message.Foo[0].RequiredInt32 = 12;

            Assert.True(message.IsInitialized());
        }

        [Test]
        public void RoundTrip_Groups()
        {
            var message = new TestAllTypes
            {
                OptionalGroup = new TestAllTypes.Types.OptionalGroup
                {
                    A = 10
                },
                RepeatedGroup =
                {
                    new TestAllTypes.Types.RepeatedGroup { A = 10 },
                    new TestAllTypes.Types.RepeatedGroup { A = 20 },
                    new TestAllTypes.Types.RepeatedGroup { A = 30 }
                }
            };

            byte[] bytes = message.ToByteArray();
            TestAllTypes parsed = TestAllTypes.Parser.ParseFrom(bytes);
            Assert.AreEqual(message, parsed);
        }

        [Test]
        public void RoundTrip_ExtensionGroups()
        {
            var message = new TestAllExtensions();
            message.SetExtension(UnittestExtensions.OptionalGroupExtension, new OptionalGroup_extension { A = 10 });
            message.GetOrRegisterExtension(UnittestExtensions.RepeatedGroupExtension).AddRange(new[]
            {
                new RepeatedGroup_extension { A = 10 },
                new RepeatedGroup_extension { A = 20 },
                new RepeatedGroup_extension { A = 30 }
            });

            byte[] bytes = message.ToByteArray();
            TestAllExtensions extendable_parsed = TestAllExtensions.Parser.WithExtensionRegistry(new ExtensionRegistry() { UnittestExtensions.OptionalGroupExtension, UnittestExtensions.RepeatedGroupExtension }).ParseFrom(bytes);
            Assert.AreEqual(message, extendable_parsed);
        }

        [Test]
        public void RoundTrip_NestedExtensionGroup()
        {
            var message = new TestGroupExtension();
            message.SetExtension(TestNestedExtension.Extensions.OptionalGroupExtension, new TestNestedExtension.Types.OptionalGroup_extension { A = 10 });

            byte[] bytes = message.ToByteArray();
            TestGroupExtension extendable_parsed = TestGroupExtension.Parser.WithExtensionRegistry(new ExtensionRegistry() { TestNestedExtension.Extensions.OptionalGroupExtension }).ParseFrom(bytes);
            Assert.AreEqual(message, extendable_parsed);
        }
    }
}
