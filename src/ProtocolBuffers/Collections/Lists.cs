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
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Google.ProtocolBuffers.Collections
{
    /// <summary>
    /// Utility non-generic class for calling into Lists{T} using type inference.
    /// </summary>
    public static class Lists
    {
        /// <summary>
        /// Returns a read-only view of the specified list.
        /// </summary>
        public static IList<T> AsReadOnly<T>(IList<T> list)
        {
            return Lists<T>.AsReadOnly(list);
        }

        public static bool Equals<T>(IList<T> left, IList<T> right)
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

        public static int GetHashCode<T>(IList<T> list)
        {
            int hash = 31;
            foreach (T element in list)
            {
                hash = hash*29 + element.GetHashCode();
            }
            return hash;
        }
    }

    /// <summary>
    /// Utility class for dealing with lists.
    /// </summary>
    public static class Lists<T>
    {
        private static readonly ReadOnlyCollection<T> empty = new ReadOnlyCollection<T>(new T[0]);

        /// <summary>
        /// Returns an immutable empty list.
        /// </summary>
        public static ReadOnlyCollection<T> Empty
        {
            get { return empty; }
        }

        /// <summary>
        /// Returns either the original reference if it's already read-only,
        /// or a new ReadOnlyCollection wrapping the original list.
        /// </summary>
        public static IList<T> AsReadOnly(IList<T> list)
        {
            return list.IsReadOnly ? list : new ReadOnlyCollection<T>(list);
        }
    }
}