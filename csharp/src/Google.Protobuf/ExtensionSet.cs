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

using Google.Protobuf.Collections;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Google.Protobuf
{
    /// <summary>
    /// Methods for managing <see cref="ExtensionSet{TTarget}"/>s with null checking.
    /// 
    /// Most users will not use this class directly and its API is experimental and subject to change.
    /// </summary>
    public static class ExtensionSet
    {
        private static bool TryGetValue<TTarget>(ref ExtensionSet<TTarget> set, Extension extension, out IExtensionValue value) where TTarget : IExtendableMessage<TTarget>
        {
            if (set == null)
            {
                value = null;
                return false;
            }
            return set.ValuesByNumber.TryGetValue(extension.FieldNumber, out value);
        }

        /// <summary>
        /// Gets the value of the specified extension
        /// </summary>
        public static TValue Get<TTarget, TValue>(ref ExtensionSet<TTarget> set, Extension<TTarget, TValue> extension) where TTarget : IExtendableMessage<TTarget>
        {
            IExtensionValue value;
            if (TryGetValue(ref set, extension, out value))
            {
                return ((ExtensionValue<TValue>)value).GetValue();
            }
            else 
            {
                return extension.DefaultValue;
            }
        }

        /// <summary>
        /// Gets the value of the specified repeated extension or null if it doesn't exist in this set
        /// </summary>
        public static RepeatedField<TValue> Get<TTarget, TValue>(ref ExtensionSet<TTarget> set, RepeatedExtension<TTarget, TValue> extension) where TTarget : IExtendableMessage<TTarget>
        {
            IExtensionValue value;
            if (TryGetValue(ref set, extension, out value))
            {
                return ((RepeatedExtensionValue<TValue>)value).GetValue();
            }
            else 
            {
                return null;
            }
        }

        /// <summary>
        /// Gets the value of the specified repeated extension, registering it if it doesn't exist
        /// </summary>
        public static RepeatedField<TValue> GetOrInitialize<TTarget, TValue>(ref ExtensionSet<TTarget> set, RepeatedExtension<TTarget, TValue> extension) where TTarget : IExtendableMessage<TTarget>
        {
            IExtensionValue value;
            if (set == null)
            {
                value = extension.CreateValue();
                set = new ExtensionSet<TTarget>();
                set.ValuesByNumber.Add(extension.FieldNumber, value);
            }
            else
            {
                if (!set.ValuesByNumber.TryGetValue(extension.FieldNumber, out value))
                {
                    value = extension.CreateValue();
                    set.ValuesByNumber.Add(extension.FieldNumber, value);
                }
            }

            return ((RepeatedExtensionValue<TValue>)value).GetValue();
        }

        /// <summary>
        /// Sets the value of the specified extension. This will make a new instance of ExtensionSet if the set is null.
        /// </summary>
        public static void Set<TTarget, TValue>(ref ExtensionSet<TTarget> set, Extension<TTarget, TValue> extension, TValue value) where TTarget : IExtendableMessage<TTarget>
        {
            ProtoPreconditions.CheckNotNullUnconstrained(value, nameof(value));

            IExtensionValue extensionValue;
            if (set == null)
            {
                extensionValue = extension.CreateValue();
                set = new ExtensionSet<TTarget>();
                set.ValuesByNumber.Add(extension.FieldNumber, extensionValue);
            }
            else
            {
                if (!set.ValuesByNumber.TryGetValue(extension.FieldNumber, out extensionValue))
                {
                    extensionValue = extension.CreateValue();
                    set.ValuesByNumber.Add(extension.FieldNumber, extensionValue);
                }
            }
            
            ((ExtensionValue<TValue>)extensionValue).SetValue(value);
        }

        /// <summary>
        /// Gets whether the value of the specified extension is set
        /// </summary>
        public static bool Has<TTarget, TValue>(ref ExtensionSet<TTarget> set, Extension<TTarget, TValue> extension) where TTarget : IExtendableMessage<TTarget>
        {
            IExtensionValue value;
            return TryGetValue(ref set, extension, out value);
        }

        /// <summary>
        /// Clears the value of the specified extension
        /// </summary>
        public static void Clear<TTarget, TValue>(ref ExtensionSet<TTarget> set, Extension<TTarget, TValue> extension) where TTarget : IExtendableMessage<TTarget>
        {
            if (set == null)
            {
                return;
            }
            set.ValuesByNumber.Remove(extension.FieldNumber);
            if (set.ValuesByNumber.Count == 0)
            {
                set = null;
            }
        }

        /// <summary>
        /// Clears the value of the specified extension
        /// </summary>
        public static void Clear<TTarget, TValue>(ref ExtensionSet<TTarget> set, RepeatedExtension<TTarget, TValue> extension) where TTarget : IExtendableMessage<TTarget>
        {
            if (set == null)
            {
                return;
            }
            set.ValuesByNumber.Remove(extension.FieldNumber);
            if (set.ValuesByNumber.Count == 0)
            {
                set = null;
            }
        }

        /// <summary>
        /// Tries to merge a field from the coded input, returning true if the field was merged.
        /// If the set is null or the field was not otherwise merged, this returns false.
        /// </summary>
        public static bool TryMergeFieldFrom<TTarget>(ref ExtensionSet<TTarget> set, CodedInputStream stream) where TTarget : IExtendableMessage<TTarget>
        {
            Extension extension;
            int lastFieldNumber = WireFormat.GetTagFieldNumber(stream.LastTag);
            
            IExtensionValue extensionValue;
            if (set != null && set.ValuesByNumber.TryGetValue(lastFieldNumber, out extensionValue))
            {
                extensionValue.MergeFrom(stream);
                return true;
            }
            else if (stream.ExtensionRegistry != null && stream.ExtensionRegistry.ContainsInputField(stream, typeof(TTarget), out extension))
            {
                IExtensionValue value = extension.CreateValue();
                value.MergeFrom(stream);
                set = (set ?? new ExtensionSet<TTarget>());
                set.ValuesByNumber.Add(extension.FieldNumber, value);
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Merges the second set into the first set, creating a new instance if first is null
        /// </summary>
        public static void MergeFrom<TTarget>(ref ExtensionSet<TTarget> first, ExtensionSet<TTarget> second) where TTarget : IExtendableMessage<TTarget>
        {
            if (second == null)
            {
                return;
            }
            if (first == null)
            {
                first = new ExtensionSet<TTarget>();
            }
            foreach (var pair in second.ValuesByNumber)
            {
                IExtensionValue value;
                if (first.ValuesByNumber.TryGetValue(pair.Key, out value))
                {
                    value.MergeFrom(pair.Value);
                }
                else
                {
                    var cloned = pair.Value.Clone();
                    first.ValuesByNumber[pair.Key] = cloned;
                }
            }
        }

        /// <summary>
        /// Clones the set into a new set. If the set is null, this returns null
        /// </summary>
        public static ExtensionSet<TTarget> Clone<TTarget>(ExtensionSet<TTarget> set) where TTarget : IExtendableMessage<TTarget>
        {
            if (set == null)
            {
                return null;
            }

            var newSet = new ExtensionSet<TTarget>();
            foreach (var pair in set.ValuesByNumber)
            {
                var cloned = pair.Value.Clone();
                newSet.ValuesByNumber[pair.Key] = cloned;
            }
            return newSet;
        }
    }

    /// <summary>
    /// Used for keeping track of extensions in messages. 
    /// <see cref="IExtendableMessage{T}"/> methods route to this set.
    /// 
    /// Most users will not need to use this class directly
    /// </summary>
    /// <typeparam name="TTarget">The message type that extensions in this set target</typeparam>
    public sealed class ExtensionSet<TTarget> where TTarget : IExtendableMessage<TTarget>
    {
        internal Dictionary<int, IExtensionValue> ValuesByNumber { get; } = new Dictionary<int, IExtensionValue>();

        /// <summary>
        /// Gets a hash code of the set
        /// </summary>
        public override int GetHashCode()
        {
            int ret = typeof(TTarget).GetHashCode();
            foreach (KeyValuePair<int, IExtensionValue> field in ValuesByNumber)
            {
                // Use ^ here to make the field order irrelevant.
                int hash = field.Key.GetHashCode() ^ field.Value.GetHashCode();
                ret ^= hash;
            }
            return ret;
        }

        /// <summary>
        /// Returns whether this set is equal to the other object
        /// </summary>
        public override bool Equals(object other)
        {
            if (ReferenceEquals(this, other))
            {
                return true;
            }
            ExtensionSet<TTarget> otherSet = other as ExtensionSet<TTarget>;
            if (ValuesByNumber.Count != otherSet.ValuesByNumber.Count)
            {
                return false;
            }
            foreach (var pair in ValuesByNumber)
            {
                IExtensionValue secondValue;
                if (!otherSet.ValuesByNumber.TryGetValue(pair.Key, out secondValue))
                {
                    return false;
                }
                if (!pair.Value.Equals(secondValue))
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Calculates the size of this extension set
        /// </summary>
        public int CalculateSize()
        {
            int size = 0;
            foreach (var value in ValuesByNumber.Values)
            {
                size += value.CalculateSize();
            }
            return size;
        }

        /// <summary>
        /// Writes the extension values in this set to the output stream
        /// </summary>
        public void WriteTo(CodedOutputStream stream)
        {
            foreach (var value in ValuesByNumber.Values)
            {
                value.WriteTo(stream);
            }
        }

        internal bool IsInitialized()
        {
            return ValuesByNumber.Values.All(v => v.IsInitialized());
        }
    }
}
