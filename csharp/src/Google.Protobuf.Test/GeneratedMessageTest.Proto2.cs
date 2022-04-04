using Google.Protobuf.TestProtos.Proto2;
using Proto2 = Google.Protobuf.TestProtos.Proto2;
using NUnit.Framework;

using static Google.Protobuf.TestProtos.Proto2.UnittestExtensions;

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
            Assert.AreEqual(Proto2.TestAllTypes.Types.NestedEnum.Foo, message.OptionalNestedEnum);
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
            Assert.AreEqual(Proto2.TestAllTypes.OneofFieldOneofCase.None, message.OneofFieldCase);
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
            Assert.AreEqual(Proto2.TestAllTypes.Types.NestedEnum.Bar, message.DefaultNestedEnum);
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
        public void DefaultExtensionValues()
        {
            var message = new TestAllExtensions();
            Assert.AreEqual(false, message.GetExtension(OptionalBoolExtension));
            Assert.AreEqual(ByteString.Empty, message.GetExtension(OptionalBytesExtension));
            Assert.AreEqual(0.0, message.GetExtension(OptionalDoubleExtension));
            Assert.AreEqual(0, message.GetExtension(OptionalFixed32Extension));
            Assert.AreEqual(0L, message.GetExtension(OptionalFixed64Extension));
            Assert.AreEqual(0.0f, message.GetExtension(OptionalFloatExtension));
            Assert.AreEqual(ForeignEnum.ForeignFoo, message.GetExtension(OptionalForeignEnumExtension));
            Assert.IsNull(message.GetExtension(OptionalForeignMessageExtension));
            Assert.AreEqual(ImportEnum.ImportFoo, message.GetExtension(OptionalImportEnumExtension));
            Assert.IsNull(message.GetExtension(OptionalImportMessageExtension));
            Assert.AreEqual(0, message.GetExtension(OptionalInt32Extension));
            Assert.AreEqual(0L, message.GetExtension(OptionalInt64Extension));
            Assert.AreEqual(Proto2.TestAllTypes.Types.NestedEnum.Foo, message.GetExtension(OptionalNestedEnumExtension));
            Assert.IsNull(message.GetExtension(OptionalNestedMessageExtension));
            Assert.IsNull(message.GetExtension(OptionalPublicImportMessageExtension));
            Assert.AreEqual(0, message.GetExtension(OptionalSfixed32Extension));
            Assert.AreEqual(0L, message.GetExtension(OptionalSfixed64Extension));
            Assert.AreEqual(0, message.GetExtension(OptionalSint32Extension));
            Assert.AreEqual(0L, message.GetExtension(OptionalSint64Extension));
            Assert.AreEqual("", message.GetExtension(OptionalStringExtension));
            Assert.AreEqual(0U, message.GetExtension(OptionalUint32Extension));
            Assert.AreEqual(0UL, message.GetExtension(OptionalUint64Extension));

            // Repeated fields
            Assert.IsNull(message.GetExtension(RepeatedBoolExtension));
            Assert.IsNull(message.GetExtension(RepeatedBytesExtension));
            Assert.IsNull(message.GetExtension(RepeatedDoubleExtension));
            Assert.IsNull(message.GetExtension(RepeatedFixed32Extension));
            Assert.IsNull(message.GetExtension(RepeatedFixed64Extension));
            Assert.IsNull(message.GetExtension(RepeatedFloatExtension));
            Assert.IsNull(message.GetExtension(RepeatedForeignEnumExtension));
            Assert.IsNull(message.GetExtension(RepeatedForeignMessageExtension));
            Assert.IsNull(message.GetExtension(RepeatedImportEnumExtension));
            Assert.IsNull(message.GetExtension(RepeatedImportMessageExtension));
            Assert.IsNull(message.GetExtension(RepeatedNestedEnumExtension));
            Assert.IsNull(message.GetExtension(RepeatedNestedMessageExtension));
            Assert.IsNull(message.GetExtension(RepeatedSfixed32Extension));
            Assert.IsNull(message.GetExtension(RepeatedSfixed64Extension));
            Assert.IsNull(message.GetExtension(RepeatedSint32Extension));
            Assert.IsNull(message.GetExtension(RepeatedSint64Extension));
            Assert.IsNull(message.GetExtension(RepeatedStringExtension));
            Assert.IsNull(message.GetExtension(RepeatedUint32Extension));
            Assert.IsNull(message.GetExtension(RepeatedUint64Extension));

            // Oneof fields
            Assert.AreEqual(0, message.GetExtension(OneofUint32Extension));
            Assert.AreEqual("", message.GetExtension(OneofStringExtension));
            Assert.AreEqual(ByteString.Empty, message.GetExtension(OneofBytesExtension));
            Assert.IsNull(message.GetExtension(OneofNestedMessageExtension));

            Assert.AreEqual(true, message.GetExtension(DefaultBoolExtension));
            Assert.AreEqual(ByteString.CopyFromUtf8("world"), message.GetExtension(DefaultBytesExtension));
            Assert.AreEqual("123", message.GetExtension(DefaultCordExtension));
            Assert.AreEqual(52e3, message.GetExtension(DefaultDoubleExtension));
            Assert.AreEqual(47, message.GetExtension(DefaultFixed32Extension));
            Assert.AreEqual(48, message.GetExtension(DefaultFixed64Extension));
            Assert.AreEqual(51.5, message.GetExtension(DefaultFloatExtension));
            Assert.AreEqual(ForeignEnum.ForeignBar, message.GetExtension(DefaultForeignEnumExtension));
            Assert.AreEqual(ImportEnum.ImportBar, message.GetExtension(DefaultImportEnumExtension));
            Assert.AreEqual(41, message.GetExtension(DefaultInt32Extension));
            Assert.AreEqual(42, message.GetExtension(DefaultInt64Extension));
            Assert.AreEqual(Proto2.TestAllTypes.Types.NestedEnum.Bar, message.GetExtension(DefaultNestedEnumExtension));
            Assert.AreEqual(49, message.GetExtension(DefaultSfixed32Extension));
            Assert.AreEqual(-50, message.GetExtension(DefaultSfixed64Extension));
            Assert.AreEqual(-45, message.GetExtension(DefaultSint32Extension));
            Assert.AreEqual(46, message.GetExtension(DefaultSint64Extension));
            Assert.AreEqual("hello", message.GetExtension(DefaultStringExtension));
            Assert.AreEqual("abc", message.GetExtension(DefaultStringPieceExtension));
            Assert.AreEqual(43, message.GetExtension(DefaultUint32Extension));
            Assert.AreEqual(44, message.GetExtension(DefaultUint64Extension));

            Assert.False(message.HasExtension(DefaultBoolExtension));
            Assert.False(message.HasExtension(DefaultBytesExtension));
            Assert.False(message.HasExtension(DefaultCordExtension));
            Assert.False(message.HasExtension(DefaultDoubleExtension));
            Assert.False(message.HasExtension(DefaultFixed32Extension));
            Assert.False(message.HasExtension(DefaultFixed64Extension));
            Assert.False(message.HasExtension(DefaultFloatExtension));
            Assert.False(message.HasExtension(DefaultForeignEnumExtension));
            Assert.False(message.HasExtension(DefaultImportEnumExtension));
            Assert.False(message.HasExtension(DefaultInt32Extension));
            Assert.False(message.HasExtension(DefaultInt64Extension));
            Assert.False(message.HasExtension(DefaultNestedEnumExtension));
            Assert.False(message.HasExtension(DefaultSfixed32Extension));
            Assert.False(message.HasExtension(DefaultSfixed64Extension));
            Assert.False(message.HasExtension(DefaultSint32Extension));
            Assert.False(message.HasExtension(DefaultSint64Extension));
            Assert.False(message.HasExtension(DefaultStringExtension));
            Assert.False(message.HasExtension(DefaultStringPieceExtension));
            Assert.False(message.HasExtension(DefaultUint32Extension));
            Assert.False(message.HasExtension(DefaultUint64Extension));
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

        /// <summary>
        /// Code was accidentally left in message parser that threw exceptions when missing required fields after parsing.
        /// We've decided to not throw exceptions on missing fields, instead leaving it up to the consumer how they
        /// want to check and handle missing fields.
        /// </summary>
        [Test]
        public void RequiredFieldsNoThrow()
        {
            Assert.DoesNotThrow(() => MessageParsingHelpers.AssertReadingMessage(TestRequired.Parser, new byte[0], m => { }));
            Assert.DoesNotThrow(() => MessageParsingHelpers.AssertReadingMessage(TestRequired.Parser as MessageParser, new byte[0], m => { }));
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

            message.GetOrInitializeExtension(TestRequired.Extensions.Multi);

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

            message.GetOrInitializeExtension(UnittestExtensions.RepeatedBoolExtension).Add(true);

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

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertRoundtrip(Proto2.TestAllTypes.Parser, message);
        }

        [Test]
        public void RoundTrip_ExtensionGroups()
        {
            var message = new TestAllExtensions();
            message.SetExtension(UnittestExtensions.OptionalGroupExtension, new OptionalGroup_extension { A = 10 });
            message.GetOrInitializeExtension(UnittestExtensions.RepeatedGroupExtension).AddRange(new[]
            {
                new RepeatedGroup_extension { A = 10 },
                new RepeatedGroup_extension { A = 20 },
                new RepeatedGroup_extension { A = 30 }
            });

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertRoundtrip(
                TestAllExtensions.Parser.WithExtensionRegistry(new ExtensionRegistry() { UnittestExtensions.OptionalGroupExtension, UnittestExtensions.RepeatedGroupExtension }),
                message);
        }

        [Test]
        public void RoundTrip_NestedExtensionGroup()
        {
            var message = new TestGroupExtension();
            message.SetExtension(TestNestedExtension.Extensions.OptionalGroupExtension, new TestNestedExtension.Types.OptionalGroup_extension { A = 10 });

            MessageParsingHelpers.AssertWritingMessage(message);
            
            MessageParsingHelpers.AssertRoundtrip(
                TestGroupExtension.Parser.WithExtensionRegistry(new ExtensionRegistry() { TestNestedExtension.Extensions.OptionalGroupExtension }),
                message);
        }

        [Test]
        public void RoundTrip_ParseUsingCodedInput()
        {
            var message = new TestAllExtensions();
            message.SetExtension(UnittestExtensions.OptionalBoolExtension, true);
            byte[] bytes = message.ToByteArray();
            using (CodedInputStream input = new CodedInputStream(bytes))
            {
                var parsed = TestAllExtensions.Parser.WithExtensionRegistry(new ExtensionRegistry() { UnittestExtensions.OptionalBoolExtension }).ParseFrom(input);
                Assert.AreEqual(message, parsed);
            }
        }
    }
}
