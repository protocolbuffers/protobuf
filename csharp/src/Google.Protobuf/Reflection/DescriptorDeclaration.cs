#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2018 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
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
