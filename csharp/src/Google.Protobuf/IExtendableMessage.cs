#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Collections;

namespace Google.Protobuf
{
    /// <summary>
    /// Generic interface for a Protocol Buffers message containing one or more extensions, where the type parameter is expected to be the same type as the implementation class.
    /// This interface is experiemental and is subject to change.
    /// </summary>
    public interface IExtendableMessage<T> : IMessage<T> where T : IExtendableMessage<T>
    {
        /// <summary>
        /// Gets the value of the specified extension
        /// </summary>
        TValue GetExtension<TValue>(Extension<T, TValue> extension);

        /// <summary>
        /// Gets the value of the specified repeated extension or null if the extension isn't registered in this set.
        /// For a version of this method that never returns null, use <see cref="IExtendableMessage{T}.GetOrInitializeExtension{TValue}(RepeatedExtension{T, TValue})"/>
        /// </summary>
        RepeatedField<TValue> GetExtension<TValue>(RepeatedExtension<T, TValue> extension);

        /// <summary>
        /// Gets the value of the specified repeated extension, registering it if it hasn't already been registered.
        /// </summary>
        RepeatedField<TValue> GetOrInitializeExtension<TValue>(RepeatedExtension<T, TValue> extension);

        /// <summary>
        /// Sets the value of the specified extension
        /// </summary>
        void SetExtension<TValue>(Extension<T, TValue> extension, TValue value);

        /// <summary>
        /// Gets whether the value of the specified extension is set
        /// </summary>
        bool HasExtension<TValue>(Extension<T, TValue> extension);

        /// <summary>
        /// Clears the value of the specified extension
        /// </summary>
        void ClearExtension<TValue>(Extension<T, TValue> extension);

        /// <summary>
        /// Clears the value of the specified repeated extension
        /// </summary>
        void ClearExtension<TValue>(RepeatedExtension<T, TValue> extension);
    }
}
