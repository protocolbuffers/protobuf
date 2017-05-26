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
using System.IO;

namespace Google.Protobuf.Examples.AddressBook
{
    internal class ListPeople
    {
        /// <summary>
        /// Iterates though all people in the AddressBook and prints info about them.
        /// </summary>
        private static void Print(AddressBook addressBook)
        {
            foreach (Person person in addressBook.People)
            {
                Console.WriteLine("Person ID: {0}", person.Id);
                Console.WriteLine("  Name: {0}", person.Name);
                if (person.Email != "")
                {
                    Console.WriteLine("  E-mail address: {0}", person.Email);
                }

                foreach (Person.Types.PhoneNumber phoneNumber in person.Phones)
                {
                    switch (phoneNumber.Type)
                    {
                        case Person.Types.PhoneType.Mobile:
                            Console.Write("  Mobile phone #: ");
                            break;
                        case Person.Types.PhoneType.Home:
                            Console.Write("  Home phone #: ");
                            break;
                        case Person.Types.PhoneType.Work:
                            Console.Write("  Work phone #: ");
                            break;
                    }
                    Console.WriteLine(phoneNumber.Number);
                }
            }
        }

        /// <summary>
        /// Entry point - loads the addressbook and then displays it.
        /// </summary>
        public static int Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.Error.WriteLine("Usage:  ListPeople ADDRESS_BOOK_FILE");
                return 1;
            }

            if (!File.Exists(args[0]))
            {
                Console.WriteLine("{0} doesn't exist. Add a person to create the file first.", args[0]);
                return 0;
            }

            // Read the existing address book.
            using (Stream stream = File.OpenRead(args[0]))
            {
                AddressBook addressBook = AddressBook.Parser.ParseFrom(stream);
                Print(addressBook);
            }
            return 0;
        }
    }
}