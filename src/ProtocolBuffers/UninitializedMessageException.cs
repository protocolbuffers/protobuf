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

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

#if !LITE
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

#endif

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// TODO(jonskeet): Write summary text.
    /// </summary>
    public sealed class UninitializedMessageException : Exception
    {
        private readonly IList<string> missingFields;

        private UninitializedMessageException(IList<string> missingFields)
            : base(BuildDescription(missingFields))
        {
            this.missingFields = new List<string>(missingFields);
        }

        /// <summary>
        /// Returns a read-only list of human-readable names of
        /// required fields missing from this message. Each name
        /// is a full path to a field, e.g. "foo.bar[5].baz"
        /// </summary>
        public IList<string> MissingFields
        {
            get { return missingFields; }
        }

        /// <summary>
        /// Converts this exception into an InvalidProtocolBufferException.
        /// When a parsed message is missing required fields, this should be thrown
        /// instead of UninitializedMessageException.
        /// </summary>
        public InvalidProtocolBufferException AsInvalidProtocolBufferException()
        {
            return new InvalidProtocolBufferException(Message);
        }

        /// <summary>
        /// Constructs the description string for a given list of missing fields.
        /// </summary>
        private static string BuildDescription(IEnumerable<string> missingFields)
        {
            StringBuilder description = new StringBuilder("Message missing required fields: ");
            bool first = true;
            foreach (string field in missingFields)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    description.Append(", ");
                }
                description.Append(field);
            }
            return description.ToString();
        }

        /// <summary>
        /// For Lite exceptions that do not known how to enumerate missing fields
        /// </summary>
        public UninitializedMessageException(IMessageLite message)
            : base(String.Format("Message {0} is missing required fields", message.GetType()))
        {
            missingFields = new List<string>();
        }

#if !LITE
        public UninitializedMessageException(IMessage message)
            : this(FindMissingFields(message))
        {
        }

        /// <summary>
        /// Returns a list of the full "paths" of missing required
        /// fields in the specified message.
        /// </summary>
        private static IList<String> FindMissingFields(IMessage message)
        {
            List<String> results = new List<String>();
            FindMissingFields(message, "", results);
            return results;
        }

        /// <summary>
        /// Recursive helper implementing FindMissingFields.
        /// </summary>
        private static void FindMissingFields(IMessage message, String prefix, List<String> results)
        {
            foreach (FieldDescriptor field in message.DescriptorForType.Fields)
            {
                if (field.IsRequired && !message.HasField(field))
                {
                    results.Add(prefix + field.Name);
                }
            }

            foreach (KeyValuePair<FieldDescriptor, object> entry in message.AllFields)
            {
                FieldDescriptor field = entry.Key;
                object value = entry.Value;

                if (field.MappedType == MappedType.Message)
                {
                    if (field.IsRepeated)
                    {
                        int i = 0;
                        foreach (object element in (IEnumerable) value)
                        {
                            if (element is IMessage)
                            {
                                FindMissingFields((IMessage) element, SubMessagePrefix(prefix, field, i++), results);
                            }
                            else
                            {
                                results.Add(prefix + field.Name);
                            }
                        }
                    }
                    else
                    {
                        if (message.HasField(field))
                        {
                            if (value is IMessage)
                            {
                                FindMissingFields((IMessage) value, SubMessagePrefix(prefix, field, -1), results);
                            }
                            else
                            {
                                results.Add(prefix + field.Name);
                            }
                        }
                    }
                }
            }
        }

        private static String SubMessagePrefix(String prefix, FieldDescriptor field, int index)
        {
            StringBuilder result = new StringBuilder(prefix);
            if (field.IsExtension)
            {
                result.Append('(')
                    .Append(field.FullName)
                    .Append(')');
            }
            else
            {
                result.Append(field.Name);
            }
            if (index != -1)
            {
                result.Append('[')
                    .Append(index)
                    .Append(']');
            }
            result.Append('.');
            return result.ToString();
        }
#endif
    }
}