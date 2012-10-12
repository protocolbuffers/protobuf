using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// The exception raised when a recursion limit is reached while parsing input.
    /// </summary>
#if !SILVERLIGHT && !COMPACT_FRAMEWORK
    [Serializable]
#endif
    public sealed class RecursionLimitExceededException : FormatException
    {
        const string message = "Possible malicious message had too many levels of nesting.";
        
        internal RecursionLimitExceededException() : base(message)
        {
        }

#if !SILVERLIGHT && !COMPACT_FRAMEWORK
        private RecursionLimitExceededException(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context)
            : base(info, context)
        {
        }
#endif
    }
}
