#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Interface implemented by all descriptor types.
    /// </summary>
    public interface IDescriptor
    {
        /// <summary>
        /// Returns the name of the entity (message, field etc) being described.
        /// </summary>
        string Name { get; }

        /// <summary>
        /// Returns the fully-qualified name of the entity being described.
        /// </summary>
        string FullName { get; }

        /// <summary>
        /// Returns the descriptor for the .proto file that this entity is part of.
        /// </summary>
        FileDescriptor File { get; }
    }
}