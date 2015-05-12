using System.IO;
using Google.ProtocolBuffers.Serialization;
using NUnit.Framework;

namespace Google.ProtocolBuffers.Compatibility
{
    [TestFixture]
    public class JsonCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            StringWriter sw = new StringWriter();
            JsonFormatWriter.CreateInstance(sw)
                .WriteMessage(message);
            return sw.ToString();
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            JsonFormatReader.CreateInstance((string)message).Merge(builder);
            return builder;
        }
    }

    [TestFixture]
    public class JsonCompatibilityFormattedTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            StringWriter sw = new StringWriter();
            JsonFormatWriter.CreateInstance(sw)
                .Formatted()
                .WriteMessage(message);
            return sw.ToString();
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            JsonFormatReader.CreateInstance((string)message).Merge(builder);
            return builder;
        }
    }
}