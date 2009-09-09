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
using System.IO;

namespace Google.ProtocolBuffers.ProtoDump
{
  /// <summary>
  /// Small utility to load a binary message and dump it in text form
  /// </summary>
  class Program {
    static int Main(string[] args) {
      if (args.Length != 2) {
        Console.Error.WriteLine("Usage: ProtoDump <descriptor type name> <input data>");
        Console.Error.WriteLine("The descriptor type name is the fully-qualified message name,");
        Console.Error.WriteLine("including assembly e.g. ProjectNamespace.Message,Company.Project");
        return 1;
      }
      IMessage defaultMessage;
      try {
        defaultMessage = MessageUtil.GetDefaultMessage(args[0]);
      } catch (ArgumentException e) {
        Console.Error.WriteLine(e.Message);
        return 1;
      }
      try {
        IBuilder builder = defaultMessage.WeakCreateBuilderForType();
        if (builder == null) {
          Console.Error.WriteLine("Unable to create builder");
          return 1;
        }
        byte[] inputData = File.ReadAllBytes(args[1]);
        builder.WeakMergeFrom(ByteString.CopyFrom(inputData));
        Console.WriteLine(TextFormat.PrintToString(builder.WeakBuild()));
        return 0;
      } catch (Exception e) {
        Console.Error.WriteLine("Error: {0}", e.Message);
        Console.Error.WriteLine();
        Console.Error.WriteLine("Detailed exception information: {0}", e);
        return 1;
      }
    }
  }
}