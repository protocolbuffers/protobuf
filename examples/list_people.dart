import 'dart:io';

import 'dart_tutorial/addressbook.pb.dart';
import 'dart_tutorial/addressbook.pbenum.dart';

/// Iterates though all people in the AddressBook and prints info about them.
void printAddressBook(AddressBook addressBook) {
  for (var person in addressBook.people) {
    print('Person ID: ${person.id}');
    print('  Name: ${person.name}');
    if (person.hasEmail()) {
      print('  E-mail address:${person.email}');
    }

    for (var phoneNumber in person.phones) {
      switch (phoneNumber.type) {
        case Person_PhoneType.MOBILE:
          print('   Mobile phone #: ');
          break;
        case Person_PhoneType.HOME:
          print('   Home phone #: ');
          break;
        case Person_PhoneType.WORK:
          print('   Work phone #: ');
          break;
        default:
          print('   Unknown phone #: ');
          break;
      }
      print(phoneNumber.number);
    }
  }
}

/// Reads the entire address book from a file and prints all
/// the information inside.
void main(List<String> arguments) {
  if (arguments.length != 1) {
    print('Usage: list_person ADDRESS_BOOK_FILE');
    exit(-1);
  }

  // Read the existing address book.
  final file = new File(arguments.first);
  final addressBook = new AddressBook.fromBuffer(file.readAsBytesSync());
  printAddressBook(addressBook);
}
