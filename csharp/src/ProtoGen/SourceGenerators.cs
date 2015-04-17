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

namespace Google.ProtocolBuffers.ProtoGen
{
    public delegate TResult Func<T, TResult>(T arg);

    internal static class SourceGenerators
    {
        private static readonly Dictionary<Type, Func<IDescriptor, ISourceGenerator>> GeneratorFactories =
            new Dictionary<Type, Func<IDescriptor, ISourceGenerator>>
                {
                    {typeof(FileDescriptor), descriptor => new UmbrellaClassGenerator((FileDescriptor) descriptor)},
                    {typeof(EnumDescriptor), descriptor => new EnumGenerator((EnumDescriptor) descriptor)},
                    {typeof(ServiceDescriptor), descriptor => new ServiceGenerator((ServiceDescriptor) descriptor)},
                    {typeof(MessageDescriptor), descriptor => new MessageGenerator((MessageDescriptor) descriptor)},
                    // For other fields, we have IFieldSourceGenerators.
                    {typeof(FieldDescriptor), descriptor => new ExtensionGenerator((FieldDescriptor) descriptor)}
                };

        public static IFieldSourceGenerator CreateFieldGenerator(FieldDescriptor field, int fieldOrdinal)
        {
            switch (field.MappedType)
            {
                case MappedType.Message:
                    return field.IsRepeated
                               ? (IFieldSourceGenerator) new RepeatedMessageFieldGenerator(field, fieldOrdinal)
                               : new MessageFieldGenerator(field, fieldOrdinal);
                case MappedType.Enum:
                    return field.IsRepeated
                               ? (IFieldSourceGenerator) new RepeatedEnumFieldGenerator(field, fieldOrdinal)
                               : new EnumFieldGenerator(field, fieldOrdinal);
                default:
                    return field.IsRepeated
                               ? (IFieldSourceGenerator) new RepeatedPrimitiveFieldGenerator(field, fieldOrdinal)
                               : new PrimitiveFieldGenerator(field, fieldOrdinal);
            }
        }

        public static ISourceGenerator CreateGenerator<T>(T descriptor) where T : IDescriptor
        {
            Func<IDescriptor, ISourceGenerator> factory;
            if (!GeneratorFactories.TryGetValue(typeof(T), out factory))
            {
                throw new ArgumentException("No generator registered for " + typeof(T).Name);
            }
            return factory(descriptor);
        }
    }
}