# Third-Party Add-ons for Protocol Buffers

This page lists code related to Protocol Buffers which is developed and maintained by third parties.  You may find this code useful, but note that **these projects are not affiliated with or endorsed by Google (unless explicitly marked)**; try them at your own risk.  Also note that many projects here are in the early stages of development and not production-ready.

If you have a project that should be listed here, please [send us a pull request](https://github.com/google/protobuf/pulls) to update this page.

## Programming Languages

These are projects we know about implementing Protocol Buffers for other programming languages:
* Action Script: http://code.google.com/p/protobuf-actionscript3/
* Action Script: https://code.google.com/p/protoc-gen-as3/
* Action Script: https://github.com/matrix3d/JProtoc
* Action Script: https://github.com/zhongfq/protobuf-as3/
* C: https://github.com/protobuf-c/protobuf-c
* C: http://koti.kapsi.fi/jpa/nanopb/
* C: https://github.com/cloudwu/pbc/
* C: https://github.com/haberman/upb/wiki
* C: https://github.com/squidfunk/protobluff
* C++: https://github.com/google/protobuf (Google-official implementation)
* C/C++: http://spbc.sf.net/
* C#: http://code.google.com/p/protobuf-csharp-port
* C#: http://code.google.com/p/protosharp/
* C#: https://silentorbit.com/protobuf/
* C#/.NET/WCF/VB: http://code.google.com/p/protobuf-net/
* Clojure: http://github.com/ninjudd/clojure-protobuf
* Common Lisp: http://github.com/ndantam/s-protobuf
* Common Lisp: http://github.com/brown/protobuf
* D: https://github.com/msoucy/dproto
* D: http://256.makerslocal.org/wiki/index.php/ProtocolBuffer
* D: https://github.com/opticron/ProtocolBuffer
* Dart: https://github.com/dart-lang/dart-protobuf (runtime) https://github.com/dart-lang/dart-protoc-plugin (code generator)
* Delphi: http://sourceforge.net/projects/protobuf-delphi/
* Delphi: http://fundementals.sourceforge.net/dl.html
* Elixir: https://github.com/jeremyong/exprotoc
* Elixir: https://github.com/tony612/protobuf-elixir
* Elm: https://github.com/tiziano88/elm-protobuf
* Erlang: http://github.com/ngerakines/erlang_protobuffs/tree/master
* Erlang: http://piqi.org/
* Erlang: https://code.google.com/p/protoc-gen-erl/
* Erlang: https://github.com/basho/erlang_protobuffs
* Erlang: https://github.com/tomas-abrahamsson/gpb
* Go: https://github.com/golang/protobuf (Google-official implementation)
* Go: https://github.com/akunspy/gopbuf
* Go: https://github.com/gogo/protobuf
* GopherJS: https://github.com/johanbrandhorst/protobuf
* Haskell: http://hackage.haskell.org/package/hprotoc
* Haskell: https://github.com/google/proto-lens (Google-unofficial implementation)
* Haskell: https://github.com/awakesecurity/proto3-suite (code generator) https://github.com/awakesecurity/proto3-wire (binary serializer/deserializer)
* Haxe: https://github.com/Atry/protoc-gen-haxe
* Java: https://github.com/google/protobuf (Google-official implementation)
* Java/Android: https://github.com/square/wire
* Java ME: http://code.google.com/p/protobuf-javame/
* Java ME: http://swingme.sourceforge.net/encode.shtml
* Java ME: http://code.google.com/p/protobuf-j2me/
* Javascript: http://code.google.com/p/protobuf-js/
* Javascript: http://github.com/sirikata/protojs
* Javascript: https://github.com/dcodeIO/ProtoBuf.js
* Javascript: http://code.google.com/p/protobuf-for-node/
* Javascript: http://code.google.com/p/protostuff/
* Julia: https://github.com/tanmaykm/ProtoBuf.jl
* Kotlin: https://github.com/Kotlin/kotlinx.serialization
* Lua: http://code.google.com/p/protoc-gen-lua/
* Lua: http://github.com/indygreg/lua-protobuf
* Lua: https://github.com/Neopallium/lua-pb
* Matlab: http://code.google.com/p/protobuf-matlab/
* Mercury: http://code.google.com/p/protobuf-mercury/
* Objective C: http://code.google.com/p/protobuf-objc/
* Objective C: https://github.com/alexeyxo/protobuf-objc
* OCaml: http://piqi.org/
* Perl: http://groups.google.com/group/protobuf-perl
* Perl: http://search.cpan.org/perldoc?Google::ProtocolBuffers
* Perl: https://metacpan.org/pod/Google::ProtocolBuffers::Dynamic
* Perl/XS: http://code.google.com/p/protobuf-perlxs/
* PHP: http://code.google.com/p/pb4php/
* PHP: https://github.com/allegro/php-protobuf/
* PHP: https://github.com/chobie/php-protocolbuffers
* PHP: http://drslump.github.com/Protobuf-PHP
* Prolog: http://www.swi-prolog.org/pldoc/package/protobufs.html
* Python: https://github.com/google/protobuf (Google-official implementation)
* Python: http://eigenein.github.com/protobuf/
* R: http://cran.r-project.org/package=RProtoBuf
* Ruby: http://code.google.com/p/ruby-protobuf/
* Ruby: http://github.com/mozy/ruby-protocol-buffers
* Ruby: https://github.com/bmizerany/beefcake/tree/master/lib/beefcake
* Ruby: https://github.com/localshred/protobuf
* Rust: https://github.com/stepancheg/rust-protobuf/
* Scala: http://github.com/jeffplaisance/scala-protobuf
* Scala: http://code.google.com/p/protobuf-scala
* Scala: https://github.com/SandroGrzicic/ScalaBuff
* Scala: http://trueaccord.github.io/ScalaPB/
* Swift: https://github.com/alexeyxo/protobuf-swift
* Swift: https://github.com/apple/swift-protobuf/
* Vala: https://launchpad.net/protobuf-vala
* Visual Basic: http://code.google.com/p/protobuf-net/

## RPC Implementations

GRPC (http://www.grpc.io/) is Google's RPC implementation for Protocol Buffers. There are other third-party RPC implementations as well.  Some of these actually work with Protocol Buffers service definitions (defined using the `service` keyword in `.proto` files) while others just use Protocol Buffers message objects.

* https://github.com/grpc/grpc (C++, Node.js, Python, Ruby, Objective-C, PHP, C#, Google-official implementation)
* http://zeroc.com/ice.html (Multiple languages)
* http://code.google.com/p/protobuf-net/ (C#/.NET/WCF/VB)
* https://launchpad.net/txprotobuf/ (Python)
* https://github.com/modeswitch/protobuf-rpc (Python)
* http://code.google.com/p/protobuf-socket-rpc/ (Java, Python)
* http://code.google.com/p/proto-streamer/ (Java)
* http://code.google.com/p/server1/ (C++)
* http://deltavsoft.com/RcfUserGuide/Protobufs (C++)
* http://code.google.com/p/protobuf-mina-rpc/ (Python client, Java server)
* http://code.google.com/p/casocklib/ (C++)
* http://code.google.com/p/cxf-protobuf/ (Java)
* http://code.google.com/p/protobuf-remote/ (C++/C#)
* http://code.google.com/p/protobuf-rpc-pro/ (Java)
* https://code.google.com/p/eneter-protobuf-serializer/ (Java/.NET)
* http://www.deltavsoft.com/RCFProto.html (C++/Java/Python/C#)
* https://github.com/robbinfan/claire-protorpc (C++)
* https://github.com/BaiduPS/sofa-pbrpc (C++)
* https://github.com/ebencheung/arab (C++)
* http://code.google.com/p/protobuf-csharp-rpc/ (C#)
* https://github.com/thesamet/rpcz (C++/Python, based on ZeroMQ)
* https://github.com/w359405949/libmaid (C++, Python)
* https://github.com/madwyn/libpbrpc (C++)
* https://github.com/SeriousMa/grpc-protobuf-validation (Java)
* https://github.com/tony612/grpc-elixir (Elixir)
* https://github.com/johanbrandhorst/protobuf (GopherJS)
* https://github.com/awakesecurity/gRPC-haskell (Haskell)
* https://github.com/Yeolar/raster (C++)

## Other Utilities

There are miscellaneous other things you may find useful as a Protocol Buffers developer.

* [Bazel Build](https://bazel.build)
    * [rules_closure](https://github.com/bazelbuild/rules_closure) `js-closure`
    * [rules_go](https://github.com/bazelbuild/rules_go) `go`
    * [rules_protobuf](https://github.com/pubref/rules_protobuf) `java` `c++` `c#` `go` `js-closure` `js-node` `python` `ruby`
* [NetBeans IDE plugin](http://code.google.com/p/protobuf-netbeans-plugin/)
* [Wireshark/Ethereal packet sniffer plugin](http://code.google.com/p/protobuf-wireshark/)
* [Alternate encodings (JSON, XML, HTML) for Java protobufs](http://code.google.com/p/protobuf-java-format/)
* [Another JSON encoder/decoder for Java](https://github.com/sijuv/protobuf-codec)
* [Editor for serialized protobufs](http://code.google.com/p/protobufeditor/)
* [Intellij IDEA plugin](http://github.com/nnmatveev/idea-plugin-protobuf)
* [TextMate syntax highlighting](http://github.com/michaeledgar/protobuf-tmbundle)
* [Oracle PL SQL plugin](http://code.google.com/p/protocol-buffer-plsql/)
* [Eclipse editor for protobuf (from Google)](http://code.google.com/p/protobuf-dt/)
* [C++ Builder compatible protobuf](https://github.com/saadware/protobuf-cppbuilder)
* Maven Protobuf Compiler Plugin
    * By xolstice.org ([Documentation](https://www.xolstice.org/protobuf-maven-plugin/)) ([Source](https://github.com/xolstice/protobuf-maven-plugin/)) [![Maven Central](https://img.shields.io/maven-central/v/org.xolstice.maven.plugins/protobuf-maven-plugin.svg)](https://repo1.maven.org/maven2/org/xolstice/maven/plugins/protobuf-maven-plugin/)
    * http://igor-petruk.github.com/protobuf-maven-plugin/
    * http://code.google.com/p/maven-protoc-plugin/
    * https://github.com/os72/protoc-jar-maven-plugin
* [Documentation generator plugin (Markdown/HTML/DocBook/...)](https://github.com/pseudomuto/protoc-gen-doc)
* [DocBook generator for .proto files](http://code.google.com/p/protoc-gen-docbook/)
* [Protobuf for nginx module](https://github.com/dbcode/protobuf-nginx/)
* [RSpec matchers and Cucumber step defs for testing Protocol Buffers](https://github.com/connamara/protobuf_spec)
* [Sbt plugin for Protocol Buffers](https://github.com/Atry/sbt-cppp)
* [Gradle Protobuf Plugin](https://github.com/aantono/gradle-plugin-protobuf)
* [Multi-platform executable JAR and Java API for protoc](https://github.com/os72/protoc-jar)
* [Python scripts to convert between Protocol Buffers and JSON](https://github.com/NextTuesday/py-pb-converters)
* [Visual Studio Language Service support for Protocol Buffers](http://visualstudiogallery.msdn.microsoft.com/4bc0f38c-b058-4e05-ae38-155e053c19c5)
* [C++ library for serialization/de-serialization between Protocol Buffers and JSON.](https://github.com/yinqiwen/pbjson)
* [ProtoBuf with Java EE7 Expression Language 3.0; pure Java ProtoBuf Parser and Builder.](https://github.com/protobufel/protobuf-el)
* [Notepad++ Syntax Highlighting for .proto files](https://github.com/chai2010/notepadplus-protobuf)
* [Linter for .proto files](https://github.com/ckaznocha/protoc-gen-lint)
* [Protocol Buffers Dynamic Schema - create protobuf schemas programmatically (Java)](https://github.com/os72/protobuf-dynamic)
* [Make protoc plugins in NodeJS](https://github.com/konsumer/node-protoc-plugin)
* [ProfaneDB - A Protocol Buffers database](https://profanedb.gitlab.io)
* [Protocol Buffer property-based testing utility and example message generator (Python / Hypothesis)](https://github.com/CurataEng/hypothesis-protobuf)
