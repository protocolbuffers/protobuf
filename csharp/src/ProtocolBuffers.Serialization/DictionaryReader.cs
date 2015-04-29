using System;
using System.Collections.Generic;
using System.Globalization;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Allows reading messages from a name/value dictionary
    /// </summary>
    public class DictionaryReader : AbstractReader
    {
        private readonly IEnumerator<KeyValuePair<string, object>> _input;
        private bool _ready;

        /// <summary>
        /// Creates a dictionary reader from an enumeration of KeyValuePair data, like an IDictionary
        /// </summary>
        public DictionaryReader(IEnumerable<KeyValuePair<string, object>> input)
        {
            _input = input.GetEnumerator();
            _ready = _input.MoveNext();
        }

        /// <summary>
        /// No-op
        /// </summary>
        public override void ReadMessageStart()
        { }

        /// <summary>
        /// No-op
        /// </summary>
        public override void ReadMessageEnd()
        { }

        /// <summary>
        /// Merges the contents of stream into the provided message builder
        /// </summary>
        public override TBuilder Merge<TBuilder>(TBuilder builder, ExtensionRegistry registry)
        {
            builder.WeakMergeFrom(this, registry);
            return builder;
        }

        /// <summary>
        /// Peeks at the next field in the input stream and returns what information is available.
        /// </summary>
        /// <remarks>
        /// This may be called multiple times without actually reading the field.  Only after the field
        /// is either read, or skipped, should PeekNext return a different value.
        /// </remarks>
        protected override bool PeekNext(out string field)
        {
            field = _ready ? _input.Current.Key : null;
            return _ready;
        }

        /// <summary>
        /// Causes the reader to skip past this field
        /// </summary>
        protected override void Skip()
        {
            _ready = _input.MoveNext();
        }

        private bool GetValue<T>(ref T value)
        {
            if (!_ready)
            {
                return false;
            }

            object obj = _input.Current.Value;
            if (obj is T)
            {
                value = (T) obj;
            }
            else
            {
                try
                {
                    if (obj is IConvertible)
                    {
                        value = (T)Convert.ChangeType(obj, typeof(T), FrameworkPortability.InvariantCulture);
                    }
                    else
                    {
                        value = (T) obj;
                    }
                }
                catch
                {
                    _ready = _input.MoveNext();
                    return false;
                }
            }
            _ready = _input.MoveNext();
            return true;
        }

        /// <summary>
        /// Returns true if it was able to read a Boolean from the input
        /// </summary>
        protected override bool Read(ref bool value)
        {
            return GetValue(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a Int32 from the input
        /// </summary>
        protected override bool Read(ref int value)
        {
            return GetValue(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a UInt32 from the input
        /// </summary>
        protected override bool Read(ref uint value)
        {
            return GetValue(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a Int64 from the input
        /// </summary>
        protected override bool Read(ref long value)
        {
            return GetValue(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a UInt64 from the input
        /// </summary>
        protected override bool Read(ref ulong value)
        {
            return GetValue(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a Single from the input
        /// </summary>
        protected override bool Read(ref float value)
        {
            return GetValue(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a Double from the input
        /// </summary>
        protected override bool Read(ref double value)
        {
            return GetValue(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a String from the input
        /// </summary>
        protected override bool Read(ref string value)
        {
            return GetValue(ref value);
        }

        /// <summary>
        /// Returns true if it was able to read a ByteString from the input
        /// </summary>
        protected override bool Read(ref ByteString value)
        {
            byte[] rawbytes = null;
            if (GetValue(ref rawbytes))
            {
                value = ByteString.CopyFrom(rawbytes);
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
            return GetValue(ref value);
        }

        /// <summary>
        /// Merges the input stream into the provided IBuilderLite 
        /// </summary>
        protected override bool ReadMessage(IBuilderLite builder, ExtensionRegistry registry)
        {
            IDictionary<string, object> values = null;
            if (GetValue(ref values))
            {
                new DictionaryReader(values).Merge(builder, registry);
                return true;
            }
            return false;
        }

        public override bool ReadArray<T>(FieldType type, string field, ICollection<T> items)
        {
            object[] array = null;
            if (GetValue(ref array))
            {
                if (typeof(T) == typeof(ByteString))
                {
                    ICollection<ByteString> output = (ICollection<ByteString>) items;
                    foreach (byte[] item in array)
                    {
                        output.Add(ByteString.CopyFrom(item));
                    }
                }
                else
                {
                    foreach (T item in array)
                    {
                        items.Add(item);
                    }
                }
                return true;
            }
            return false;
        }

        public override bool ReadEnumArray(string field, ICollection<object> items)
        {
            object[] array = null;
            if (GetValue(ref array))
            {
                foreach (object item in array)
                {
                    items.Add(item);
                }
                return true;
            }
            return false;
        }

        public override bool ReadMessageArray<T>(string field, ICollection<T> items, IMessageLite messageType,
                                                 ExtensionRegistry registry)
        {
            object[] array = null;
            if (GetValue(ref array))
            {
                foreach (IDictionary<string, object> item in array)
                {
                    IBuilderLite builder = messageType.WeakCreateBuilderForType();
                    new DictionaryReader(item).Merge(builder);
                    items.Add((T) builder.WeakBuild());
                }
                return true;
            }
            return false;
        }

        public override bool ReadGroupArray<T>(string field, ICollection<T> items, IMessageLite messageType,
                                               ExtensionRegistry registry)
        {
            return ReadMessageArray(field, items, messageType, registry);
        }
    }
}