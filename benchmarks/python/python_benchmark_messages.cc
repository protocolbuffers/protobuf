#include <Python.h>

#include "benchmarks.pb.h"
#include "datasets/google_message1/proto2/benchmark_message1_proto2.pb.h"
#include "datasets/google_message1/proto3/benchmark_message1_proto3.pb.h"
#include "datasets/google_message2/benchmark_message2.pb.h"
#include "datasets/google_message3/benchmark_message3.pb.h"
#include "datasets/google_message4/benchmark_message4.pb.h"

static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                     "libbenchmark_messages",
                                     "Benchmark messages Python module",
                                     -1,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL};

extern "C" {
PyMODINIT_FUNC
PyInit_libbenchmark_messages() {
  benchmarks::BenchmarkDataset().descriptor();
  benchmarks::proto3::GoogleMessage1().descriptor();
  benchmarks::proto2::GoogleMessage1().descriptor();
  benchmarks::proto2::GoogleMessage2().descriptor();
  benchmarks::google_message3::GoogleMessage3().descriptor();
  benchmarks::google_message4::GoogleMessage4().descriptor();

  return PyModule_Create(&_module);
}
}
