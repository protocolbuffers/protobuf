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
using System.IO;
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers
{
    public abstract partial class ExtendableMessageLite<TMessage, TBuilder> : GeneratedMessageLite<TMessage, TBuilder>
        where TMessage : GeneratedMessageLite<TMessage, TBuilder>
        where TBuilder : GeneratedBuilderLite<TMessage, TBuilder>
    {
        protected ExtendableMessageLite()
        {
        }

        private readonly FieldSet extensions = FieldSet.CreateInstance();

        /// <summary>
        /// Access for the builder.
        /// </summary>
        internal FieldSet Extensions
        {
            get { return extensions; }
        }

        public override bool Equals(object obj)
        {
            ExtendableMessageLite<TMessage, TBuilder> other = obj as ExtendableMessageLite<TMessage, TBuilder>;
            return !ReferenceEquals(null, other) &&
                   Dictionaries.Equals(extensions.AllFields, other.extensions.AllFields);
        }

        public override int GetHashCode()
        {
            return Dictionaries.GetHashCode(extensions.AllFields);
        }

        /// <summary>
        /// writes the extensions to the text stream
        /// </summary>
        public override void PrintTo(TextWriter writer)
        {
            foreach (KeyValuePair<IFieldDescriptorLite, object> entry in extensions.AllFields)
            {
                string fn = string.Format("[{0}]", entry.Key.FullName);
                if (entry.Key.IsRepeated)
                {
                    foreach (object o in ((IEnumerable) entry.Value))
                    {
                        PrintField(fn, true, o, writer);
                    }
                }
                else
                {
                    PrintField(fn, true, entry.Value, writer);
                }
            }
        }

        /// <summary>
        /// Checks if a singular extension is present.
        /// </summary>
        public bool HasExtension<TExtension>(GeneratedExtensionLite<TMessage, TExtension> extension)
        {
            VerifyExtensionContainingType(extension);
            return extensions.HasField(extension.Descriptor);
        }

        /// <summary>
        /// Returns the number of elements in a repeated extension.
        /// </summary>
        public int GetExtensionCount<TExtension>(GeneratedExtensionLite<TMessage, IList<TExtension>> extension)
        {
            VerifyExtensionContainingType(extension);
            return extensions.GetRepeatedFieldCount(extension.Descriptor);
        }

        /// <summary>
        /// Returns the value of an extension.
        /// </summary>
        public TExtension GetExtension<TExtension>(GeneratedExtensionLite<TMessage, TExtension> extension)
        {
            VerifyExtensionContainingType(extension);
            object value = extensions[extension.Descriptor];
            if (value == null)
            {
                return extension.DefaultValue;
            }
            else
            {
                return (TExtension) extension.FromReflectionType(value);
            }
        }

        /// <summary>
        /// Returns one element of a repeated extension.
        /// </summary>
        public TExtension GetExtension<TExtension>(GeneratedExtensionLite<TMessage, IList<TExtension>> extension,
                                                   int index)
        {
            VerifyExtensionContainingType(extension);
            return (TExtension) extension.SingularFromReflectionType(extensions[extension.Descriptor, index]);
        }

        /// <summary>
        /// Called to check if all extensions are initialized.
        /// </summary>
        protected bool ExtensionsAreInitialized
        {
            get { return extensions.IsInitialized; }
        }

        public override bool IsInitialized
        {
            get { return ExtensionsAreInitialized; }
        }

        /// <summary>
        /// Used by subclasses to serialize extensions. Extension ranges may be
        /// interleaves with field numbers, but we must write them in canonical
        /// (sorted by field number) order. This class helps us to write individual
        /// ranges of extensions at once.
        /// 
        /// TODO(jonskeet): See if we can improve this in terms of readability.
        /// </summary>
        protected class ExtensionWriter
        {
            private readonly IEnumerator<KeyValuePair<IFieldDescriptorLite, object>> iterator;
            private readonly FieldSet extensions;
            private KeyValuePair<IFieldDescriptorLite, object>? next = null;

            internal ExtensionWriter(ExtendableMessageLite<TMessage, TBuilder> message)
            {
                extensions = message.extensions;
                iterator = message.extensions.GetEnumerator();
                if (iterator.MoveNext())
                {
                    next = iterator.Current;
                }
            }

            public void WriteUntil(int end, ICodedOutputStream output)
            {
                while (next != null && next.Value.Key.FieldNumber < end)
                {
                    extensions.WriteField(next.Value.Key, next.Value.Value, output);
                    if (iterator.MoveNext())
                    {
                        next = iterator.Current;
                    }
                    else
                    {
                        next = null;
                    }
                }
            }
        }

        protected ExtensionWriter CreateExtensionWriter(ExtendableMessageLite<TMessage, TBuilder> message)
        {
            return new ExtensionWriter(message);
        }

        /// <summary>
        /// Called by subclasses to compute the size of extensions.
        /// </summary>
        protected int ExtensionsSerializedSize
        {
            get { return extensions.SerializedSize; }
        }

        internal void VerifyExtensionContainingType<TExtension>(GeneratedExtensionLite<TMessage, TExtension> extension)
        {
            if (!ReferenceEquals(extension.ContainingTypeDefaultInstance, DefaultInstanceForType))
            {
                // This can only happen if someone uses unchecked operations.
                throw new ArgumentException(
                    String.Format("Extension is for type \"{0}\" which does not match message type \"{1}\".",
                                  extension.ContainingTypeDefaultInstance, DefaultInstanceForType
                        ));
            }
        }
    }
}