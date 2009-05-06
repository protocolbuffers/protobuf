This directory contains example code that uses Protocol Buffers to manage an
address book.  Two programs are provided, each with three different
implementations, one written in each of C++, Java, and Python.  The add_person
example adds a new person to an address book, prompting the user to input
the person's information.  The list_people example lists people already in the
address book.  The examples use the exact same format in all three languages,
so you can, for example, use add_person_java to create an address book and then
use list_people_python to read it.

You must install the protobuf package before you can build these.

To build all the examples (on a unix-like system), simply run "make".  This
creates the following executable files in the current directory:
  add_person_cpp     list_people_cpp
  add_person_java    list_people_java
  add_person_python  list_people_python

If you only want to compile examples in one language, use "make cpp"*,
"make java", or "make python".

All of these programs simply take an address book file as their parameter.
The add_person programs will create the file if it doesn't already exist.

These examples are part of the Protocol Buffers tutorial, located at:
  http://code.google.com/apis/protocolbuffers/docs/tutorials.html

* Note that on some platforms you may have to edit the Makefile and remove
"-lpthread" from the linker commands (perhaps replacing it with something else).
We didn't do this automatically because we wanted to keep the example simple.
