using System;

namespace Google.ProtocolBuffers.Compatibility
{
    public class BinaryCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            byte[] bresult = message.ToByteArray();
            return Convert.ToBase64String(bresult);
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            return builder.MergeFrom((byte[])Convert.FromBase64String((string)message), registry);
        }
    }
}