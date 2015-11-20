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
  https://developers.google.com/protocol-buffers/docs/tutorials

* Note that on some platforms you may have to edit the Makefile and remove
"-lpthread" from the linker commands (perhaps replacing it with something else).
We didn't do this automatically because we wanted to keep the example simple.

## Go ##

The Go example requires a plugin to the protocol buffer compiler, so it is not
build with all the other examples.  See:
  https://github.com/golang/protobuf
for more information about Go protocol buffer support.

First, install the the Protocol Buffers compiler (protoc).
Then, install the Go Protocol Buffers plugin
($GOPATH/bin must be in your $PATH for protoc to find it):
  go get github.com/golang/protobuf/protoc-gen-go

Build the Go samples in this directory with "make go".  This creates the
following executable files in the current directory:
  add_person_go      list_people_go
To run the example:
  ./add_person_go addressbook.data
to add a person to the protocol buffer encoded file addressbook.data.  The file
is created if it does not exist.  To view the data, run:
  ./list_people_go addressbook.data

Observe that the C++, Python, and Java examples in this directory run in a
similar way and can view/modify files created by the Go example and vice
versa.
