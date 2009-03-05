Contents
--------

This folder contains three kinds of file:

- Code, such as ProtoBench.java, to build the benchmarking framework.
- Protocol buffer definitions (.proto files)
- Sample data files

If we end up with a lot of different benchmarks it may be worth
separating these out info different directories, but while there are
so few they might as well all be together.

Running a benchmark (Java)
--------------------------

1) Build protoc and the Java protocol buffer library. The examples
   below assume a jar file (protobuf.jar) has been built and copied
   into this directory.

2) Build ProtoBench:
   $ javac -d tmp -cp protobuf.jar ProtoBench.java
   
3) Generate code for the relevant benchmark protocol buffer, e.g.
   $ protoc --java_out=tmp google_size.proto google_speed.proto
   
4) Build the generated code, e.g.
   $ cd tmp
   $ javac -d . -cp ../protobuf.jar benchmarks/*.java
           
5) Run the test. Arguments are given in pairs - the first argument
   is the descriptor type; the second is the filename. For example:
   $ java -cp .;../protobuf.jar com.google.protocolbuffers.ProtoBench
          benchmarks.GoogleSize$SizeMessage1 ../google_message1.dat
          benchmarks.GoogleSpeed$SpeedMessage1 ../google_message1.dat
          benchmarks.GoogleSize$SizeMessage2 ../google_message2.dat
          benchmarks.GoogleSpeed$SpeedMessage2 ../google_message2.dat
          
6) Wait! Each test runs for around 30 seconds, and there are 6 tests
   per class/data combination. The above command would therefore take
   about 12 minutes to run.

   
Benchmarks available
--------------------

From Google:
google_size.proto and google_speed.proto, messages
google_message1.dat and google_message2.dat. The proto files are
equivalent, but optimized differently.
