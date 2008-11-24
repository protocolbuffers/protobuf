
using System;
using System.IO;

namespace Google.ProtocolBuffers.Examples.AddressBook {
  class ListPeople {
    /// <summary>
    /// Iterates though all people in the AddressBook and prints info about them.
    /// </summary>
    static void Print(AddressBook addressBook) {
      foreach (Person person in addressBook.PersonList) {
        Console.WriteLine("Person ID: {0}", person.Id);
        Console.WriteLine("  Name: {0}", person.Name);
        if (person.HasEmail) {
          Console.WriteLine("  E-mail address: {0}", person.Email);
        }

        foreach (Person.Types.PhoneNumber phoneNumber in person.PhoneList) {
          switch (phoneNumber.Type) {
            case Person.Types.PhoneType.MOBILE:
              Console.Write("  Mobile phone #: ");
              break;
            case Person.Types.PhoneType.HOME:
              Console.Write("  Home phone #: ");
              break;
            case Person.Types.PhoneType.WORK:
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
    public static int Main(string[] args) {
      if (args.Length != 1) {
        Console.Error.WriteLine("Usage:  ListPeople ADDRESS_BOOK_FILE");
        return 1;
      }

      if (!File.Exists(args[0])) {
        Console.WriteLine("{0} doesn't exist. Add a person to create the file first.", args[0]);
        return 0;
      }

      // Read the existing address book.
      using (Stream stream = File.OpenRead(args[0])) {
        AddressBook addressBook = AddressBook.ParseFrom(stream);
        Print(addressBook);
      }
      return 0;
    }
  }
}
