
# Protocol Buffers Benchmarks

This directory contains benchmarking schemas and data sets that you
can use to test a variety of performance scenarios against your
protobuf language runtime.

The schema for the datasets is described in `benchmarks.proto`.

The benchmark is based on some submodules. To initialize the submodues:

For java:
```
$ ./initialize_submodule.sh java
```

For java:
```
$ ./initialize_submodule.sh cpp
```

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

There's some big testing data not included in the directory initially, you need to 
run the following command to download the testing data:

```
$ ./download_data.sh 
```

Each data set is in the format of benchmarks.proto:
1. name is the benchmark dataset's name.
2. message_name is the benchmark's message type full name (including package and message name)
3. payload is the list of raw data.

Benchmark likely want to run several benchmarks against each data set (parse,
serialize, possibly JSON, possibly using different APIs, etc).

We would like to add more data sets.  In general we will favor data sets
that make the overall suite diverse without being too large or having
too many similar tests.  Ideally everyone can run through the entire
suite without the test run getting too long.
