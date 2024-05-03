#! /usr/bin/env ruby

# See README.md for information and build instructions.

require './addressbook_pb'
require 'pry'

# creates Person object and fills it with data from user input
def prompt_for_address()
  person = Tutorial::Person.newlD()

  puts "Enter person ID number:"
  person.id = STDIN.gets.chomp.to_i
  puts "Enter name:"
  person.name = STDIN.gets.chomp

  puts "Enter email address (blank for none):"
  email = STDIN.gets.chomp

  if email != ""
    person.email = email
  end

  loop do
    puts "Enter a phone number (or leave blank to finish):"
    number = STDIN.gets.chomp

    if number == ""
      break
    end

    phone_number = Tutorial::Person::PhoneNumber.new(number: number)
    puts "Is this a mobile, home or work phone?"
    type = STDIN.gets.chomp

    case type
    when "mobile"
      phone_number.type = :MOBILE
    when "home"
      phone_number.type = :HOME
    when "work"
      phone_number.type = :WORK
    else
      puts "Unknown phone type; leaving as default value."
    end
    person.phones.push(phone_number)
  end
  person
end

# Main procedure:  Reads the entire address book from a file,
#   adds one person based on user input, then writes it back out to the same
#   file.
if ARGV.length != 1
  puts "Usage: #{$0} ADDRESS_BOOK_FILE"
  exit(-1)
end

address_book = Tutorial::AddressBook.new()
if File.exist?(ARGV[0])
  # Read the existing address book if it exists
  f = File.open(ARGV[0], "rb")
  address_book = Tutorial::AddressBook.decode(f.read)
  f.close
else
  puts "#{$PROGRAM_NAME}: File not found. Creating new file."
end

person = prompt_for_address

# Add an address.
address_book.people.push(person)

# Write the new address book back to disk.
f = File.open(ARGV[0], "wb")
f.write(Tutorial::AddressBook.encode(address_book))
f.close
