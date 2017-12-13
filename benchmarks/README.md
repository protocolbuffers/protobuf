
# Protocol Buffers Benchmarks

This directory contains benchmarking schemas and data sets that you
can use to test a variety of performance scenarios against your
protobuf language runtime.

## Benchmark tools and build instructions

First, you need to follow the instruction in the root directory's README to 
build your language's protobuf, then:

### CPP
We are using [google/benchmark](https://github.com/google/benchmark) as the 
benchmark tool for testing cpp. This is included as submodule under third_party
directory. To init and build this tools, you need to do this under root dirctory:

```
$ cd third_party
$ git submodule update --init -r
$ cd benchmark && cmake -DCMAKE_BUILD_TYPE=Release && make && cd ../..
```

### JAVA
We're using maven to build the java benchmarks, which is the same as to build 
the Java protobuf. There're no other tools need to install. We're using 
[google/caliper](https://github.com/google/caliper) as benchmark tool, which 
can be automaticly included by maven.
  

## Run instructions

To run all the benchmark dataset:

For java:

```
$ make java
```

For cpp:

```
$ make cpp
```

To run a specific dataset:

For java:

```
$ make java-benchmark
$ ./java-benchmark $(specific generated dataset file name)
```

For cpp:

```
$ make cpp-benchmark
$ ./cpp-benchmark $(specific generated dataset file name)
```

## Benchmark datasets

There's some big testing data not included in the directory initially, you need to 
run the following command to download the testing data:

```
$ ./download_data.sh 
```

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
