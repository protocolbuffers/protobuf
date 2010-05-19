Welcome to the C# port of Google Protocol Buffers, written by Jon Skeet
(skeet@pobox.com) based on the work of many talented people.

For more information about this port, visit its homepage:
http://protobuf-csharp-port.googlecode.com

For more information about Protocol Buffers in general, visit the
project page for the C++, Java and Python project:
http://protobuf.googlecode.com


Release 0.9.1
-------------

Fix to release 0.9:

- Include protos in binary download
- Fix issue 10: incorrect encoding of packed fields when serialized
  size wasn't fetched first


Release 0.9
-----------

Due to popular demand, I have built a version of the binaries to put
on the web site. Currently these are set at assembly version 0.9,
and an assembly file version of 0.9. This should be seen as a mark
of the readiness of the release process more than the stability of
the code. As far as I'm aware, the code itself is perfectly fine: I
certainly have plans for more features particularly around making
code generation simpler, but you should feel confident about the
parsing and serialization of messages produced with this version of
the library. Of course, if you do find any problems, *please* report
them at the web site.

Currently the downloadable release is built with the snk file which
is in the open source library. I am considering having a privately
held key so that you can check that you're building against a
"blessed" release - feedback on this (and any other aspect of the
release process) is very welcome.
