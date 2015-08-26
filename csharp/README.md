This directory contains the C# Protocol Buffers runtime library.

Status: Alpha - ready for early adopters
========================================

This code is still under significant churn. Unlike the original port,
it only supports proto3 (but not *all* of proto3 yet) - there are no
unknown fields or extensions, for example. protoc will (eventually)
deliberately fail if it is asked to generate C# code for proto2
messages other than descriptor.proto, which is still required for
reflection. (It's currently exposed publicly, but won't be
eventually.)

Also unlike the original port, the new version embraces mutability -
there are no builder types.

Usage
=====

The easiest way how to use C# protobufs is via the `Google.Protobuf`
NuGet package. Just add the NuGet package to your VS project.

Besides C# runtime library, the NuGet package also contains 
precompiled version of `protoc.exe` and a copy of well known `.proto`
files under the package's `tools` directory.

To generate C# files from your `.proto` files, invoke `protoc` with the 
`--csharp_out` option.

Supported platforms
===================

The runtime library is built as a portable class library, supporting:

- .NET 4.5
- Windows 8
- Windows Phone Silverlight 8
- Windows Phone 8.1
- .NET Core (dnxcore)

Building
========

Open the `src/Google.Protobuf.sln` solution in Visual Studio. Click "Build solution" to build the solution. You should be able to run the NUnit test from Test Explorer (you might need to install NUnit Visual Studio add-in).

Supported Visual Studio versions are VS2013 (update 4) and VS2015. On Linux, you can also use Monodevelop 5.9 (older versions might work fine).

History of C# protobufs
=======================

This subtree was originally imported from https://github.com/jskeet/protobuf-csharp-port
and represents the latest development version of C# protobufs, that will now be developed
and maintained by Google. All the development will be done in open, under this repository
(https://github.com/google/protobuf).
