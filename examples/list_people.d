// See README.txt for information and build instructions.

import std.conv;
import std.stdio;
import std.string;
import google.protobuf;
import tutorial.addressbook;

/// Iterates though all people in the AddressBook and prints info about them.
void listPeople(const AddressBook addressBook)
{
    foreach (person; addressBook.people)
    {
        writeln("Person ID: ", person.id);
        writeln("  Name: ", person.name);
        if (!person.email.empty)
            writeln("  E-mail address: ", person.email);

        foreach (phone; person.phones) with (phone)
            writefln("  %s phone #: %s", type.to!string.capitalize, number);
    }
}

// Main function:  Reads the entire address book from a file and prints all
//   the information inside.
int main(string[] args)
{
    if (args.length != 2)
    {
        stderr.writefln("Usage:  %s ADDRESS_BOOK_FILE", args[0]);
        return -1;
    }

    auto input = File(args[1], "rb");
    scope(exit) input.close;
    ubyte[] inputBuffer = input.rawRead(new ubyte[64 * 1024]);
    auto addressBook = inputBuffer.fromProtobuf!AddressBook;
    listPeople(addressBook);

    return 0;
}
