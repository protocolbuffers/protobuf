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
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// A table of known extensions, searchable by name or field number.  When
    /// parsing a protocol message that might have extensions, you must provide
    /// an <see cref="ExtensionRegistry"/> in which you have registered any extensions
    /// that you want to be able to parse.  Otherwise, those extensions will just
    /// be treated like unknown fields.
    /// </summary>
    /// <example>
    /// For example, if you had the <c>.proto</c> file:
    /// <code>
    /// option java_class = "MyProto";
    ///
    /// message Foo {
    ///   extensions 1000 to max;
    /// }
    ///
    /// extend Foo {
    ///   optional int32 bar;
    /// }
    /// </code>
    ///
    /// Then you might write code like:
    ///
    /// <code>
    /// ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
    /// registry.Add(MyProto.Bar);
    /// MyProto.Foo message = MyProto.Foo.ParseFrom(input, registry);
    /// </code>
    /// </example>
    ///
    /// <remarks>
    /// <para>You might wonder why this is necessary. Two alternatives might come to
    /// mind. First, you might imagine a system where generated extensions are
    /// automatically registered when their containing classes are loaded. This
    /// is a popular technique, but is bad design; among other things, it creates a
    /// situation where behavior can change depending on what classes happen to be
    /// loaded. It also introduces a security vulnerability, because an
    /// unprivileged class could cause its code to be called unexpectedly from a
    /// privileged class by registering itself as an extension of the right type.
    /// </para>
    /// <para>Another option you might consider is lazy parsing: do not parse an
    /// extension until it is first requested, at which point the caller must
    /// provide a type to use. This introduces a different set of problems. First,
    /// it would require a mutex lock any time an extension was accessed, which
    /// would be slow. Second, corrupt data would not be detected until first
    /// access, at which point it would be much harder to deal with it. Third, it
    /// could violate the expectation that message objects are immutable, since the
    /// type provided could be any arbitrary message class. An unprivileged user
    /// could take advantage of this to inject a mutable object into a message
    /// belonging to privileged code and create mischief.</para>
    /// </remarks>
    public sealed partial class ExtensionRegistry
    {
        /// <summary>
        /// Finds an extension by fully-qualified field name, in the
        /// proto namespace, i.e. result.Descriptor.FullName will match
        /// <paramref name="fullName"/> if a match is found. A null
        /// reference is returned if the extension can't be found.
        /// </summary>
        [Obsolete("Please use the FindByName method instead.", true)]
        public ExtensionInfo this[string fullName]
        {
            get
            {
                foreach (IGeneratedExtensionLite ext in extensionsByNumber.Values)
                {
                    if (StringComparer.Ordinal.Equals(ext.Descriptor.FullName, fullName))
                    {
                        return ext as ExtensionInfo;
                    }
                }
                return null;
            }
        }

#if !LITE
        /// <summary>
        /// Finds an extension by containing type and field number.
        /// A null reference is returned if the extension can't be found.
        /// </summary>
        public ExtensionInfo this[MessageDescriptor containingType, int fieldNumber]
        {
            get
            {
                IGeneratedExtensionLite ret;
                extensionsByNumber.TryGetValue(new ExtensionIntPair(containingType, fieldNumber), out ret);
                return ret as ExtensionInfo;
            }
        }

        public ExtensionInfo FindByName(MessageDescriptor containingType, string fieldName)
        {
            return FindExtensionByName(containingType, fieldName) as ExtensionInfo;
        }
#endif

        /// <summary>
        /// Add an extension from a generated file to the registry.
        /// </summary>
        public void Add<TExtension>(GeneratedExtensionBase<TExtension> extension)
        {
            if (extension.Descriptor.MappedType == MappedType.Message)
            {
                Add(new ExtensionInfo(extension.Descriptor, extension.MessageDefaultInstance));
            }
            else
            {
                Add(new ExtensionInfo(extension.Descriptor, null));
            }
        }

        /// <summary>
        /// Adds a non-message-type extension to the registry by descriptor.
        /// </summary>
        /// <param name="type"></param>
        public void Add(FieldDescriptor type)
        {
            if (type.MappedType == MappedType.Message)
            {
                throw new ArgumentException("ExtensionRegistry.Add() must be provided a default instance "
                                            + "when adding an embedded message extension.");
            }
            Add(new ExtensionInfo(type, null));
        }

        /// <summary>
        /// Adds a message-type-extension to the registry by descriptor.
        /// </summary>
        /// <param name="type"></param>
        /// <param name="defaultInstance"></param>
        public void Add(FieldDescriptor type, IMessage defaultInstance)
        {
            if (type.MappedType != MappedType.Message)
            {
                throw new ArgumentException("ExtensionRegistry.Add() provided a default instance for a "
                                            + "non-message extension.");
            }
            Add(new ExtensionInfo(type, defaultInstance));
        }

        private void Add(ExtensionInfo extension)
        {
            if (readOnly)
            {
                throw new InvalidOperationException("Cannot add entries to a read-only extension registry");
            }
            if (!extension.Descriptor.IsExtension)
            {
                throw new ArgumentException("ExtensionRegistry.add() was given a FieldDescriptor for a "
                                            + "regular (non-extension) field.");
            }

            IGeneratedExtensionLite liteExtension = extension;
            Add(liteExtension);

            FieldDescriptor field = extension.Descriptor;
            if (field.ContainingType.Options.MessageSetWireFormat
                && field.FieldType == FieldType.Message
                && field.IsOptional
                && field.ExtensionScope == field.MessageType)
            {
                // This is an extension of a MessageSet type defined within the extension
                // type's own scope. For backwards-compatibility, allow it to be looked
                // up by type name.
                Dictionary<string, IGeneratedExtensionLite> map;
                if (extensionsByName.TryGetValue(liteExtension.ContainingType, out map))
                {
                    map[field.MessageType.FullName] = extension;
                }
            }
        }
    }
}