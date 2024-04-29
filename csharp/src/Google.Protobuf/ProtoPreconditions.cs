#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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