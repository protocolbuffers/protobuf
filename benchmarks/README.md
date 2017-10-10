
# Protocol Buffers Benchmarks

This directory contains benchmarking schemas and data sets that you
can use to test a variety of performance scenarios against your
protobuf language runtime.

The schema for the datasets is described in `benchmarks.proto`.

Generate the data sets like so:

```
$ make
$ ./generate-datasets
Wrote dataset: dataset.google_message1_proto3.pb
Wrote dataset: dataset.google_message1_proto2.pb
Wrote dataset: dataset.google_message2.pb
$
```

Each data set will be written to its own file.  Benchmarks will
likely want to run several benchmarks against each data set (parse,
serialize, possibly JSON, possibly using different APIs, etc).

We would like to add more data sets.  In general we will favor data sets
that make the overall suite diverse without being too large or having
too many similar tests.  Ideally everyone can run through the entire
suite without the test run getting too long.
