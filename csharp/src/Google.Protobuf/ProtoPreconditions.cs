#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

namespace Google.Protobuf
{
    /// <summary>
    /// Helper methods for throwing exceptions when preconditions are not met.
    /// </summary>
    /// <remarks>
    /// This class is used internally and by generated code; it is not particularly
    /// expected to be used from application code, although nothing prevents it
    /// from being used that way.
    /// </remarks>
    public static class ProtoPreconditions
    {
        /// <summary>
        /// Throws an ArgumentNullException if the given value is null, otherwise
        /// return the value to the caller.
        /// </summary>
        public static T CheckNotNull<T>(T value, string name) where T : class
        {
            if (value == null)
            {
                throw new ArgumentNullException(name);
            }
            return value;
        }

        /// <summary>
        /// Throws an ArgumentNullException if the given value is null, otherwise
        /// return the value to the caller.
        /// </summary>
        /// <remarks>
        /// This is equivalent to <see cref="CheckNotNull{T}(T, string)"/> but without the type parameter
        /// constraint. In most cases, the constraint is useful to prevent you from calling CheckNotNull
        /// with a value type - but it gets in the way if either you want to use it with a nullable
        /// value type, or you want to use it with an unconstrained type parameter.
        /// </remarks>
        internal static T CheckNotNullUnconstrained<T>(T value, string name)
        {
            if (value == null)
            {
                throw new ArgumentNullException(name);
            }
            return value;
        }
    }
}