/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.tutorial

import com.example.tutorial.AddressBookProtos.AddressBook
import java.io.FileInputStream
import kotlin.system.exitProcess

object ListPeople {
  fun print(addressBook: AddressBook) {
    for (person in addressBook.personList) {
      println("Person ID: ${person.id}")
      println("  Name: ${person.name}")
      if (person.hasEmail()) {
        println("  E-mail address: ${person.email}")
      }
      for (phoneNumber in person.phoneList) {
        when (phoneNumber.type) {
          AddressBookProtos.Person.PhoneType.MOBILE -> print("  Mobile phone #: ")
          AddressBookProtos.Person.PhoneType.HOME -> print("  Home phone #: ")
          AddressBookProtos.Person.PhoneType.WORK -> print("  Work phone #: ")
        }
        println(phoneNumber.number)
      }
    }
  }

  @JvmStatic
  fun main(args: Array<String>) {
    if (args.size != 1) {
      System.err.println("Usage:  ListPeople ADDRESS_BOOK_FILE")
      exitProcess(-1)
    }
    print(FileInputStream(args.single()).use {
      AddressBook.parseFrom(it)
    })
  }
}