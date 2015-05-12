#region Copyright notice and license

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

#endregion

using System;
using System.Collections.Generic;
using NUnit.Framework;

namespace Google.ProtocolBuffers.Collections
{
    public class PopsicleListTest
    {
        [Test]
        public void MutatingOperationsOnFrozenList()
        {
            PopsicleList<string> list = new PopsicleList<string>();
            list.MakeReadOnly();
            Assert.Throws<NotSupportedException>(() => list.Add(""));
            Assert.Throws<NotSupportedException>(() => list.Clear());
            Assert.Throws<NotSupportedException>(() => list.Insert(0, ""));
            Assert.Throws<NotSupportedException>(() => list.Remove(""));
            Assert.Throws<NotSupportedException>(() => list.RemoveAt(0));
            Assert.Throws<NotSupportedException>(() => list.Add(new[] { "", "" }));
        }

        [Test]
        public void NonMutatingOperationsOnFrozenList()
        {
            PopsicleList<string> list = new PopsicleList<string>();
            list.MakeReadOnly();
            Assert.IsFalse(list.Contains(""));
            Assert.AreEqual(0, list.Count);
            list.CopyTo(new string[5], 0);
            list.GetEnumerator();
            Assert.AreEqual(-1, list.IndexOf(""));
            Assert.IsTrue(list.IsReadOnly);
        }

        [Test]
        public void MutatingOperationsOnFluidList()
        {
            PopsicleList<string> list = new PopsicleList<string>();
            list.Add("");
            list.Clear();
            list.Insert(0, "");
            list.Remove("");
            list.Add("x"); // Just to make the next call valid
            list.RemoveAt(0);
        }

        [Test]
        public void NonMutatingOperationsOnFluidList()
        {
            PopsicleList<string> list = new PopsicleList<string>();
            Assert.IsFalse(list.Contains(""));
            Assert.AreEqual(0, list.Count);
            list.CopyTo(new string[5], 0);
            list.GetEnumerator();
            Assert.AreEqual(-1, list.IndexOf(""));
            Assert.IsFalse(list.IsReadOnly);
        }

        [Test]
        public void DoesNotAddNullEnumerable()
        {
            PopsicleList<string> list = new PopsicleList<string>();
            Assert.Throws<ArgumentNullException>(() => list.Add((IEnumerable<string>) null));
        }

        [Test]
        public void DoesNotAddRangeWithNull()
        {
            PopsicleList<string> list = new PopsicleList<string>();
            // TODO(jonskeet): Change to ArgumentException? The argument isn't null...
            Assert.Throws<ArgumentNullException>(() => list.Add(new[] {"a", "b", null}));
        }

        [Test]
        public void DoesNotAddNull()
        {
            PopsicleList<string> list = new PopsicleList<string>();
            Assert.Throws<ArgumentNullException>(() => list.Add((string) null));
        }

        [Test]
        public void DoesNotSetNull()
        {
            PopsicleList<string> list = new PopsicleList<string>();
            list.Add("a");
            Assert.Throws<ArgumentNullException>(() => list[0] = null);
        }
    }
}