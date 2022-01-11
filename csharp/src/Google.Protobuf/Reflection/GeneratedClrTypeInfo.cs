#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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
using System.Diagnostics.CodeAnalysis;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Extra information provided by generated code when initializing a message or file descriptor.
    /// These are constructed as required, and are not long-lived. Hand-written code should
    /// never need to use this type.
    /// </summary>
    public sealed class GeneratedClrTypeInfo
    {
        private static readonly string[] EmptyNames = new string[0];
        private static readonly GeneratedClrTypeInfo[] EmptyCodeInfo = new GeneratedClrTypeInfo[0];
        private static readonly Extension[] EmptyExtensions = new Extension[0];
        internal const DynamicallyAccessedMemberTypes MessageAccessibility =
            // Creating types
            DynamicallyAccessedMemberTypes.PublicConstructors |
            // Getting and setting properties
            DynamicallyAccessedMemberTypes.PublicProperties |
            DynamicallyAccessedMemberTypes.NonPublicProperties |
            // Calling presence methods
            DynamicallyAccessedMemberTypes.PublicMethods;

        /// <summary>
        /// Irrelevant for file descriptors; the CLR type for the message for message descriptors.
        /// </summary>
        [DynamicallyAccessedMembers(MessageAccessibility)]
        public Type ClrType { get; private set; }

        /// <summary>
        /// Irrelevant for file descriptors; the parser for message descriptors.
        /// </summary>
        public MessageParser Parser { get; }

        /// <summary>
        /// Irrelevant for file descriptors; the CLR property names (in message descriptor field order)
        /// for fields in the message for message descriptors.
        /// </summary>
        public string[] PropertyNames { get; }

        /// <summary>
        /// The extensions defined within this file/message descriptor
        /// </summary>
        public Extension[] Extensions { get; }

        /// <summary>
        /// Irrelevant for file descriptors; the CLR property "base" names (in message descriptor oneof order)
        /// for oneofs in the message for message descriptors. It is expected that for a oneof name of "Foo",
        /// there will be a "FooCase" property and a "ClearFoo" method.
        /// </summary>
        public string[] OneofNames { get; }

        /// <summary>
        /// The reflection information for types within this file/message descriptor. Elements may be null
        /// if there is no corresponding generated type, e.g. for map entry types.
        /// </summary>
        public GeneratedClrTypeInfo[] NestedTypes { get; }

        /// <summary>
        /// The CLR types for enums within this file/message descriptor.
        /// </summary>
        public Type[] NestedEnums { get; }

        /// <summary>
        /// Creates a GeneratedClrTypeInfo for a message descriptor, with nested types, nested enums, the CLR type, property names and oneof names.
        /// Each array parameter may be null, to indicate a lack of values.
        /// The parameter order is designed to make it feasible to format the generated code readably.
        /// </summary>
        public GeneratedClrTypeInfo(
            // Preserve all public members on message types when trimming is enabled.
            // This ensures that members used by reflection, e.g. JSON serialization, are preserved.
            [DynamicallyAccessedMembers(MessageAccessibility)]
            Type clrType, MessageParser parser, string[] propertyNames, string[] oneofNames, Type[] nestedEnums, Extension[] extensions, GeneratedClrTypeInfo[] nestedTypes)
        {
            NestedTypes = nestedTypes ?? EmptyCodeInfo;
            NestedEnums = nestedEnums ?? ReflectionUtil.EmptyTypes;
            ClrType = clrType;
            Extensions = extensions ?? EmptyExtensions;
            Parser = parser;
            PropertyNames = propertyNames ?? EmptyNames;
            OneofNames = oneofNames ?? EmptyNames;
        }

        /// <summary>
        /// Creates a GeneratedClrTypeInfo for a message descriptor, with nested types, nested enums, the CLR type, property names and oneof names.
        /// Each array parameter may be null, to indicate a lack of values.
        /// The parameter order is designed to make it feasible to format the generated code readably.
        /// </summary>
        public GeneratedClrTypeInfo(
            // Preserve all public members on message types when trimming is enabled.
            // This ensures that members used by reflection, e.g. JSON serialization, are preserved.
            [DynamicallyAccessedMembers(MessageAccessibility)]
            Type clrType, MessageParser parser, string[] propertyNames, string[] oneofNames, Type[] nestedEnums, GeneratedClrTypeInfo[] nestedTypes)
            : this(clrType, parser, propertyNames, oneofNames, nestedEnums, null, nestedTypes)
        {
        }

        /// <summary>
        /// Creates a GeneratedClrTypeInfo for a file descriptor, with only types, enums, and extensions.
        /// </summary>
        public GeneratedClrTypeInfo(Type[] nestedEnums, Extension[] extensions, GeneratedClrTypeInfo[] nestedTypes)
            : this(null, null, null, null, nestedEnums, extensions, nestedTypes)
        {
        }

        /// <summary>
        /// Creates a GeneratedClrTypeInfo for a file descriptor, with only types and enums.
        /// </summary>
        public GeneratedClrTypeInfo(Type[] nestedEnums, GeneratedClrTypeInfo[] nestedTypes)
            : this(null, null, null, null, nestedEnums, nestedTypes)
        {
        }
    }
}