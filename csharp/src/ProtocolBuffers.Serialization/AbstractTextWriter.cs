using System;
using System.Xml;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Provides a base class for text writers
    /// </summary>
    public abstract class AbstractTextWriter : AbstractWriter
    {
        /// <summary>
        /// Encodes raw bytes to be written to the stream
        /// </summary>
        protected virtual string EncodeBytes(ByteString bytes)
        {
            return bytes.ToBase64();
        }

        /// <summary>
        /// Writes a typed field as a text value
        /// </summary>
        protected abstract void WriteAsText(string field, string textValue, object typedValue);

        /// <summary>
        /// Writes a String value
        /// </summary>
        protected override void Write(string field, string value)
        {
            WriteAsText(field, value, value);
        }

        /// <summary>
        /// Writes a Boolean value
        /// </summary>
        protected override void Write(string field, bool value)
        {
            WriteAsText(field, XmlConvert.ToString(value), value);
        }

        /// <summary>
        /// Writes a Int32 value
        /// </summary>
        protected override void Write(string field, int value)
        {
            WriteAsText(field, XmlConvert.ToString(value), value);
        }

        /// <summary>
        /// Writes a UInt32 value
        /// </summary>
        protected override void Write(string field, uint value)
        {
            WriteAsText(field, XmlConvert.ToString(value), value);
        }

        /// <summary>
        /// Writes a Int64 value
        /// </summary>
        protected override void Write(string field, long value)
        {
            WriteAsText(field, XmlConvert.ToString(value), value);
        }

        /// <summary>
        /// Writes a UInt64 value
        /// </summary>
        protected override void Write(string field, ulong value)
        {
            WriteAsText(field, XmlConvert.ToString(value), value);
        }

        /// <summary>
        /// Writes a Single value
        /// </summary>
        protected override void Write(string field, float value)
        {
            WriteAsText(field, XmlConvert.ToString(value), value);
        }

        /// <summary>
        /// Writes a Double value
        /// </summary>
        protected override void Write(string field, double value)
        {
            WriteAsText(field, XmlConvert.ToString(value), value);
        }

        /// <summary>
        /// Writes a set of bytes
        /// </summary>
        protected override void Write(string field, ByteString value)
        {
            WriteAsText(field, EncodeBytes(value), value);
        }

        /// <summary>
        /// Writes a System.Enum by the numeric and textual value
        /// </summary>
        protected override void WriteEnum(string field, int number, string name)
        {
            WriteAsText(field, name, number);
        }
    }
}