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
    /// Represents a package in the symbol table.  We use PackageDescriptors
    /// just as placeholders so that someone cannot define, say, a message type
    /// that has the same name as an existing package.
    /// </summary>
    internal sealed class PackageDescriptor : IDescriptor
    {
        internal PackageDescriptor(string name, string fullName, FileDescriptor file)
        {
            File = file;
            FullName = fullName;
            Name = name;
        }

        public string Name { get; }
        public string FullName { get; }
        public FileDescriptor File { get; }
    }
}