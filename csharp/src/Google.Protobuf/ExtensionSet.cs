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
    /// Most users will not use this class directly
    /// </summary>
    public static class ExtensionSet
    {
        private static IExtensionValue GetValue<TTarget>(ref ExtensionSet<TTarget> set, Extension extension) where TTarget : IExtensionMessage<TTarget>
        {
            if (!set.ValuesByIdentifier.ContainsKey(extension))
            {
                Register(ref set, extension);
            }
            return set.ValuesByIdentifier[extension];
        }

        /// <summary>
        /// Registers the specified extension in this set
        /// </summary>
        public static void Register<TTarget>(ref ExtensionSet<TTarget> set, Extension extension) where TTarget : IExtensionMessage<TTarget>
        {
            if (set == null)
            {
                set = new ExtensionSet<TTarget>();
            }
            if (set.ValuesByIdentifier.ContainsKey(extension))
            {
                return;
            }
            if (extension.TargetType != typeof(TTarget))
            {
                throw new ArgumentException("Cannot register extension for wrong target type");
            }
            if (set.ValuesByNumber.ContainsKey(extension.FieldNumber))
            {
                throw new ArgumentException("Set already contains an extension with the specified field number");
            }
            var value = extension.CreateValue();
            set.ValuesByIdentifier.Add(extension, value);
            set.ValuesByNumber.Add(extension.FieldNumber, value);
        }

        /// <summary>
        /// Gets the value of the specified extension
        /// </summary>
        public static TValue Get<TTarget, TValue>(ref ExtensionSet<TTarget> set, Extension<TTarget, TValue> extension) where TTarget : IExtensionMessage<TTarget>
        {
            if (set == null)
            {
                set = new ExtensionSet<TTarget>();
            }
            return ((ExtensionValue<TValue>)GetValue(ref set, extension)).GetValue();
        }

        /// <summary>
        /// Gets the value of the specified repeated extension
        /// </summary>
        public static RepeatedField<TValue> Get<TTarget, TValue>(ref ExtensionSet<TTarget> set, RepeatedExtension<TTarget, TValue> extension) where TTarget : IExtensionMessage<TTarget>
        {
            if (set == null)
            {
                set = new ExtensionSet<TTarget>();
            }
            return ((RepeatedExtensionValue<TValue>)GetValue(ref set, extension)).GetValue();
        }

        /// <summary>
        /// Sets the value of the specified extension
        /// </summary>
        public static void Set<TTarget, TValue>(ref ExtensionSet<TTarget> set, Extension<TTarget, TValue> extension, TValue value) where TTarget : IExtensionMessage<TTarget>
        {
            if (set == null)
            {
                set = new ExtensionSet<TTarget>();
            }
            ((ExtensionValue<TValue>)GetValue(ref set, extension)).SetValue(value);
        }

        /// <summary>
        /// Gets whether the value of the specified extension is set
        /// </summary>
        public static bool Has<TTarget, TValue>(ref ExtensionSet<TTarget> set, Extension<TTarget, TValue> extension) where TTarget : IExtensionMessage<TTarget>
        {
            if (set == null)
            {
                set = new ExtensionSet<TTarget>();
            }
            return ((ExtensionValue<TValue>)GetValue(ref set, extension)).HasValue;
        }

        /// <summary>
        /// Clears the value of the specified extension
        /// </summary>
        public static void Clear<TTarget, TValue>(ref ExtensionSet<TTarget> set, Extension<TTarget, TValue> extension) where TTarget : IExtensionMessage<TTarget>
        {
            if (set == null)
            {
                set = new ExtensionSet<TTarget>();
            }
            ((ExtensionValue<TValue>)GetValue(ref set, extension)).ClearValue();
        }

        /// <summary>
        /// Tries to merge a field from the coded input, returning true if the field was merged.
        /// If the set is null or the field was not otherwise merged, this returns false.
        /// </summary>
        public static bool TryMergeFieldFrom<TTarget>(ref ExtensionSet<TTarget> set, CodedInputStream stream) where TTarget : IExtensionMessage<TTarget>
        {
            if (set == null)
            {
                return false;
            }
            IExtensionValue extensionValue;
            if (set.ValuesByNumber.TryGetValue(WireFormat.GetTagFieldNumber(stream.LastTag), out extensionValue))
            {
                extensionValue.MergeFrom(stream);
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
        public static void MergeFrom<TTarget>(ref ExtensionSet<TTarget> first, ExtensionSet<TTarget> second) where TTarget : IExtensionMessage<TTarget>
        {
            if (second == null)
            {
                return;
            }
            if (first == null)
            {
                first = new ExtensionSet<TTarget>();
            }
            foreach (var pair in second.ValuesByIdentifier)
            {
                IExtensionValue value;
                if (first.ValuesByIdentifier.TryGetValue(pair.Key, out value))
                {
                    value.MergeFrom(pair.Value);
                }
                else
                {
                    var cloned = pair.Value.Clone();
                    first.ValuesByIdentifier[pair.Key] = cloned;
                    first.ValuesByNumber[pair.Key.FieldNumber] = cloned;
                }
            }
        }

        /// <summary>
        /// Clones the set into a new set. If the set is null, this returns null
        /// </summary>
        public static ExtensionSet<TTarget> Clone<TTarget>(ExtensionSet<TTarget> set) where TTarget : IExtensionMessage<TTarget>
        {
            if (set == null)
            {
                return null;
            }

            var newSet = new ExtensionSet<TTarget>();
            foreach (var pair in set.ValuesByIdentifier)
            {
                var cloned = pair.Value.Clone();
                newSet.ValuesByIdentifier[pair.Key] = cloned;
                newSet.ValuesByNumber[pair.Key.FieldNumber] = cloned;
            }
            return newSet;
        }
    }

    /// <summary>
    /// Used for keeping track of extensions in messages. 
    /// <see cref="IExtensionMessage{T}"/> methods route to this set.
    /// 
    /// Most users will not need to use this class directly
    /// </summary>
    /// <typeparam name="TTarget">The message type that extensions in this set target</typeparam>
    public sealed class ExtensionSet<TTarget> where TTarget : IExtensionMessage<TTarget>
    {
        internal Dictionary<Extension, IExtensionValue> ValuesByIdentifier { get; } = new Dictionary<Extension, IExtensionValue>();
        internal Dictionary<int, IExtensionValue> ValuesByNumber { get; } = new Dictionary<int, IExtensionValue>();

        /// <summary>
        /// Gets a hash code of the set
        /// </summary>
        public override int GetHashCode()
        {
            int ret = typeof(TTarget).GetHashCode();
            foreach (KeyValuePair<Extension, IExtensionValue> field in ValuesByIdentifier)
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
            if (ValuesByIdentifier.Count != otherSet.ValuesByIdentifier.Count)
            {
                return false;
            }
            foreach (var pair in ValuesByIdentifier)
            {
                IExtensionValue secondValue;
                if (!otherSet.ValuesByIdentifier.TryGetValue(pair.Key, out secondValue))
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
            foreach (var value in ValuesByIdentifier.Values)
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
            foreach (var value in ValuesByIdentifier.Values)
            {
                value.WriteTo(stream);
            }
        }
    }
}
