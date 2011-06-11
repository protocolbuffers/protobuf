using System.IO;
using Google.ProtocolBuffers.Serialization;
using NUnit.Framework;

namespace Google.ProtocolBuffers.CompatTests
{
    [TestFixture]
    public class XmlCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            StringWriter text = new StringWriter();
            XmlFormatWriter writer = XmlFormatWriter.CreateInstance(text);
            writer.WriteMessage("root", message);
            return text.ToString();
        }

        protected override TBuilder DeerializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            XmlFormatReader reader = XmlFormatReader.CreateInstance((string)message);
            return reader.Merge("root", builder, registry);
        }
    }
}