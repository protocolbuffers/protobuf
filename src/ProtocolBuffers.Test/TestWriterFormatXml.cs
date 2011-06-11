using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using Google.ProtocolBuffers.Serialization;
using NUnit.Framework;
using Google.ProtocolBuffers.TestProtos;

namespace Google.ProtocolBuffers
{
    [TestFixture]
    public class TestWriterFormatXml
    {
        [Test]
        public void TestToXmlParseFromXml()
        {
            TestAllTypes msg = new TestAllTypes.Builder().SetDefaultBool(true).Build();
            string xml = msg.ToXml();
            Assert.AreEqual("<root><default_bool>true</default_bool></root>", xml);
            TestAllTypes copy = TestAllTypes.ParseFromXml(XmlReader.Create(new StringReader(xml)));
            Assert.IsTrue(copy.HasDefaultBool && copy.DefaultBool);
            Assert.AreEqual(msg, copy);
        }

        [Test]
        public void TestToXmlParseFromXmlWithRootName()
        {
            TestAllTypes msg = new TestAllTypes.Builder().SetDefaultBool(true).Build();
            string xml = msg.ToXml("message");
            Assert.AreEqual("<message><default_bool>true</default_bool></message>", xml);
            TestAllTypes copy = TestAllTypes.ParseFromXml("message", XmlReader.Create(new StringReader(xml)));
            Assert.IsTrue(copy.HasDefaultBool && copy.DefaultBool);
            Assert.AreEqual(msg, copy);
        }

        [Test]
        public void TestEmptyMessage()
        {
            TestXmlChild message = TestXmlChild.CreateBuilder()
                .Build();

            StringWriter sw = new StringWriter();
            XmlTextWriter xw = new XmlTextWriter(sw);

            //When we call message.WriteTo, we are responsible for the root element
            xw.WriteStartElement("root");
            message.WriteTo(XmlFormatWriter.CreateInstance(xw));
            xw.WriteEndElement();
            xw.Flush();

            string xml = sw.ToString();
            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlChild copy = rdr.Merge(TestXmlChild.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestRepeatedField()
        {
            TestXmlChild message = TestXmlChild.CreateBuilder()
                .AddOptions(EnumOptions.ONE)
                .AddOptions(EnumOptions.TWO)
                .Build();

            //Allow the writer to write the root element
            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();
            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlChild copy = rdr.Merge(TestXmlChild.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestNestedEmptyMessage()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetChild(TestXmlChild.CreateBuilder().Build())
                .Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();
            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestNestedMessage()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetChild(TestXmlChild.CreateBuilder().AddOptions(EnumOptions.TWO).Build())
                .Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();
            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestBooleanTypes()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetValid(true)
                .Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();
            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestFullMessage()
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
                .SetChild(TestXmlChild.CreateBuilder().AddOptions(EnumOptions.ONE).SetBinary(ByteString.CopyFrom(new byte[1])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.TWO).SetBinary(ByteString.CopyFrom(new byte[2])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.THREE).SetBinary(ByteString.CopyFrom(new byte[3])))
                .Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestFullMessageWithRichTypes()
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
                .SetChild(TestXmlChild.CreateBuilder().AddOptions(EnumOptions.ONE).SetBinary(ByteString.CopyFrom(new byte[1])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.TWO).SetBinary(ByteString.CopyFrom(new byte[2])))
                .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.THREE).SetBinary(ByteString.CopyFrom(new byte[3])))
                .Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw)
                .SetOptions(XmlWriterOptions.OutputNestedArrays | XmlWriterOptions.OutputEnumValues)
                .WriteMessage("root", message);

            string xml = sw.ToString();

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            rdr.Options = XmlReaderOptions.ReadNestedArrays;
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestFullMessageWithUnknownFields()
        {
            TestXmlMessage origial = TestXmlMessage.CreateBuilder()
                    .SetValid(true)
                    .SetText("text")
                    .AddTextlines("a")
                    .AddTextlines("b")
                    .AddTextlines("c")
                    .SetNumber(0x1010101010)
                    .AddNumbers(1)
                    .AddNumbers(2)
                    .AddNumbers(3)
                    .SetChild(TestXmlChild.CreateBuilder().AddOptions(EnumOptions.ONE).SetBinary(ByteString.CopyFrom(new byte[1])))
                    .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.TWO).SetBinary(ByteString.CopyFrom(new byte[2])))
                    .AddChildren(TestXmlMessage.Types.Children.CreateBuilder().AddOptions(EnumOptions.THREE).SetBinary(ByteString.CopyFrom(new byte[3])))
                .Build();
            TestXmlNoFields message = TestXmlNoFields.CreateBuilder().MergeFrom(origial.ToByteArray()).Build();

            Assert.AreEqual(0, message.AllFields.Count);

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw)
                .SetOptions(XmlWriterOptions.OutputNestedArrays | XmlWriterOptions.OutputEnumValues)
                .WriteMessage("root", message);

            string xml = sw.ToString();

            using (XmlReader x = XmlReader.Create(new StringReader(xml)))
            {
                x.MoveToContent();
                Assert.AreEqual(XmlNodeType.Element, x.NodeType);
                //should always be empty
                Assert.IsTrue(x.IsEmptyElement ||
                    (x.Read() && x.NodeType == XmlNodeType.EndElement)
                    );
            }

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            rdr.Options = XmlReaderOptions.ReadNestedArrays;
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(TestXmlMessage.DefaultInstance, copy);
        }
        [Test]
        public void TestMessageWithXmlText()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetText("<text>").Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestXmlWithWhitespace()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetText(" \t <- leading space and trailing -> \r\n\t").Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder()).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestXmlWithExtensionText()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionText, " extension text value ! ")
                .Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnitTestXmlSerializerTestProtoFile.RegisterAllExtensions(registry);

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder(), registry).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestXmlWithExtensionMessage()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionMessage,
                new TestXmlExtension.Builder().SetNumber(42).Build()).Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnitTestXmlSerializerTestProtoFile.RegisterAllExtensions(registry);

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder(), registry).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestXmlWithExtensionArray()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 100)
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 101)
                .AddExtension(UnitTestXmlSerializerTestProtoFile.ExtensionNumber, 102)
                .Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnitTestXmlSerializerTestProtoFile.RegisterAllExtensions(registry);

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder(), registry).Build();
            Assert.AreEqual(message, copy);
        }
        [Test]
        public void TestXmlWithExtensionEnum()
        {
            TestXmlMessage message = TestXmlMessage.CreateBuilder()
                .SetExtension(UnitTestXmlSerializerTestProtoFile.ExtensionEnum, EnumOptions.ONE)
                .Build();

            StringWriter sw = new StringWriter();
            XmlFormatWriter.CreateInstance(sw).WriteMessage("root", message);

            string xml = sw.ToString();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnitTestXmlSerializerTestProtoFile.RegisterAllExtensions(registry);

            XmlFormatReader rdr = XmlFormatReader.CreateInstance(xml);
            TestXmlMessage copy = rdr.Merge(TestXmlMessage.CreateBuilder(), registry).Build();
            Assert.AreEqual(message, copy);
        }
        [Test, ExpectedException(typeof(InvalidProtocolBufferException))]
        public void TestRecursiveLimit()
        {
            StringBuilder sb = new StringBuilder(8192);
            for (int i = 0; i < 80; i++)
                sb.Append("<child>");
            TestXmlRescursive msg = TestXmlRescursive.ParseFromXml("child", XmlReader.Create(new StringReader(sb.ToString())));
        }
    }
}
