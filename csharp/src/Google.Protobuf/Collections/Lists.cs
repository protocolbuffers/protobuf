#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.Collections.Generic;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// Utility to compare if two Lists are the same, and the hash code
    /// of a List.
    /// </summary>
    public static class Lists
    {
        /// <summary>
        /// Checks if two lists are equal.
        /// </summary>
        public static bool Equals<T>(List<T> left, List<T> right)
        {
            if (left == right)
            {
                return true;
            }
            if (left == null || right == null)
            {
                return false;
            }
            if (left.Count != right.Count)
            {
                return false;
            }
            IEqualityComparer<T> comparer = EqualityComparer<T>.Default;
            for (int i = 0; i < left.Count; i++)
            {
                if (!comparer.Equals(left[i], right[i]))
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Gets the list's hash code.
        /// </summary>
        public static int GetHashCode<T>(List<T> list)
        {
            if (list == null)
            {
                return 0;
            }
            int hash = 31;
            foreach (T element in list)
            {
                hash = hash * 29 + element.GetHashCode();
            }
            return hash;
        }
    }
}