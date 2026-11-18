#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;

namespace Google.Protobuf.Examples.AddressBook
{
    /// <summary>
    /// Entry point. Repeatedly prompts user for an action to take, delegating actual behaviour
    /// to individual actions. Each action has its own Main method, so that it can be used as an
    /// individual complete program.
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