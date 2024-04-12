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

#if !NET5_0_OR_GREATER
// Copied with permission from https://github.com/dotnet/runtime/tree/8fbf206d0e518b45ca855832e8bfb391afa85972/src/libraries/System.Private.CoreLib/src/System/Diagnostics/CodeAnalysis
namespace System.Diagnostics.CodeAnalysis
{
    /// <summary>
    /// Indicates that the specified method requires dynamic access to code that is not referenced
    /// statically, for example through <see cref="System.Reflection"/>.
    /// </summary>
    /// <remarks>
    /// This allows tools to understand which methods are unsafe to call when removing unreferenced
    /// code from an application.
    /// </remarks>
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class, Inherited = false)]
    internal sealed class RequiresUnreferencedCodeAttribute : Attribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RequiresUnreferencedCodeAttribute"/> class
        /// with the specified message.
        /// </summary>
        /// <param name="message">
        /// A message that contains information about the usage of unreferenced code.
        /// </param>
        public RequiresUnreferencedCodeAttribute(string message)
        {
            Message = message;
        }

        /// <summary>
        /// Gets a message that contains information about the usage of unreferenced code.
        /// </summary>
        public string Message { get; }

        /// <summary>
        /// Gets or sets an optional URL that contains more information about the method,
        /// why it requires unreferenced code, and what options a consumer has to deal with it.
        /// </summary>
        public string Url { get; set; }
    }
}
#endif
