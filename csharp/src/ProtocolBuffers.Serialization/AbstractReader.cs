using System;
using System.Collections.Generic;
using System.Globalization;
using Google.ProtocolBuffers.Descriptors;

//Disable CS3011: only CLS-compliant members can be abstract
#pragma warning disable 3011

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Provides a base-class that provides some basic functionality for handling type dispatching
    /// </summary>
    public abstract class AbstractReader : ICodedInputStream
    {
        private const int DefaultMaxDepth = 64;
        private int _depth;
        
        /// <summary> Constructs a new reader </summary>
        protected AbstractReader() { MaxDepth = DefaultMaxDepth; }

        /// <summary> Gets or sets the maximum recursion depth allowed </summary>
        public int MaxDepth { get; set; }

        /// <summary>
        /// Merges the contents of stream into the provided message builder
        /// </summary>
        public TBuilder Merge<TBuilder>(TBuilder builder) where TBuilder : IBuilderLite
        {
            return Merge(builder, ExtensionRegistry.Empty);
        }

        /// <summary>
        /// Merges the contents of stream into the provided message builder
        /// </summary>
        public abstract TBuilder Merge<TBuilder>(TBuilder builder, ExtensionRegistry registry)
            where TBuilder : IBuilderLite;

        /// <summary>
        /// Peeks at the next field in the input stream and returns what information is available.
        /// </summary>
        /// <remarks>
        /// This may be called multiple times without actually reading the field.  Only after the field
        /// is either read, or skipped, should PeekNext return a different value.
        /// </remarks>
        protected abstract bool PeekNext(out string field);

        /// <summary>
        /// Causes the reader to skip past this field
        /// </summary>
        protected abstract void Skip();

        /// <summary>
        /// Returns true if it was able to read a Boolean from the input
        /// </summary>
        protected abstract bool Read(ref bool value);

        /// <summary>
        /// Returns true if it was able to read a Int32 from the input
        /// </summary>
        protected abstract bool Read(ref int value);

        /// <summary>
        /// Returns true if it was able to read a UInt32 from the input
        /// </summary>
        protected abstract bool Read(ref uint value);

        /// <summary>
        /// Returns true if it was able to read a Int64 from the input
        /// </summary>
        protected abstract bool Read(ref long value);

        /// <summary>
        /// Returns true if it was able to read a UInt64 from the input
        /// </summary>
        protected abstract bool Read(ref ulong value);

        /// <summary>
        /// Returns true if it was able to read a Single from the input
        /// </summary>
        protected abstract bool Read(ref float value);

        /// <summary>
        /// Returns true if it was able to read a Double from the input
        /// </summary>
        protected abstract bool Read(ref double value);

        /// <summary>
        /// Returns true if it was able to read a String from the input
        /// </summary>
        protected abstract bool Read(ref string value);

        /// <summary>
        /// Returns true if it was able to read a ByteString from the input
        /// </summary>
        protected abstract bool Read(ref ByteString value);

        /// <summary>
        /// returns true if it was able to read a single value into the value reference.  The value
        /// stored may be of type System.String, System.Int32, or an IEnumLite from the IEnumLiteMap.
        /// </summary>
        protected abstract bool ReadEnum(ref object value);

        /// <summary>
        /// Merges the input stream into the provided IBuilderLite 
        /// </summary>
        protected abstract bool ReadMessage(IBuilderLite builder, ExtensionRegistry registry);
        
        /// <summary>
        /// Reads the root-message preamble specific to this formatter
        /// </summary>
        public abstract void ReadMessageStart();

        /// <summary>
        /// Reads the root-message close specific to this formatter
        /// </summary>
        public abstract void ReadMessageEnd();

        /// <summary>
        /// Merges the input stream into the provided IBuilderLite 
        /// </summary>
        public virtual bool ReadGroup(IBuilderLite value, ExtensionRegistry registry)
        {
            return ReadMessage(value, registry);
        }

        /// <summary>
        /// Cursors through the array elements and stops at the end of the array
        /// </summary>
        protected virtual IEnumerable<string> ForeachArrayItem(string field)
        {
            string next = field;
            while (true)
            {
                yield return next;

                if (!PeekNext(out next) || next != field)
                {
                    break;
                }
            }
        }

        /// <summary>
        /// Reads an array of T messages
        /// </summary>
        public virtual bool ReadMessageArray<T>(string field, ICollection<T> items, IMessageLite messageType,
                                                ExtensionRegistry registry)
        {
            bool success = false;
            foreach (string next in ForeachArrayItem(field))
            {
                IBuilderLite builder = messageType.WeakCreateBuilderForType();
                if (ReadMessage(builder, registry))
                {
                    items.Add((T) builder.WeakBuild());
                    success |= true;
                }
            }
            return success;
        }

        /// <summary>
        /// Reads an array of T messages as a proto-buffer group
        /// </summary>
        public virtual bool ReadGroupArray<T>(string field, ICollection<T> items, IMessageLite messageType,
                                              ExtensionRegistry registry)
        {
            bool success = false;
            foreach (string next in ForeachArrayItem(field))
            {
                IBuilderLite builder = messageType.WeakCreateBuilderForType();
                if (ReadGroup(builder, registry))
                {
                    items.Add((T) builder.WeakBuild());
                    success |= true;
                }
            }
            return success;
        }

        /// <summary>
        /// Reads an array of System.Enum type T and adds them to the collection
        /// </summary>
        public virtual bool ReadEnumArray(string field, ICollection<object> items)
        {
            bool success = false;
            foreach (string next in ForeachArrayItem(field))
            {
                object temp = null;
                if (ReadEnum(ref temp))
                {
                    items.Add(temp);
                    success |= true;
                }
            }
            return success;
        }

        /// <summary>
        /// Reads an array of T, where T is a primitive type defined by FieldType
        /// </summary>
        public virtual bool ReadArray<T>(FieldType type, string field, ICollection<T> items)
        {
            bool success = false;
            foreach (string next in ForeachArrayItem(field))
            {
                object temp = null;
                if (ReadField(type, ref temp))
                {
                    items.Add((T) temp);
                    success |= true;
                }
            }
            return success;
        }

        /// <summary>
        /// returns true if it was able to read a single primitive value of FieldType into the value reference
        /// </summary>
        public virtual bool ReadField(FieldType type, ref object value)
        {
            switch (type)
            {
                case FieldType.Bool:
                    {
                        bool temp = false;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                case FieldType.Int64:
                case FieldType.SInt64:
                case FieldType.SFixed64:
                    {
                        long temp = 0;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                case FieldType.UInt64:
                case FieldType.Fixed64:
                    {
                        ulong temp = 0;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                case FieldType.Int32:
                case FieldType.SInt32:
                case FieldType.SFixed32:
                    {
                        int temp = 0;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                case FieldType.UInt32:
                case FieldType.Fixed32:
                    {
                        uint temp = 0;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                case FieldType.Float:
                    {
                        float temp = float.NaN;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                case FieldType.Double:
                    {
                        double temp = float.NaN;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                case FieldType.String:
                    {
                        string temp = null;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                case FieldType.Bytes:
                    {
                        ByteString temp = null;
                        if (Read(ref temp))
                        {
                            value = temp;
                        }
                        else
                        {
                            return false;
                        }
                        break;
                    }
                default:
                    throw InvalidProtocolBufferException.InvalidTag();
            }
            return true;
        }

        #region ICodedInputStream Members

        bool ICodedInputStream.ReadTag(out uint fieldTag, out string fieldName)
        {
            fieldTag = 0;
            if (PeekNext(out fieldName))
            {
                return true;
            }
            return false;
        }

        bool ICodedInputStream.ReadDouble(ref double value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadFloat(ref float value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadUInt64(ref ulong value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadInt64(ref long value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadInt32(ref int value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadFixed64(ref ulong value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadFixed32(ref uint value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadBool(ref bool value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadString(ref string value)
        {
            return Read(ref value);
        }

        void ICodedInputStream.ReadGroup(int fieldNumber, IBuilderLite builder, ExtensionRegistry extensionRegistry)
        {
            if (_depth++ > MaxDepth)
            {
                throw new RecursionLimitExceededException();
            }
            ReadGroup(builder, extensionRegistry);
            _depth--;
        }

        void ICodedInputStream.ReadUnknownGroup(int fieldNumber, IBuilderLite builder)
        {
            throw new NotSupportedException();
        }

        void ICodedInputStream.ReadMessage(IBuilderLite builder, ExtensionRegistry extensionRegistry)
        {
            if (_depth++ > MaxDepth)
            {
                throw new RecursionLimitExceededException();
            }
            ReadMessage(builder, extensionRegistry);
            _depth--;
        }

        bool ICodedInputStream.ReadBytes(ref ByteString value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadUInt32(ref uint value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadEnum(ref IEnumLite value, out object unknown, IEnumLiteMap mapping)
        {
            value = null;
            unknown = null;
            if (ReadEnum(ref unknown))
            {
                if (unknown is int)
                {
                    value = mapping.FindValueByNumber((int) unknown);
                }
                else if (unknown is string)
                {
                    value = mapping.FindValueByName((string) unknown);
                }
                return value != null;
            }
            return false;
        }

        bool ICodedInputStream.ReadEnum<T>(ref T value, out object rawValue)
        {
            rawValue = null;
            if (ReadEnum(ref rawValue))
            {
                if (!EnumParser<T>.TryConvert(rawValue, ref value))
                {
                    value = default(T);
                    return false;
                }
                return true;
            }
            return false;
        }

        bool ICodedInputStream.ReadSFixed32(ref int value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadSFixed64(ref long value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadSInt32(ref int value)
        {
            return Read(ref value);
        }

        bool ICodedInputStream.ReadSInt64(ref long value)
        {
            return Read(ref value);
        }

        void ICodedInputStream.ReadPrimitiveArray(FieldType fieldType, uint fieldTag, string fieldName,
                                                  ICollection<object> list)
        {
            ReadArray(fieldType, fieldName, list);
        }

        void ICodedInputStream.ReadEnumArray(uint fieldTag, string fieldName, ICollection<IEnumLite> list,
                                             out ICollection<object> unknown, IEnumLiteMap mapping)
        {
            unknown = null;
            List<object> array = new List<object>();
            if (ReadEnumArray(fieldName, array))
            {
                foreach (object rawValue in array)
                {
                    IEnumLite item = null;
                    if (rawValue is int)
                    {
                        item = mapping.FindValueByNumber((int) rawValue);
                    }
                    else if (rawValue is string)
                    {
                        item = mapping.FindValueByName((string) rawValue);
                    }

                    if (item != null)
                    {
                        list.Add(item);
                    }
                    else
                    {
                        if (unknown == null)
                        {
                            unknown = new List<object>();
                        }
                        unknown.Add(rawValue);
                    }
                }
            }
        }

        void ICodedInputStream.ReadEnumArray<T>(uint fieldTag, string fieldName, ICollection<T> list,
                                                out ICollection<object> unknown)
        {
            unknown = null;
            List<object> array = new List<object>();
            if (ReadEnumArray(fieldName, array))
            {
                foreach (object rawValue in array)
                {
                    T val = default(T);
                    if (EnumParser<T>.TryConvert(rawValue, ref val))
                    {
                        list.Add(val);
                    }
                    else
                    {
                        if (unknown == null)
                        {
                            unknown = new List<object>();
                        }
                        unknown.Add(rawValue);
                    }
                }
            }
        }

        void ICodedInputStream.ReadMessageArray<T>(uint fieldTag, string fieldName, ICollection<T> list, T messageType,
                                                   ExtensionRegistry registry)
        {
            if (_depth++ > MaxDepth)
            {
                throw new RecursionLimitExceededException();
            }
            ReadMessageArray(fieldName, list, messageType, registry);
            _depth--;
        }

        void ICodedInputStream.ReadGroupArray<T>(uint fieldTag, string fieldName, ICollection<T> list, T messageType,
                                                 ExtensionRegistry registry)
        {
            if (_depth++ > MaxDepth)
            {
                throw new RecursionLimitExceededException();
            }
            ReadGroupArray(fieldName, list, messageType, registry);
            _depth--;
        }

        bool ICodedInputStream.ReadPrimitiveField(FieldType fieldType, ref object value)
        {
            return ReadField(fieldType, ref value);
        }

        bool ICodedInputStream.IsAtEnd
        {
            get
            {
                string next;
                return PeekNext(out next) == false;
            }
        }

        bool ICodedInputStream.SkipField()
        {
            Skip();
            return true;
        }

        void ICodedInputStream.ReadStringArray(uint fieldTag, string fieldName, ICollection<string> list)
        {
            ReadArray(FieldType.String, fieldName, list);
        }

        void ICodedInputStream.ReadBytesArray(uint fieldTag, string fieldName, ICollection<ByteString> list)
        {
            ReadArray(FieldType.Bytes, fieldName, list);
        }

        void ICodedInputStream.ReadBoolArray(uint fieldTag, string fieldName, ICollection<bool> list)
        {
            ReadArray(FieldType.Bool, fieldName, list);
        }

        void ICodedInputStream.ReadInt32Array(uint fieldTag, string fieldName, ICollection<int> list)
        {
            ReadArray(FieldType.Int32, fieldName, list);
        }

        void ICodedInputStream.ReadSInt32Array(uint fieldTag, string fieldName, ICollection<int> list)
        {
            ReadArray(FieldType.SInt32, fieldName, list);
        }

        void ICodedInputStream.ReadUInt32Array(uint fieldTag, string fieldName, ICollection<uint> list)
        {
            ReadArray(FieldType.UInt32, fieldName, list);
        }

        void ICodedInputStream.ReadFixed32Array(uint fieldTag, string fieldName, ICollection<uint> list)
        {
            ReadArray(FieldType.Fixed32, fieldName, list);
        }

        void ICodedInputStream.ReadSFixed32Array(uint fieldTag, string fieldName, ICollection<int> list)
        {
            ReadArray(FieldType.SFixed32, fieldName, list);
        }

        void ICodedInputStream.ReadInt64Array(uint fieldTag, string fieldName, ICollection<long> list)
        {
            ReadArray(FieldType.Int64, fieldName, list);
        }

        void ICodedInputStream.ReadSInt64Array(uint fieldTag, string fieldName, ICollection<long> list)
        {
            ReadArray(FieldType.SInt64, fieldName, list);
        }

        void ICodedInputStream.ReadUInt64Array(uint fieldTag, string fieldName, ICollection<ulong> list)
        {
            ReadArray(FieldType.UInt64, fieldName, list);
        }

        void ICodedInputStream.ReadFixed64Array(uint fieldTag, string fieldName, ICollection<ulong> list)
        {
            ReadArray(FieldType.Fixed64, fieldName, list);
        }

        void ICodedInputStream.ReadSFixed64Array(uint fieldTag, string fieldName, ICollection<long> list)
        {
            ReadArray(FieldType.SFixed64, fieldName, list);
        }

        void ICodedInputStream.ReadDoubleArray(uint fieldTag, string fieldName, ICollection<double> list)
        {
            ReadArray(FieldType.Double, fieldName, list);
        }

        void ICodedInputStream.ReadFloatArray(uint fieldTag, string fieldName, ICollection<float> list)
        {
            ReadArray(FieldType.Float, fieldName, list);
        }

        #endregion
    }
}