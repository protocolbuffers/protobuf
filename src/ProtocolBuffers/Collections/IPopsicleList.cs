// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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
using System.Collections.Generic;

namespace Google.ProtocolBuffers.Collections
{
    /// <summary>
    /// A list which has an Add method which accepts an IEnumerable[T].
    /// This allows whole collections to be added easily using collection initializers.
    /// It causes a potential overload confusion if T : IEnumerable[T], but in
    /// practice that won't happen in protocol buffers.
    /// </summary>
    /// <remarks>This is only currently implemented by PopsicleList, and it's likely
    /// to stay that way - hence the name. More genuinely descriptive names are
    /// horribly ugly. (At least, the ones the author could think of...)</remarks>
    /// <typeparam name="T">The element type of the list</typeparam>
    public interface IPopsicleList<T> : IList<T>
    {
        void Add(IEnumerable<T> collection);
    }

    /// <summary>
    /// Used to efficiently cast the elements of enumerations
    /// </summary>
    internal interface ICastArray
    {
        IEnumerable<TItemType> CastArray<TItemType>();
    }
}