#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

using System;

namespace Google.Protobuf.Examples.AddressBook
{
    /// <summary>
    /// Entry point. Repeatedly prompts user for an action to take, delegating actual behaviour
    /// to individual actions. Each action has its own Main method, so that it can be used as an
    /// invidual complete program.
    /// </summary>
    internal class Program
    {
        private static int Main(string[] args)
        {
            if (args.Length > 1)
            {
                Console.Error.WriteLine("Usage: AddressBook [file]");
                Console.Error.WriteLine("If the filename isn't specified, \"addressbook.data\" is used instead.");
                return 1;
            }
            string addressBookFile = args.Length > 0 ? args[0] : "addressbook.data";

            bool stopping = false;
            while (!stopping)
            {
                Console.WriteLine("Options:");
                Console.WriteLine("  L: List contents");
                Console.WriteLine("  A: Add new person");
                Console.WriteLine("  Q: Quit");
                Console.Write("Action? ");
                Console.Out.Flush();
                char choice = Console.ReadKey().KeyChar;
                Console.WriteLine();
                try
                {
                    switch (choice)
                    {
                        case 'A':
                        case 'a':
                            AddPerson.Main(new string[] {addressBookFile});
                            break;
                        case 'L':
                        case 'l':
                            ListPeople.Main(new string[] {addressBookFile});
                            break;
                        case 'Q':
                        case 'q':
                            stopping = true;
                            break;
                        default:
                            Console.WriteLine("Unknown option: {0}", choice);
                            break;
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine("Exception executing action: {0}", e);
                }
                Console.WriteLine();
            }
            return 0;
        }
    }
}