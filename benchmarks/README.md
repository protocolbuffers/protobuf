
# Protocol Buffers Benchmarks

This directory contains benchmarking schemas and data sets that you
can use to test a variety of performance scenarios against your
protobuf language runtime. If you are looking for performance 
numbers of officially support languages, see [here](
https://github.com/google/protobuf/blob/master/docs/Performance.md)

## Prerequisite

First, you need to follow the instruction in the root directory's README to
build your language's protobuf, then:

### CPP
You need to install [cmake](https://cmake.org/) before building the benchmark.

We are using [google/benchmark](https://github.com/google/benchmark) as the
benchmark tool for testing cpp. This will be automaticly made during build the
cpp benchmark.

The cpp protobuf performance can be improved by linking with [tcmalloc library](
https://gperftools.github.io/gperftools/tcmalloc.html). For using tcmalloc, you
need to build [gpertools](https://github.com/gperftools/gperftools) to generate
libtcmallc.so library.

### Java
We're using maven to build the java benchmarks, which is the same as to build
the Java protobuf. There're no other tools need to install. We're using
[google/caliper](https://github.com/google/caliper) as benchmark tool, which
can be automaticly included by maven.

### Python
We're using python C++ API for testing the generated
CPP proto version of python protobuf, which is also a prerequisite for Python
protobuf cpp implementation. You need to install the correct version of Python
C++ extension package before run generated CPP proto version of Python
protobuf's benchmark. e.g. under Ubuntu, you need to

```
$ sudo apt-get install python-dev
$ sudo apt-get install python3-dev
```
And you also need to make sure `pkg-config` is installed.

### Go
Go protobufs are maintained at [github.com/golang/protobuf](
http://github.com/golang/protobuf). If not done already, you need to install the 
toolchain and the Go protoc-gen-go plugin for protoc. 

To install protoc-gen-go, run:

```
$ go get -u github.com/golang/protobuf/protoc-gen-go
$ export PATH=$PATH:$(go env GOPATH)/bin
```

The first command installs `protoc-gen-go` into the `bin` directory in your local `GOPATH`.
The second command adds the `bin` directory to your `PATH` so that `protoc` can locate the plugin later.

### Big data

There's some optional big testing data which is not included in the directory
initially, you need to run the following command to download the testing data:

```
$ ./download_data.sh
```

After doing this the big data file will automaticly generated in the
benchmark directory.

## Run instructions

To run all the benchmark dataset:

### Java:

```
$ make java
```

### CPP:

```
$ make cpp
```

For linking with tcmalloc:

```
$ env LD_PRELOAD={directory to libtcmalloc.so} make cpp
```

### Python:

We have three versions of python protobuf implementation: pure python, cpp
reflection and cpp generated code. To run these version benchmark, you need to:

#### Pure Python:

```
$ make python-pure-python
```

#### CPP reflection:

```
$ make python-cpp-reflection
```

#### CPP generated code:

```
$ make python-cpp-generated-code
```

### Go
```
$ make go
```

To run a specific dataset or run with specific options:

### Java:

```
$ make java-benchmark
$ ./java-benchmark $(specific generated dataset file name) [$(caliper options)]
```

### CPP:

```
$ make cpp-benchmark
$ ./cpp-benchmark $(specific generated dataset file name) [$(benchmark options)]
```

### Python:

For Python benchmark we have `--json` for outputing the json result

#### Pure Python:

```
$ make python-pure-python-benchmark
$ ./python-pure-python-benchmark [--json] $(specific generated dataset file name)
```

#### CPP reflection:

```
$ make python-cpp-reflection-benchmark
$ ./python-cpp-reflection-benchmark [--json] $(specific generated dataset file name)
```

#### CPP generated code:

```
$ make python-cpp-generated-code-benchmark
$ ./python-cpp-generated-code-benchmark [--json] $(specific generated dataset file name)
```

### Go:
```
$ make go-benchmark
$ ./go-benchmark $(specific generated dataset file name) [go testing options]
```


## Benchmark datasets

Each data set is in the format of benchmarks.proto:

1. name is the benchmark dataset's name.
2. message_name is the benchmark's message type full name (including package and message name)
3. payload is the list of raw data.

The schema for the datasets is described in `benchmarks.proto`.

Benchmark likely want to run several benchmarks against each data set (parse,
serialize, possibly JSON, possibly using different APIs, etc).

We would like to add more data sets.  In general we will favor data sets
that make the overall suite diverse without being too large or having
too many similar tests.  Ideally everyone can run through the entire
suite without the test run getting too long.
