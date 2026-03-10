#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.Reflection;

namespace Google.Protobuf.Compatibility
{
    /// <summary>
    /// Extension methods for <see cref="PropertyInfo"/>, effectively providing
    /// the familiar members from previous desktop framework versions while
    /// targeting the newer releases, .NET Core etc.
    /// </summary>
    internal static class PropertyInfoExtensions
    {
        /// <summary>
        /// Returns the public getter of a property, or null if there is no such getter
        /// (either because it's read-only, or the getter isn't public).
        /// </summary>
        internal static MethodInfo GetGetMethod(this PropertyInfo target)
        {
            var method = target.GetMethod;
            return method != null && method.IsPublic ? method : null;
        }

        /// <summary>
        /// Returns the public setter of a property, or null if there is no such setter
        /// (either because it's write-only, or the setter isn't public).
        /// </summary>
        internal static MethodInfo GetSetMethod(this PropertyInfo target)
        {
            var method = target.SetMethod;
            return method != null && method.IsPublic ? method : null;
        }
    }
}
