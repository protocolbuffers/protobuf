using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Serialization;
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
    public class DictionaryCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            DictionaryWriter writer = new DictionaryWriter();
            writer.WriteMessage(message);
            return writer.ToDictionary();
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            new DictionaryReader((IDictionary<string, object>)message).Merge(builder);
            return builder;
        }

        protected override void AssertOutputEquals(object lhs, object rhs)
        {
            IDictionary<string, object> left = (IDictionary<string, object>)lhs;
            IDictionary<string, object> right = (IDictionary<string, object>)rhs;

            Assert.AreEqual(
                String.Join(",", new List<string>(left.Keys).ToArray()),
                String.Join(",", new List<string>(right.Keys).ToArray())
            );
        }
    }
}