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
                using Stream file = File.OpenRead(args[0]);
                addressBook = AddressBook.Parser.ParseFrom(file);
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