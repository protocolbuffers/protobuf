using System;
using System.Collections;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Allows writing messages to a name/value dictionary
    /// </summary>
    public class DictionaryWriter : AbstractWriter
    {
        private readonly IDictionary<string, object> _output;

        /// <summary>
        /// Constructs a writer using a new dictionary
        /// </summary>
        public DictionaryWriter()
            : this(new Dictionary<string, object>(StringComparer.Ordinal))
        {
        }

        /// <summary>
        /// Constructs a writer using an existing dictionary
        /// </summary>
        public DictionaryWriter(IDictionary<string, object> output)
        {
            ThrowHelper.ThrowIfNull(output, "output");
            _output = output;
        }

        /// <summary>
        /// Creates the dictionary instance for a child message.
        /// </summary>
        protected virtual DictionaryWriter Create()
        {
            return new DictionaryWriter();
        }

        /// <summary>
        /// Accesses the dictionary that is backing this writer
        /// </summary>
        public IDictionary<string, object> ToDictionary()
        {
            return _output;
        }

        /// <summary>
        /// Writes the message to the the formatted stream.
        /// </summary>
        public override void WriteMessage(IMessageLite message)
        {
            message.WriteTo(this);
        }


        /// <summary>
        /// No-op
        /// </summary>
        public override void WriteMessageStart()
        { }

        /// <summary>
        /// No-op
        /// </summary>
        public override void WriteMessageEnd()
        { }

        /// <summary>
        /// Writes a Boolean value
        /// </summary>
        protected override void Write(string field, bool value)
        {
            _output[field] = value;
        }

        /// <summary>
        /// Writes a Int32 value
        /// </summary>
        protected override void Write(string field, int value)
        {
            _output[field] = value;
        }

        /// <summary>
        /// Writes a UInt32 value
        /// </summary>
        protected override void Write(string field, uint value)
        {
            _output[field] = value;
        }

        /// <summary>
        /// Writes a Int64 value
        /// </summary>
        protected override void Write(string field, long value)
        {
            _output[field] = value;
        }

        /// <summary>
        /// Writes a UInt64 value
        /// </summary>
        protected override void Write(string field, ulong value)
        {
            _output[field] = value;
        }

        /// <summary>
        /// Writes a Single value
        /// </summary>
        protected override void Write(string field, float value)
        {
            _output[field] = value;
        }

        /// <summary>
        /// Writes a Double value
        /// </summary>
        protected override void Write(string field, double value)
        {
            _output[field] = value;
        }

        /// <summary>
        /// Writes a String value
        /// </summary>
        protected override void Write(string field, string value)
        {
            _output[field] = value;
        }

        /// <summary>
        /// Writes a set of bytes
        /// </summary>
        protected override void Write(string field, ByteString value)
        {
            _output[field] = value.ToByteArray();
        }

        /// <summary>
        /// Writes a message or group as a field
        /// </summary>
        protected override void WriteMessageOrGroup(string field, IMessageLite message)
        {
            DictionaryWriter writer = Create();
            writer.WriteMessage(message);

            _output[field] = writer.ToDictionary();
        }

        /// <summary>
        /// Writes a System.Enum by the numeric and textual value
        /// </summary>
        protected override void WriteEnum(string field, int number, string name)
        {
            _output[field] = number;
        }

        /// <summary>
        /// Writes an array of field values
        /// </summary>
        protected override void WriteArray(FieldType fieldType, string field, IEnumerable items)
        {
            List<object> objects = new List<object>();
            foreach (object o in items)
            {
                switch (fieldType)
                {
                    case FieldType.Group:
                    case FieldType.Message:
                        {
                            DictionaryWriter writer = Create();
                            writer.WriteMessage((IMessageLite) o);
                            objects.Add(writer.ToDictionary());
                        }
                        break;
                    case FieldType.Bytes:
                        objects.Add(((ByteString) o).ToByteArray());
                        break;
                    case FieldType.Enum:
                        if (o is IEnumLite)
                        {
                            objects.Add(((IEnumLite) o).Number);
                        }
                        else
                        {
                            objects.Add((int) o);
                        }
                        break;
                    default:
                        objects.Add(o);
                        break;
                }
            }

            _output[field] = objects.ToArray();
        }
    }
}