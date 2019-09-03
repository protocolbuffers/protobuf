#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
