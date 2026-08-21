#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Thrown when building descriptors fails because the source DescriptorProtos
    /// are not valid.
    /// </summary>
    public sealed class DescriptorValidationException : Exception
    {
        /// <value>
        /// The full name of the descriptor where the error occurred.
        /// </value>
        public string ProblemSymbolName { get; }

        /// <value>
        /// A human-readable description of the error. (The Message property
        /// is made up of the descriptor's name and this description.)
        /// </value>
        public string Description { get; }

        internal DescriptorValidationException(IDescriptor problemDescriptor, string description) :
            base(problemDescriptor.FullName + ": " + description)
        {
            // Note that problemDescriptor may be partially uninitialized, so we
            // don't want to expose it directly to the user.  So, we only provide
            // the name and the original proto.
            ProblemSymbolName = problemDescriptor.FullName;
            Description = description;
        }

        internal DescriptorValidationException(IDescriptor problemDescriptor, string description, Exception cause) :
            base(problemDescriptor.FullName + ": " + description, cause)
        {
            ProblemSymbolName = problemDescriptor.FullName;
            Description = description;
        }
    }
}