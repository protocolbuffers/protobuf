using System.IO;
using Google.ProtocolBuffers.Serialization;

namespace Google.ProtocolBuffers.Compatibility
{
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