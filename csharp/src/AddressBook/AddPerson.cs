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
    internal class AddPerson
    {
        /// <summary>
        /// Builds a person based on user input
        /// </summary>
        private static Person PromptForAddress(TextReader input, TextWriter output)
        {
            Person person = new Person();

            output.Write("Enter person ID: ");
            person.Id = int.Parse(input.ReadLine());

            output.Write("Enter name: ");
            person.Name = input.ReadLine();

            output.Write("Enter email address (blank for none): ");
            string email = input.ReadLine();
            if (email.Length > 0)
            {
                person.Email = email;
            }

            while (true)
            {
                output.Write("Enter a phone number (or leave blank to finish): ");
                string number = input.ReadLine();
                if (number.Length == 0)
                {
                    break;
                }

                Person.Types.PhoneNumber phoneNumber = new Person.Types.PhoneNumber { Number = number };

                output.Write("Is this a mobile, home, or work phone? ");
                String type = input.ReadLine();
                switch (type)
                {
                    case "mobile":
                        phoneNumber.Type = Person.Types.PhoneType.Mobile;
                        break;
                    case "home":
                        phoneNumber.Type = Person.Types.PhoneType.Home;
                        break;
                    case "work":
                        phoneNumber.Type = Person.Types.PhoneType.Work;
                        break;
                    default:
                        output.Write("Unknown phone type. Using default.");
                        break;
                }

                person.Phones.Add(phoneNumber);
            }
            return person;
        }

        /// <summary>
        /// Entry point - loads an existing addressbook or creates a new one,
        /// then writes it back to the file.
        /// </summary>
        public static int Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.Error.WriteLine("Usage:  AddPerson ADDRESS_BOOK_FILE");
                return -1;
            }

            AddressBook addressBook;

            if (File.Exists(args[0]))
            {
                using (Stream file = File.OpenRead(args[0]))
                {
                    addressBook = AddressBook.Parser.ParseFrom(file);
                }
            }
            else
            {
                Console.WriteLine("{0}: File not found. Creating a new file.", args[0]);
                addressBook = new AddressBook();
            }

            // Add an address.
            addressBook.People.Add(PromptForAddress(Console.In, Console.Out));

            // Write the new address book back to disk.
            using (Stream output = File.OpenWrite(args[0]))
            {
                addressBook.WriteTo(output);
            }
            return 0;
        }
    }
}