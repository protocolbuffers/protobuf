Getting started with Protocol Buffers on Mono
---------------------------------------------

Prerequisites:

o Mono 2.4 or higher. Earlier versions of Mono had too
  many issues with the weird and wonderful generic type
  relationships in Protocol Buffers. (Even Mono 2.4 *did*
  have a few compile-time problems, but I've worked round them.)

o Some sort of Linux/Unix system
  You can try running with Bash on Windows via MINGW32 or
  something similar, but you're on your own :) It's easier
  to build and test everything with .NET if you're on
  Windows.

o The native Protocol Buffers build for your system.
  Get it from http://code.google.com/p/protobuf/
  After building it, copy the executable protoc
  file into this directory.
  
o The NUnit binaries from http://nunit.org
  I generally just download the latest version, which
  may not be the one which goes with nunit.framework.dll
  in ../lib, but I've never found this to be a problem.
  
Building the code with current sources
--------------------------------------

1) Edit buildall.sh to tell it where to find nunit-console.exe
   (and possibly change other options)
   
2) Run buildall.sh from this directory. It should build the
   main library code + tests and ProtoGen code + tests, running
   each set of tests after building it.
   
Note that currently one test is ignored in ServiceTest.cs. This
made the Mono VM blow up - I suspect it's some interaction with
Rhino which doesn't quite work on Mono 2.4. If you want to see a
truly nasty stack trace, just comment out the Ignore attribute in
ServiceTest.cs and rerun.

The binaries will be produced in a bin directory under this one. The
build currently starts from scratch each time, cleaning out the bin
directory first. Once I've decided on a full NAnt or xbuild
strategy, I'll do something a little cleaner.

Rebuilding sources for generated code
-------------------------------------

1) Build the current code first. The bootstrapping issue is why
   the generated source code is in the source repository :) See
   the steps above.
   
2) Run generatesource.sh from this directory. This will create a
   temporary directory, compile the .proto files into a binary
   format, then run ProtoGen to generate .cs files from the binary
   format. It will copy these files to the right places in the tree,
   and finally delete the temporary directory.
   
3) Rebuild to test that your newly generated sources work. (Optionally
   regenerate as well, and hash the generated files to check that
   the new build generates the same code as the old build :)
   
Running the code
----------------

Once you've built the binaries, you should be able to use them just
as if you'd built them with .NET. (And indeed, you should be able to
use binaries built with .NET as if you'd built them with Mono :)

See the getting started guide for more information:
http://code.google.com/p/protobuf-csharp-port/wiki/GettingStarted

FAQ (Frequently Anticipated Questions)
--------------------------------------

Q) This build process sucks! Why aren't you doing X, Y, Z?
A) My Mono skills are limited. My NAnt skills are limited. My
   MSBuild/xbuild skils are limited. My shell script skills are
   limited. Any help is *very* welcome!
   
Q) Why doesn't it build ProtoBench etc?
A) This is a first initial "release" I'll add more bits to
   the build script. I'll be interested to see the results
   of benchmarking it on Mono :)

Any further questions or suggestions? Please email skeet@pobox.com
or leave a request at
http://code.google.com/p/protobuf-csharp-port/issues/list

