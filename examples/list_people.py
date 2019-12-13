#! /usr/bin/env python

# See README.txt for information and build instructions.

from __future__ import print_function
from google import protobuf
protos = protobuf.protos("addressbook.proto")
import sys


# Iterates though all people in the AddressBook and prints info about them.
def ListPeople(address_book):
  for person in address_book.people:
    print("Person ID:", person.id)
    print("  Name:", person.name)
    if person.email != "":
      print("  E-mail address:", person.email)

    for phone_number in person.phones:
      if phone_number.type == protos.Person.MOBILE:
        print("  Mobile phone #:", end=" ")
      elif phone_number.type == protos.Person.HOME:
        print("  Home phone #:", end=" ")
      elif phone_number.type == protos.Person.WORK:
        print("  Work phone #:", end=" ")
      print(phone_number.number)


# Main procedure:  Reads the entire address book from a file and prints all
#   the information inside.
if len(sys.argv) != 2:
  print("Usage:", sys.argv[0], "ADDRESS_BOOK_FILE")
  sys.exit(-1)

address_book = protos.AddressBook()

# Read the existing address book.
with open(sys.argv[1], "rb") as f:
  address_book.ParseFromString(f.read())

ListPeople(address_book)
