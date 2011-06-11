using System.IO;
using System.Text;
using Google.ProtocolBuffers.Serialization;
using NUnit.Framework;

namespace Google.ProtocolBuffers.CompatTests
{
    [TestFixture]
    public class JsonCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            StringWriter sw = new StringWriter();
            JsonFormatWriter.CreateInstance(sw)
                .Formatted()
                .WriteMessage(message);
            return sw.ToString();
        }

        protected override TBuilder DeerializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            JsonFormatReader.CreateInstance((string)message).Merge(builder);
            return builder;
        }
    }
}