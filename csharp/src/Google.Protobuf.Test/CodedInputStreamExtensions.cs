#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using NUnit.Framework;

namespace Google.Protobuf
{
    internal static class CodedInputStreamExtensions
    {
        public static void AssertNextTag(this CodedInputStream input, uint expectedTag)
        {
            uint tag = input.ReadTag();
            Assert.AreEqual(expectedTag, tag);
        }

        public static T ReadMessage<T>(this CodedInputStream stream, MessageParser<T> parser)
            where T : IMessage<T>
        {
            var message = parser.CreateTemplate();
            stream.ReadMessage(message);
            return message;
        }
    }
}
