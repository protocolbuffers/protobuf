#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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
        private readonly IDictionary<ObjectIntPair<Type>, Extension> extensions;

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

        internal bool ContainsInputField(uint lastTag, Type target, out Extension extension)
        {
            return extensions.TryGetValue(new ObjectIntPair<Type>(target, WireFormat.GetTagFieldNumber(lastTag)), out extension);
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
