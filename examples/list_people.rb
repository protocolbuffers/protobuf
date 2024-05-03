#! /usr/bin/env ruby

require './addressbook_pb'
require 'pry'

# Iterates though all people in the AddressBook and prints info about them.
def list_people(address_book)
  address_book.people.each do |person|
    puts "Person ID: #{person.id}"
    puts "  Name: #{person.name}"
    if person.email != ""
      puts "  Email: #{person.email}"
    end

    person.phones.each do |phone_number|
      type =
        case phone_number.type
        when :MOBILE
          "Mobile phone"
        when :HOME
          "Home phone"
        when :WORK
          "Work phone"
        end
      puts "  #{type} #: #{phone_number.number}"
    end
  end
end

# Main procedure:  Reads the entire address book from a file and prints all
#   the information inside.
if ARGV.length != 1
  puts "Usage: #{$PROGRAM_NAME} ADDRESS_BOOK_FILE"
  exit(-1)
end

# Read the existing address book.
f = File.open(ARGV[0], "rb")
address_book = Tutorial::AddressBook.decode(f.read)
f.close

list_people(address_book)
