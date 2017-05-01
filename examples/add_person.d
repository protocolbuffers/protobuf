// See README.txt for information and build instructions.

import std.array;
import std.conv;
import std.exception;
import std.stdio;
import std.string;
import google.protobuf;
import tutorial.addressbook;

/// This function fills in a Person message based on user input.
Person promptForAddress()
{
    auto person = new Person;

    write("Enter person ID number: ");
    readf("%d", &person.id);
    readln!string;

    write("Enter name: ");;
    person.name = readln!string.strip;

    write("Enter email address (blank for none): ");
    person.email = readln!string.strip;

    while (true)
    {
        auto phoneNumber = new Person.PhoneNumber;
        write("Enter a phone number (or leave blank to finish): ");
        phoneNumber.number = readln!string.strip;
        if (phoneNumber.number.empty)
            break;

        write("Is this a mobile, home, or work phone? ");
        string phoneType = readln!string.strip.toUpper;
        try
        {
            phoneNumber.type = phoneType.to!(Person.PhoneType);
        }
        catch (ConvException exception)
        {
            writeln("Unknown phone type.  Using default.");
        }

        person.phones ~= phoneNumber;
    }

    return person;
}

// Main function:  Reads the entire address book from a file,
//   adds one person based on user input, then writes it back out to the same
//   file.
int main(string[] args)
{
    if (args.length != 2)
    {
        stderr.writefln("Usage:  %s ADDRESS_BOOK_FILE", args[0]);
        return -1;
    }

    auto addressBook = new AddressBook;
    try
    {
        auto input = File(args[1], "rb");
        scope(exit) input.close;
        ubyte[] inputBuffer = input.rawRead(new ubyte[64 * 1024]);
        inputBuffer.fromProtobuf!AddressBook(addressBook);
    }
    catch (ErrnoException exception)
    {
        stderr.writefln("%s: File not found.  Creating a new file.", args[1]);
    }

    // Add an address.
    addressBook.people ~= promptForAddress;

    {
        auto output = File(args[1], "wb");
        scope(exit) output.close;
        output.rawWrite(addressBook.toProtobuf.array);
    }

    return 0;
}
