using System;
using System.Globalization;
using System.Xml;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Provides a base class for text-parsing readers
    /// </summary>
    public abstract class AbstractTextReader : AbstractReader
    {
        /// <summary> Constructs a new reader </summary>
        protected AbstractTextReader() { }

        /// <summary>
        /// Reads a typed field as a string
        /// </summary>
        protected abstract bool ReadAsText(ref string textValue, Type type);

        /// <summary>
        /// Returns true if it was able to read a String from the input
        /// </summary>
        protected override bool Read(ref string value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(string)))
            {
                value = text;
                return true;
            }
            return false;
        }

        /// <summary>
        /// Returns true if it was able to read a Boolean from the input
        /// </summary>
        protected override bool Read(ref bool value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(bool)))
            {
                value = XmlConvert.ToBoolean(text);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Returns true if it was able to read a Int32 from the input
        /// </summary>
        protected override bool Read(ref int value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(int)))
            {
                value = XmlConvert.ToInt32(text);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Returns true if it was able to read a UInt32 from the input
        /// </summary>
        protected override bool Read(ref uint value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(uint)))
            {
                value = XmlConvert.ToUInt32(text);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Returns true if it was able to read a Int64 from the input
        /// </summary>
        protected override bool Read(ref long value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(long)))
            {
                value = XmlConvert.ToInt64(text);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Returns true if it was able to read a UInt64 from the input
        /// </summary>
        protected override bool Read(ref ulong value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(ulong)))
            {
                value = XmlConvert.ToUInt64(text);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Returns true if it was able to read a Single from the input
        /// </summary>
        protected override bool Read(ref float value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(float)))
            {
                value = XmlConvert.ToSingle(text);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Returns true if it was able to read a Double from the input
        /// </summary>
        protected override bool Read(ref double value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(double)))
            {
                value = XmlConvert.ToDouble(text);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Provides decoding of bytes read from the input stream
        /// </summary>
        protected virtual ByteString DecodeBytes(string bytes)
        {
            return ByteString.FromBase64(bytes);
        }

        /// <summary>
        /// Returns true if it was able to read a ByteString from the input
        /// </summary>
        protected override bool Read(ref ByteString value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(ByteString)))
            {
                value = DecodeBytes(text);
                return true;
            }
            return false;
        }

        /// <summary>
        /// returns true if it was able to read a single value into the value reference.  The value
        /// stored may be of type System.String, System.Int32, or an IEnumLite from the IEnumLiteMap.
        /// </summary>
        protected override bool ReadEnum(ref object value)
        {
            string text = null;
            if (ReadAsText(ref text, typeof(Enum)))
            {
                int number;
                if (FrameworkPortability.TryParseInt32(text, NumberStyles.Integer, FrameworkPortability.InvariantCulture, out number))
                {
                    value = number;
                    return true;
                }
                value = text;
                return true;
            }
            return false;
        }
    }
}