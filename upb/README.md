
# μpb: small, fast C protos

μpb (often written 'upb') is a small
[protobuf](https://github.com/protocolbuffers/protobuf) implementation written
in C.

upb is the core runtime for protobuf languages extensions in
[Ruby](https://github.com/protocolbuffers/protobuf/tree/main/ruby),
[PHP](https://github.com/protocolbuffers/protobuf/tree/main/php), and
[Python](https://github.com/protocolbuffers/protobuf/tree/main/upb/python).

While upb offers a C API, the C API & ABI **are not stable**. For this reason,
upb is not generally offered as a C library for direct consumption, and there
are no releases.

## Features

upb has comparable speed to protobuf C++, but is an order of magnitude smaller
in code size.

Like the main protobuf implementation in C++, it supports:

- a generated API (in C)
- reflection
- binary & JSON wire formats
- text format serialization
- all standard features of protobufs (oneofs, maps, unknown fields, extensions,
  etc.)
- full conformance with the protobuf conformance tests

upb also supports some features that C++ does not:

- **optional reflection:** generated messages are agnostic to whether
  reflection will be linked in or not.
- **no global state:** no pre-main registration or other global state.
- **fast reflection-based parsing:** messages loaded at runtime parse
  just as fast as compiled-in messages.

However there are a few features it does not support:

- text format parsing
- deep descriptor verification: upb's descriptor validation is not as exhaustive
  as `protoc`.

## Install

For Ruby, use [RubyGems](https://rubygems.org/gems/google-protobuf):

```
$ gem install google-protobuf
```

For PHP, use [PECL](https://pecl.php.net/package/protobuf):

```
$ sudo pecl install protobuf
```

For Python, use [PyPI](https://pypi.org/project/protobuf/):

```
$ sudo pip install protobuf
```

Alternatively, you can build and install upb using
[vcpkg](https://github.com/microsoft/vcpkg/) dependency manager:

    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg integrate install
    ./vcpkg install upb

The upb port in vcpkg is kept up to date by microsoft team members and community
contributors.

If the version is out of date, please
[create an issue or pull request](https://github.com/Microsoft/vcpkg) on the
vcpkg repository.

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md).
