using System;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Options available for the xml reader output
    /// </summary>
    [Flags]
    public enum XmlReaderOptions
    {
        /// <summary> Simple xml formatting with no attributes </summary>
        None,

        /// <summary> Requires that arrays items are nested in an &lt;item> element </summary>
        ReadNestedArrays = 1,
    }
}