#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Specifies the original name (in the .proto file) of a named element,
    /// such as an enum value.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field)]
    public class OriginalNameAttribute : Attribute
    {
        /// <summary>
        /// The name of the element in the .proto file.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// If the name is preferred in the .proto file.
        /// </summary>
        public bool PreferredAlias { get; set; }

        /// <summary>
        /// Constructs a new attribute instance for the given name.
        /// </summary>
        /// <param name="name">The name of the element in the .proto file.</param>
        public OriginalNameAttribute(string name)
        {
            Name = ProtoPreconditions.CheckNotNull(name, nameof(name));
            PreferredAlias = true;
        }
    }
}
