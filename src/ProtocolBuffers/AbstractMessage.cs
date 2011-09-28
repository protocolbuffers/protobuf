#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#endregion

using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Implementation of the non-generic IMessage interface as far as possible.
    /// </summary>
    public abstract partial class AbstractMessage<TMessage, TBuilder> : AbstractMessageLite<TMessage, TBuilder>,
                                                                IMessage<TMessage, TBuilder>
        where TMessage : AbstractMessage<TMessage, TBuilder>
        where TBuilder : AbstractBuilder<TMessage, TBuilder>
    {
        /// <summary>
        /// The serialized size if it's already been computed, or null
        /// if we haven't computed it yet.
        /// </summary>
        private int? memoizedSize = null;

        #region Unimplemented members of IMessage

        public abstract MessageDescriptor DescriptorForType { get; }
        public abstract IDictionary<FieldDescriptor, object> AllFields { get; }
        public abstract bool HasField(FieldDescriptor field);
        public abstract object this[FieldDescriptor field] { get; }
        public abstract int GetRepeatedFieldCount(FieldDescriptor field);
        public abstract object this[FieldDescriptor field, int index] { get; }
        public abstract UnknownFieldSet UnknownFields { get; }

        #endregion

        /// <summary>
        /// Returns true iff all required fields in the message and all embedded
        /// messages are set.
        /// </summary>
        public override bool IsInitialized
        {
            get
            {
                // Check that all required fields are present.
                foreach (FieldDescriptor field in DescriptorForType.Fields)
                {
                    if (field.IsRequired && !HasField(field))
                    {
                        return false;
                    }
                }

                // Check that embedded messages are initialized.
                foreach (KeyValuePair<FieldDescriptor, object> entry in AllFields)
                {
                    FieldDescriptor field = entry.Key;
                    if (field.MappedType == MappedType.Message)
                    {
                        if (field.IsRepeated)
                        {
                            // We know it's an IList<T>, but not the exact type - so
                            // IEnumerable is the best we can do. (C# generics aren't covariant yet.)
                            foreach (IMessageLite element in (IEnumerable) entry.Value)
                            {
                                if (!element.IsInitialized)
                                {
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            if (!((IMessageLite) entry.Value).IsInitialized)
                            {
                                return false;
                            }
                        }
                    }
                }
                return true;
            }
        }

        public override sealed string ToString()
        {
            return TextFormat.PrintToString(this);
        }

        public override sealed void PrintTo(TextWriter writer)
        {
            TextFormat.Print(this, writer);
        }

        /// <summary>
        /// Serializes the message and writes it to the given output stream.
        /// This does not flush or close the stream.
        /// </summary>
        /// <remarks>
        /// Protocol Buffers are not self-delimiting. Therefore, if you write
        /// any more data to the stream after the message, you must somehow ensure
        /// that the parser on the receiving end does not interpret this as being
        /// part of the protocol message. One way of doing this is by writing the size
        /// of the message before the data, then making sure you limit the input to
        /// that size when receiving the data. Alternatively, use WriteDelimitedTo(Stream).
        /// </remarks>
        public override void WriteTo(ICodedOutputStream output)
        {
            foreach (KeyValuePair<FieldDescriptor, object> entry in AllFields)
            {
                FieldDescriptor field = entry.Key;
                if (field.IsRepeated)
                {
                    // We know it's an IList<T>, but not the exact type - so
                    // IEnumerable is the best we can do. (C# generics aren't covariant yet.)
                    IEnumerable valueList = (IEnumerable) entry.Value;
                    if (field.IsPacked)
                    {
                        output.WritePackedArray(field.FieldType, field.FieldNumber, field.Name, valueList);
                    }
                    else
                    {
                        output.WriteArray(field.FieldType, field.FieldNumber, field.Name, valueList);
                    }
                }
                else
                {
                    output.WriteField(field.FieldType, field.FieldNumber, field.Name, entry.Value);
                }
            }

            UnknownFieldSet unknownFields = UnknownFields;
            if (DescriptorForType.Options.MessageSetWireFormat)
            {
                unknownFields.WriteAsMessageSetTo(output);
            }
            else
            {
                unknownFields.WriteTo(output);
            }
        }

        /// <summary>
        /// Returns the number of bytes required to encode this message.
        /// The result is only computed on the first call and memoized after that.
        /// </summary>
        public override int SerializedSize
        {
            get
            {
                if (memoizedSize != null)
                {
                    return memoizedSize.Value;
                }

                int size = 0;
                foreach (KeyValuePair<FieldDescriptor, object> entry in AllFields)
                {
                    FieldDescriptor field = entry.Key;
                    if (field.IsRepeated)
                    {
                        IEnumerable valueList = (IEnumerable) entry.Value;
                        if (field.IsPacked)
                        {
                            int dataSize = 0;
                            foreach (object element in valueList)
                            {
                                dataSize += CodedOutputStream.ComputeFieldSizeNoTag(field.FieldType, element);
                            }
                            size += dataSize;
                            size += CodedOutputStream.ComputeTagSize(field.FieldNumber);
                            size += CodedOutputStream.ComputeRawVarint32Size((uint) dataSize);
                        }
                        else
                        {
                            foreach (object element in valueList)
                            {
                                size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber, element);
                            }
                        }
                    }
                    else
                    {
                        size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber, entry.Value);
                    }
                }

                UnknownFieldSet unknownFields = UnknownFields;
                if (DescriptorForType.Options.MessageSetWireFormat)
                {
                    size += unknownFields.SerializedSizeAsMessageSet;
                }
                else
                {
                    size += unknownFields.SerializedSize;
                }

                memoizedSize = size;
                return size;
            }
        }

        /// <summary>
        /// Compares the specified object with this message for equality.
        /// Returns true iff the given object is a message of the same type
        /// (as defined by DescriptorForType) and has identical values
        /// for all its fields.
        /// </summary>
        public override bool Equals(object other)
        {
            if (other == this)
            {
                return true;
            }
            IMessage otherMessage = other as IMessage;
            if (otherMessage == null || otherMessage.DescriptorForType != DescriptorForType)
            {
                return false;
            }
            return Dictionaries.Equals(AllFields, otherMessage.AllFields) &&
                   UnknownFields.Equals(otherMessage.UnknownFields);
        }

        /// <summary>
        /// Returns the hash code value for this message.
        /// TODO(jonskeet): Specify the hash algorithm, but better than the Java one!
        /// </summary>
        public override int GetHashCode()
        {
            int hash = 41;
            hash = (19*hash) + DescriptorForType.GetHashCode();
            hash = (53*hash) + Dictionaries.GetHashCode(AllFields);
            hash = (29*hash) + UnknownFields.GetHashCode();
            return hash;
        }

        #region Explicit Members

        IBuilder IMessage.WeakCreateBuilderForType()
        {
            return CreateBuilderForType();
        }

        IBuilder IMessage.WeakToBuilder()
        {
            return ToBuilder();
        }

        IMessage IMessage.WeakDefaultInstanceForType
        {
            get { return DefaultInstanceForType; }
        }

        #endregion
    }
}