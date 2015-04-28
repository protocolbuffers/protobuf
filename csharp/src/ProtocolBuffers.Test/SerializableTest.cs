#if !NOSERIALIZABLE
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.Hosting;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Google.ProtocolBuffers
{
    [TestClass]
    public class SerializableTest
    {
        /// <summary>
        /// Just keep it from even compiling if we these objects don't implement the expected interface.
        /// </summary>
        public static readonly ISerializable CompileTimeCheckSerializableMessage = TestXmlMessage.DefaultInstance;
        public static readonly ISerializable CompileTimeCheckSerializableBuilder = new TestXmlMessage.Builder();

        [TestMethod]
        [Ignore] // Serialization hasn't been reimplemented yet
        public void TestPlainMessage()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .SetText("text")
                .AddTextlines("a")
                .AddTextlines("b")
                .AddTextlines("c")
                .SetNumber(0x1010101010)
                .AddNumbers(1)
                .AddNumbers(2)
                .AddNumbers(3)
                .SetChild(TestXmlChild.CreateBuilder()
                                      .AddOptions(EnumOptions.ONE)
                                      .SetBinary(ByteString.CopyFrom(new byte[1])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder()
                                                          .AddOptions(EnumOptions.TWO)
                                                          .SetBinary(ByteString.CopyFrom(new byte[2])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder()
                                                          .AddOptions(EnumOptions.THREE)
                                                          .SetBinary(ByteString.CopyFrom(new byte[3])))
                .Build();

            MemoryStream ms = new MemoryStream();
            new BinaryFormatter().Serialize(ms, message);

            ms.Position = 0;
            TestXmlMessage copy = (TestXmlMessage)new BinaryFormatter().Deserialize(ms);

            Assert.AreEqual(message, copy);
        }

        [TestMethod]
        [Ignore] // Serialization hasn't been reimplemented yet
        public void TestMessageWithExtensions()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .SetText("text")
                .AddTextlines("a")
                .AddTextlines("b")
                .AddTextlines("c")
                .SetNumber(0x1010101010)
                .AddNumbers(1)
                .AddNumbers(2)
                .AddNumbers(3)
                .SetChild(TestXmlChild.CreateBuilder()
                                      .AddOptions(EnumOptions.ONE)
                                      .SetBinary(ByteString.CopyFrom(new byte[1])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder()
                                                          .AddOptions(EnumOptions.TWO)
                                                          .SetBinary(ByteString.CopyFrom(new byte[2])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder()
                                                          .AddOptions(EnumOptions.THREE)
                                                          .SetBinary(ByteString.CopyFrom(new byte[3])))
                .SetExtension(UnittestExtrasXmltest.ExtensionText, " extension text value ! ")
                .SetExtension(UnittestExtrasXmltest.ExtensionMessage, new TestXmlExtension.Builder().SetNumber(42).Build())
                .AddExtension(UnittestExtrasXmltest.ExtensionNumber, 100)
                .AddExtension(UnittestExtrasXmltest.ExtensionNumber, 101)
                .AddExtension(UnittestExtrasXmltest.ExtensionNumber, 102)
                .SetExtension(UnittestExtrasXmltest.ExtensionEnum, EnumOptions.ONE)
                .Build();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestExtrasXmltest.RegisterAllExtensions(registry);

            MemoryStream ms = new MemoryStream();
            new BinaryFormatter().Serialize(ms, message);

            ms.Position = 0;
            //you need to provide the extension registry as context to the serializer
            BinaryFormatter bff = new BinaryFormatter(null, new StreamingContext(StreamingContextStates.All, registry));
            TestXmlMessage copy = (TestXmlMessage)bff.Deserialize(ms);

            // And all extensions will be defined.
            Assert.AreEqual(message, copy);
        }

        [TestMethod]
        [Ignore] // Serialization hasn't been reimplemented yet
        public void TestPlainBuilder()
        {
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .SetText("text")
                .AddTextlines("a")
                .AddTextlines("b")
                .AddTextlines("c")
                .SetNumber(0x1010101010)
                .AddNumbers(1)
                .AddNumbers(2)
                .AddNumbers(3)
                .SetChild(TestXmlChild.CreateBuilder()
                                      .AddOptions(EnumOptions.ONE)
                                      .SetBinary(ByteString.CopyFrom(new byte[1])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder()
                                                          .AddOptions(EnumOptions.TWO)
                                                          .SetBinary(ByteString.CopyFrom(new byte[2])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder()
                                                          .AddOptions(EnumOptions.THREE)
                                                          .SetBinary(ByteString.CopyFrom(new byte[3])))
                ;

            MemoryStream ms = new MemoryStream();
            new BinaryFormatter().Serialize(ms, builder);

            ms.Position = 0;
            TestXmlMessage.Builder copy = (TestXmlMessage.Builder)new BinaryFormatter().Deserialize(ms);

            Assert.AreEqual(builder.Build(), copy.Build());
        }

        [TestMethod]
        [Ignore] // Serialization hasn't been reimplemented yet
        public void TestBuilderWithExtensions()
        {
            TestXmlMessage.Builder builder = TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .SetText("text")
                .AddTextlines("a")
                .AddTextlines("b")
                .AddTextlines("c")
                .SetNumber(0x1010101010)
                .AddNumbers(1)
                .AddNumbers(2)
                .AddNumbers(3)
                .SetChild(TestXmlChild.CreateBuilder()
                                      .AddOptions(EnumOptions.ONE)
                                      .SetBinary(ByteString.CopyFrom(new byte[1])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder()
                                                          .AddOptions(EnumOptions.TWO)
                                                          .SetBinary(ByteString.CopyFrom(new byte[2])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder()
                                                          .AddOptions(EnumOptions.THREE)
                                                          .SetBinary(ByteString.CopyFrom(new byte[3])))
                .SetExtension(UnittestExtrasXmltest.ExtensionText, " extension text value ! ")
                .SetExtension(UnittestExtrasXmltest.ExtensionMessage, new TestXmlExtension.Builder().SetNumber(42).Build())
                .AddExtension(UnittestExtrasXmltest.ExtensionNumber, 100)
                .AddExtension(UnittestExtrasXmltest.ExtensionNumber, 101)
                .AddExtension(UnittestExtrasXmltest.ExtensionNumber, 102)
                .SetExtension(UnittestExtrasXmltest.ExtensionEnum, EnumOptions.ONE)
                ;

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestExtrasXmltest.RegisterAllExtensions(registry);

            MemoryStream ms = new MemoryStream();
            new BinaryFormatter().Serialize(ms, builder);

            ms.Position = 0;
            //you need to provide the extension registry as context to the serializer
            BinaryFormatter bff = new BinaryFormatter(null, new StreamingContext(StreamingContextStates.All, registry));
            TestXmlMessage.Builder copy = (TestXmlMessage.Builder)bff.Deserialize(ms);

            // And all extensions will be defined.
            Assert.AreEqual(builder.Build(), copy.Build());
        }
    }
}
#endif