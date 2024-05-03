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