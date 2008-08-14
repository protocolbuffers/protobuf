To build the C# code:

1) Copy protoc.exe, libprotoc.dll and libprotobuf.dll into
this directory from the native code output (or use a path when
running protoc.exe below)

2) Copy Google.ProtocolBuffers.dll from the built C# library code to
this directory

3) Run this to generate the code:
protoc --csharp_out=. addressbook.proto

4) Build the AddPerson app:
csc /r:Google.ProtocolBuffers.dll AddPerson.cs AddressBookProtos.cs

5) Build the ListPeople app:
csc /r:Google.ProtocolBuffers.dll ListPeople.cs AddressBookProtos.cs


