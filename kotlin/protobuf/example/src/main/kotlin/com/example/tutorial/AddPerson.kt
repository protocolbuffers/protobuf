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
import com.example.tutorial.AddressBookProtos.Person
import com.example.tutorial.AddressBookProtosKt.PersonKt.phoneNumber
import com.example.tutorial.AddressBookProtosKt.person
import java.io.BufferedReader
import java.io.FileInputStream
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.InputStreamReader
import java.io.PrintStream
import kotlin.system.exitProcess

object AddPerson {
  fun promptForAddress(input: BufferedReader, output: PrintStream): Person {
    return person {
      output.print("Enter person ID: ")
      id = input.readLine().toInt()

      output.print("Enter name: ")
      name = input.readLine()

      output.print("Enter email address (blank for none): ")
      val email = input.readLine()
      if (email.isNotEmpty()) {
        this.email = email
      }

      while (true) {
        output.print("Enter a phone number (or leave blank to finish): ")
        val number = input.readLine()
        if (number.isEmpty()) {
          break
        }

        phone += phoneNumber {
          this.number = number
          output.print("Is this a mobile, home, or work phone? ")
          when (input.readLine()) {
            "mobile" -> type = Person.PhoneType.MOBILE
            "home" -> type = Person.PhoneType.HOME
            "work" -> type = Person.PhoneType.WORK
            else -> output.println("Unknown phone type.  Using default.")
          }
        }
      }
    }
  }

  @JvmStatic
  fun main(args: Array<String>) {
    if (args.size != 1) {
      System.err.println("Usage: AddPerson ADDRESS_BOOK_FILE")
      exitProcess(-1)
    }

    val addressBook = try {
      FileInputStream(args.single()).use {
        AddressBook.parseFrom(it)
      }
    } catch (e: FileNotFoundException) {
      AddressBook.getDefaultInstance()
    }

    val newAddressBook = addressBook.copy {
      BufferedReader(InputStreamReader(System.`in`, Charsets.UTF_8)).use {
        person += promptForAddress(it, System.out)
      }
    }

    FileOutputStream(args.single()).use {
      newAddressBook.writeTo(it)
    }
  }
}