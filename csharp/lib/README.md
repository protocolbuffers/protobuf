This directory used to contain the binary dependencies, but they were removed during a cleanup associated with importing the project
into google/protobuf repository. Before we figure a better way to manage the dependencies, you will have to manually download the dependencies 
to be able to build the test projects:

1. Download https://github.com/jskeet/protobuf-csharp-port/archive/2.4.1.555.zip
2. Open the archive and copy following files into this directory:
   * `lib/Microsoft.Silverlight.Testing/`
   * `lib/NUnit/`
   * `lib/proto.exe`

After that, you should be able to fully build the C# protobufs Visual Studio solutions.

TODO(jtattermusch): the way we pull in dependencies needs to change
