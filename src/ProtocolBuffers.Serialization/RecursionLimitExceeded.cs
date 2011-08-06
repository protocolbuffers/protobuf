using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// The exception raised when a recursion limit is reached while parsing input.
    /// </summary>
#if !SILVERLIGHT2
    [Serializable]
#endif
    public sealed class RecursionLimitExceededException : FormatException
    {
        const string message = "Possible malicious message had too many levels of nesting.";
        
        internal RecursionLimitExceededException() : base(message)
        {
        }

#if !SILVERLIGHT2
        private RecursionLimitExceededException(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context)
            : base(info, context)
        {
        }
#endif
    }
}
