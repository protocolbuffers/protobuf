#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2018 Google Inc.  All rights reserved.
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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using static Google.Protobuf.Reflection.SourceCodeInfo.Types;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Provides additional information about the declaration of a descriptor,
    /// such as source location and comments.
    /// </summary>
    public sealed class DescriptorDeclaration
    {
        /// <summary>
        /// The descriptor this declaration relates to.
        /// </summary>
        public IDescriptor Descriptor { get; }

        /// <summary>
        /// The start line of the declaration within the source file. This value is 1-based.
        /// </summary>
        public int StartLine { get; }
        /// <summary>
        /// The start column of the declaration within the source file. This value is 1-based.
        /// </summary>
        public int StartColumn { get; }

        /// <summary>
        /// // The end line of the declaration within the source file. This value is 1-based.
        /// </summary>
        public int EndLine { get; }
        /// <summary>
        /// The end column of the declaration within the source file. This value is 1-based, and
        /// exclusive. (The final character of the declaration is on the column before this value.)
        /// </summary>
        public int EndColumn { get; }

        /// <summary>
        /// Comments appearing before the declaration. Never null, but may be empty. Multi-line comments
        /// are represented as a newline-separated string. Leading whitespace and the comment marker ("//")
        /// are removed from each line.
        /// </summary>
        public string LeadingComments { get; }

        /// <summary>
        /// Comments appearing after the declaration. Never null, but may be empty. Multi-line comments
        /// are represented as a newline-separated string. Leading whitespace and the comment marker ("//")
        /// are removed from each line.
        /// </summary>
        public string TrailingComments { get; }

        /// <summary>
        /// Comments appearing before the declaration, but separated from it by blank
        /// lines. Each string represents a newline-separated paragraph of comments.
        /// Leading whitespace and the comment marker ("//") are removed from each line.
        /// The list is never null, but may be empty. Likewise each element is never null, but may be empty.
        /// </summary>
        public IReadOnlyList<string> LeadingDetachedComments { get; }

        private DescriptorDeclaration(IDescriptor descriptor, Location location)
        {
            // TODO: Validation
            Descriptor = descriptor;
            bool hasEndLine = location.Span.Count == 4;
            // Lines and columns are 0-based in the proto.
            StartLine = location.Span[0] + 1;
            StartColumn = location.Span[1] + 1;
            EndLine = hasEndLine ? location.Span[2] + 1 : StartLine;
            EndColumn = location.Span[hasEndLine ? 3 : 2] + 1;
            LeadingComments = location.LeadingComments;
            TrailingComments = location.TrailingComments;
            LeadingDetachedComments = new ReadOnlyCollection<string>(location.LeadingDetachedComments.ToList());
        }

        internal static DescriptorDeclaration FromProto(IDescriptor descriptor, Location location) =>
            new DescriptorDeclaration(descriptor, location);
    }
}
