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
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace Google.Protobuf
{
    /// <summary>
    /// Provides extensions to messages while parsing. This API is experimental and subject to change.
    /// </summary>
    public sealed class ExtensionRegistry : ICollection<Extension>, IDeepCloneable<ExtensionRegistry>
    {
        internal sealed class ExtensionComparer : IEqualityComparer<Extension>
        {
            public bool Equals(Extension a, Extension b)
            {
                return new ObjectIntPair<Type>(a.TargetType, a.FieldNumber).Equals(new ObjectIntPair<Type>(b.TargetType, b.FieldNumber));
            }
            public int GetHashCode(Extension a)
            {
                return new ObjectIntPair<Type>(a.TargetType, a.FieldNumber).GetHashCode();
            }

            internal static ExtensionComparer Instance = new ExtensionComparer();
        }
        private IDictionary<ObjectIntPair<Type>, Extension> extensions;

        /// <summary>
        /// Creates a new empty extension registry
        /// </summary>
        public ExtensionRegistry()
        {
            extensions = new Dictionary<ObjectIntPair<Type>, Extension>();
        }

        private ExtensionRegistry(IDictionary<ObjectIntPair<Type>, Extension> collection)
        {
            extensions = collection.ToDictionary(k => k.Key, v => v.Value);
        }

        /// <summary>
        /// Gets the total number of extensions in this extension registry
        /// </summary>
        public int Count => extensions.Count;

        /// <summary>
        /// Returns whether the registry is readonly
        /// </summary>
        bool ICollection<Extension>.IsReadOnly => false;

        internal bool ContainsInputField(CodedInputStream stream, Type target, out Extension extension)
        {
            return extensions.TryGetValue(new ObjectIntPair<Type>(target, WireFormat.GetTagFieldNumber(stream.LastTag)), out extension);
        }

        /// <summary>
        /// Adds the specified extension to the registry
        /// </summary>
        public void Add(Extension extension)
        {
            ProtoPreconditions.CheckNotNull(extension, nameof(extension));

            extensions.Add(new ObjectIntPair<Type>(extension.TargetType, extension.FieldNumber), extension);
        }

        /// <summary>
        /// Adds the specified extensions to the registry
        /// </summary>
        public void AddRange(IEnumerable<Extension> extensions)
        {
            ProtoPreconditions.CheckNotNull(extensions, nameof(extensions));

            foreach (var extension in extensions)
            {
                Add(extension);
            }
        }

        /// <summary>
        /// Clears the registry of all values
        /// </summary>
        public void Clear()
        {
            extensions.Clear();
        }

        /// <summary>
        /// Gets whether the extension registry contains the specified extension
        /// </summary>
        public bool Contains(Extension item)
        {
            ProtoPreconditions.CheckNotNull(item, nameof(item));

            return extensions.ContainsKey(new ObjectIntPair<Type>(item.TargetType, item.FieldNumber));
        }

        /// <summary>
        /// Copies the arrays in the registry set to the specified array at the specified index
        /// </summary>
        /// <param name="array">The array to copy to</param>
        /// <param name="arrayIndex">The array index to start at</param>
        void ICollection<Extension>.CopyTo(Extension[] array, int arrayIndex)
        {
            ProtoPreconditions.CheckNotNull(array, nameof(array));
            if (arrayIndex < 0 || arrayIndex >= array.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(arrayIndex));
            }
            if (array.Length - arrayIndex < Count)
            {
                throw new ArgumentException("The provided array is shorter than the number of elements in the registry");
            }

            for (int i = 0; i < array.Length; i++)
            {
                Extension extension = array[i];
                extensions.Add(new ObjectIntPair<Type>(extension.TargetType, extension.FieldNumber), extension);
            }
        }

        /// <summary>
        /// Returns an enumerator to enumerate through the items in the registry
        /// </summary>
        /// <returns>Returns an enumerator for the extensions in this registry</returns>
        public IEnumerator<Extension> GetEnumerator()
        {
            return extensions.Values.GetEnumerator();
        }

        /// <summary>
        /// Removes the specified extension from the set
        /// </summary>
        /// <param name="item">The extension</param>
        /// <returns><c>true</c> if the extension was removed, otherwise <c>false</c></returns>
        public bool Remove(Extension item)
        {
            ProtoPreconditions.CheckNotNull(item, nameof(item));

            return extensions.Remove(new ObjectIntPair<Type>(item.TargetType, item.FieldNumber));
        }

        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

        /// <summary>
        /// Clones the registry into a new registry
        /// </summary>
        public ExtensionRegistry Clone()
        {
            return new ExtensionRegistry(extensions);
        }
    }
}
