#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.IO;
using System.Reflection;

namespace Google.Protobuf.ProtoDump
{
    /// <summary>
    /// Small utility to load a binary message and dump it in JSON format.
    /// </summary>
    internal class Program
    {
        private static int Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.Error.WriteLine("Usage: Google.Protobuf.JsonDump <descriptor type name> <input data>");
                Console.Error.WriteLine("The descriptor type name is the fully-qualified message name,");
                Console.Error.WriteLine("including assembly e.g. ProjectNamespace.Message,Company.Project");
                return 1;
            }
            Type type = Type.GetType(args[0]);
            if (type == null)
            {
                Console.Error.WriteLine("Unable to load type {0}.", args[0]);
                return 1;
            }
            if (!typeof(IMessage).GetTypeInfo().IsAssignableFrom(type))
            {
                Console.Error.WriteLine("Type {0} doesn't implement IMessage.", args[0]);
                return 1;
            }
            IMessage message = (IMessage) Activator.CreateInstance(type);
            using (var input = File.OpenRead(args[1]))
            {
                message.MergeFrom(input);
            }
            Console.WriteLine(message);
            return 0;
        }
    }
}