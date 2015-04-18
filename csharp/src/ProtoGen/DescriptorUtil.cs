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
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen
{
    /// <summary>
    /// Utility class for determining namespaces etc.
    /// </summary>
    internal static class DescriptorUtil
    {
        internal static string GetFullUmbrellaClassName(IDescriptor descriptor)
        {
            CSharpFileOptions options = descriptor.File.CSharpOptions;
            string result = options.Namespace;
            if (result != "")
            {
                result += '.';
            }
            result += GetQualifiedUmbrellaClassName(options);
            return "global::" + result;
        }

        /// <summary>
        /// Evaluates the options and returns the qualified name of the umbrella class
        /// relative to the descriptor type's namespace.  Basically concatenates the
        /// UmbrellaNamespace + UmbrellaClassname fields.
        /// </summary>
        internal static string GetQualifiedUmbrellaClassName(CSharpFileOptions options)
        {
            string fullName = options.UmbrellaClassname;
            if (!options.NestClasses && options.UmbrellaNamespace != "")
            {
                fullName = String.Format("{0}.{1}", options.UmbrellaNamespace, options.UmbrellaClassname);
            }
            return fullName;
        }

        internal static string GetMappedTypeName(MappedType type)
        {
            switch (type)
            {
                case MappedType.Int32:
                    return "int";
                case MappedType.Int64:
                    return "long";
                case MappedType.UInt32:
                    return "uint";
                case MappedType.UInt64:
                    return "ulong";
                case MappedType.Single:
                    return "float";
                case MappedType.Double:
                    return "double";
                case MappedType.Boolean:
                    return "bool";
                case MappedType.String:
                    return "string";
                case MappedType.ByteString:
                    return "pb::ByteString";
                case MappedType.Enum:
                    return null;
                case MappedType.Message:
                    return null;
                default:
                    throw new ArgumentOutOfRangeException("Unknown mapped type " + type);
            }
        }
    }
}