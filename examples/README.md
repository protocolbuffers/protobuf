This directory contains example code that uses Protocol Buffers to manage an
address book.  Two programs are provided, each with three different
implementations, one written in each of C++, Java, and Python.  The _add_person_
example adds a new person to an address book, prompting the user to input
the person's information.  The _list_people_ example lists people already in the
address book.  The examples use the exact same format in all three languages,
so you can, for example, use _add_person_java_ to create an address book and then
use _list_people_python_ to read it.

You must install the protobuf package before you can build these.

To build all the examples (on a unix-like system), simply run `make`.  This
creates the following executable files in the current directory:
  * _add_person_cpp_
  * _list_people_cpp_
  * _add_person_java_
  * _list_people_java_
  * _add_person_python_
  * _list_people_python_

If you only want to compile examples in one language, use
* `make cpp*` &nbsp; _for c++_,
* `make java` &nbsp;_for java_,
* `make python` &nbsp; _for python_.

All of these programs simply take an address book file as their parameter.
The _add_person_ programs will create the file if it doesn't already exist.

These examples are part of the Protocol Buffers [tutorial](https://developers.google.com/protocol-buffers/docs/tutorials).

* Note that on some platforms you may have to edit the `Makefile` and remove
__-lpthread__ from the linker commands (perhaps replacing it with something else).
We didn't do this automatically because we wanted to keep the example simple.

## Go

The Go example requires a plugin to the protocol buffer compiler, so it is not
build with all the other examples.  See [docs](https://github.com/golang/protobuf) for more information about Go protocol buffer support.

First, install the Protocol Buffers compiler ([protoc](https://github.com/google/protobuf)).
Then, install the Go Protocol Buffers plugin by running the command :
`go get github.com/golang/protobuf/protoc-gen-go`
( _$GOPATH/bin_ must be in your _$PATH_ for protoc to find it):

Build the Go samples in this directory with `make go`.  This creates the
following executable files in the current directory:
  * _add_person_go_
  * _list_people_go_

To run the example, run : `./add_person_go addressbook.data`
    it adds a person to the protocol buffer encoded file _addressbook.data_.The file is created if it does not exist.

To view the data, run : `./list_people_go addressbook.data`

Observe that the C++, Python, and Java examples in this directory run in a
similar way and can view/modify files created by the Go example and vice
versa.

