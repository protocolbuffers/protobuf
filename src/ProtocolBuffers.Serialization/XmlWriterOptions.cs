using System;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Options available for the xml writer output
    /// </summary>
    [Flags]
    public enum XmlWriterOptions
    {
        /// <summary> Simple xml formatting with no attributes </summary>
        None,

        /// <summary> Writes the 'value' attribute on all enumerations with the numeric identifier </summary>
        OutputEnumValues = 0x1,

        /// <summary> Embeds array items into child &lt;item> elements </summary>
        OutputNestedArrays = 0x4,

        /// <summary> Outputs the 'type' attribute for compatibility with the <see cref="System.Runtime.Serialization.Json.JsonReaderWriterFactory">JsonReaderWriterFactory</see> </summary>
        /// <remarks> This option must, by nessessity, also enable NestedArrayItems </remarks>
        OutputJsonTypes = 0x8,
    }
}