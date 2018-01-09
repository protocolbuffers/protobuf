
# Protocol Buffers Benchmarks

This directory contains benchmarking schemas and data sets that you
can use to test a variety of performance scenarios against your
protobuf language runtime.

## Prerequisite

First, you need to follow the instruction in the root directory's README to
build your language's protobuf, then:

### CPP
You need to install [cmake](https://cmake.org/) before building the benchmark.

We are using [google/benchmark](https://github.com/google/benchmark) as the
benchmark tool for testing cpp. This will be automaticly made during build the
cpp benchmark.

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

To run a specific dataset:

### Java:

```
$ make java-benchmark
$ ./java-benchmark $(specific generated dataset file name) [-- $(caliper option)]
```

### CPP:

```
$ make cpp-benchmark
$ ./cpp-benchmark $(specific generated dataset file name)
```

### Python:

#### Pure Python:

```
$ make python-pure-python-benchmark
$ ./python-pure-python-benchmark $(specific generated dataset file name)
```

#### CPP reflection:

```
$ make python-cpp-reflection-benchmark
$ ./python-cpp-reflection-benchmark $(specific generated dataset file name)
```

#### CPP generated code:

```
$ make python-cpp-generated-code-benchmark
$ ./python-cpp-generated-code-benchmark $(specific generated dataset file name)
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
