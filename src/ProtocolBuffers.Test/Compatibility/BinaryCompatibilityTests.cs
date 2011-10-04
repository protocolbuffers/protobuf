using System;
#if SILVERLIGHT
using TestClass = Microsoft.VisualStudio.TestTools.UnitTesting.TestClassAttribute;
using Test = Microsoft.VisualStudio.TestTools.UnitTesting.TestMethodAttribute;
using Assert = Microsoft.VisualStudio.TestTools.UnitTesting.Assert;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
#endif

namespace Google.ProtocolBuffers.Compatibility
{
    [TestClass]
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