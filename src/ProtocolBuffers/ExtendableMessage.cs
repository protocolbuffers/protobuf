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
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    public abstract partial class ExtendableMessage<TMessage, TBuilder> : GeneratedMessage<TMessage, TBuilder>
        where TMessage : GeneratedMessage<TMessage, TBuilder>
        where TBuilder : GeneratedBuilder<TMessage, TBuilder>, new()
    {
        protected ExtendableMessage()
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

        /// <summary>
        /// Checks if a singular extension is present.
        /// </summary>
        public bool HasExtension<TExtension>(GeneratedExtensionBase<TExtension> extension)
        {
            return extensions.HasField(extension.Descriptor);
        }

        /// <summary>
        /// Returns the number of elements in a repeated extension.
        /// </summary>
        public int GetExtensionCount<TExtension>(GeneratedExtensionBase<IList<TExtension>> extension)
        {
            return extensions.GetRepeatedFieldCount(extension.Descriptor);
        }

        /// <summary>
        /// Returns the value of an extension.
        /// </summary>
        public TExtension GetExtension<TExtension>(GeneratedExtensionBase<TExtension> extension)
        {
            object value = extensions[extension.Descriptor];
            if (value == null)
            {
                return (TExtension) extension.MessageDefaultInstance;
            }
            else
            {
                return (TExtension) extension.FromReflectionType(value);
            }
        }

        /// <summary>
        /// Returns one element of a repeated extension.
        /// </summary>
        public TExtension GetExtension<TExtension>(GeneratedExtensionBase<IList<TExtension>> extension, int index)
        {
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
            get { return base.IsInitialized && ExtensionsAreInitialized; }
        }

        #region Reflection

        public override IDictionary<FieldDescriptor, object> AllFields
        {
            get
            {
                IDictionary<FieldDescriptor, object> result = GetMutableFieldMap();
                foreach (KeyValuePair<IFieldDescriptorLite, object> entry in extensions.AllFields)
                {
                    result[(FieldDescriptor) entry.Key] = entry.Value;
                }
                return Dictionaries.AsReadOnly(result);
            }
        }

        public override bool HasField(FieldDescriptor field)
        {
            if (field.IsExtension)
            {
                VerifyContainingType(field);
                return extensions.HasField(field);
            }
            else
            {
                return base.HasField(field);
            }
        }

        public override object this[FieldDescriptor field]
        {
            get
            {
                if (field.IsExtension)
                {
                    VerifyContainingType(field);
                    object value = extensions[field];
                    if (value == null)
                    {
                        // Lacking an ExtensionRegistry, we have no way to determine the
                        // extension's real type, so we return a DynamicMessage.
                        // TODO(jonskeet): Work out what this means
                        return DynamicMessage.GetDefaultInstance(field.MessageType);
                    }
                    else
                    {
                        return value;
                    }
                }
                else
                {
                    return base[field];
                }
            }
        }

        public override int GetRepeatedFieldCount(FieldDescriptor field)
        {
            if (field.IsExtension)
            {
                VerifyContainingType(field);
                return extensions.GetRepeatedFieldCount(field);
            }
            else
            {
                return base.GetRepeatedFieldCount(field);
            }
        }

        public override object this[FieldDescriptor field, int index]
        {
            get
            {
                if (field.IsExtension)
                {
                    VerifyContainingType(field);
                    return extensions[field, index];
                }
                else
                {
                    return base[field, index];
                }
            }
        }

        internal void VerifyContainingType(FieldDescriptor field)
        {
            if (field.ContainingType != DescriptorForType)
            {
                throw new ArgumentException("FieldDescriptor does not match message type.");
            }
        }

        #endregion

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

            internal ExtensionWriter(ExtendableMessage<TMessage, TBuilder> message)
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

        protected ExtensionWriter CreateExtensionWriter(ExtendableMessage<TMessage, TBuilder> message)
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

        internal void VerifyExtensionContainingType<TExtension>(GeneratedExtensionBase<TExtension> extension)
        {
            if (extension.Descriptor.ContainingType != DescriptorForType)
            {
                // This can only happen if someone uses unchecked operations.
                throw new ArgumentException("Extension is for type \"" + extension.Descriptor.ContainingType.FullName
                                            + "\" which does not match message type \"" + DescriptorForType.FullName +
                                            "\".");
            }
        }
    }
}