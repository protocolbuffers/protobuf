import sys
import os
import timeit
import math
import argparse
import fnmatch
import json

parser = argparse.ArgumentParser(description="Python protobuf benchmark")
parser.add_argument("data_files", metavar="dataFile", nargs="+", 
                    help="testing data files.")
parser.add_argument("--json", action="store_const", dest="json",
                    const="yes", default="no",
                    help="Whether to output json results")
parser.add_argument("--behavior_prefix", dest="behavior_prefix",
                    help="The output json format's behavior's name's prefix",
                    default="")
# BEGIN CPP GENERATED MESSAGE
parser.add_argument("--cpp_generated", action="store_const",
                    dest="cpp_generated", const="yes", default="no",
                    help="Whether to link generated code library")
# END CPP GENERATED MESSAGE
args = parser.parse_args()
# BEGIN CPP GENERATED MESSAGE
# CPP generated code must be linked before importing the generated Python code
# for the descriptor can be found in the pool
if args.cpp_generated != "no":
  sys.path.append( os.path.dirname( os.path.dirname( os.path.abspath(__file__) ) ) + "/.libs" )
  import libbenchmark_messages
  sys.path.append( os.path.dirname( os.path.dirname( os.path.abspath(__file__) ) ) + "/tmp" )
# END CPP GENERATED MESSAGE


import datasets.google_message1.proto2.benchmark_message1_proto2_pb2 as benchmark_message1_proto2_pb2
import datasets.google_message1.proto3.benchmark_message1_proto3_pb2 as benchmark_message1_proto3_pb2
import datasets.google_message2.benchmark_message2_pb2 as benchmark_message2_pb2
import datasets.google_message3.benchmark_message3_pb2 as benchmark_message3_pb2
import datasets.google_message4.benchmark_message4_pb2 as benchmark_message4_pb2
import benchmarks_pb2 as benchmarks_pb2


def run_one_test(filename):
  data = open(filename).read()
  benchmark_dataset = benchmarks_pb2.BenchmarkDataset()
  benchmark_dataset.ParseFromString(data)
  benchmark_util = Benchmark(full_iteration=len(benchmark_dataset.payload),
                             module="py_benchmark",
                             setup_method="init")
  result={}
  result["filename"] =  filename
  result["message_name"] =  benchmark_dataset.message_name
  result["benchmarks"] = {}
  benchmark_util.set_test_method("parse_from_benchmark")
  result["benchmarks"][args.behavior_prefix + "_parse_from_benchmark"] = \
    benchmark_util.run_benchmark(setup_method_args='"%s"' % (filename))
  benchmark_util.set_test_method("serialize_to_benchmark")
  result["benchmarks"][args.behavior_prefix + "_serialize_to_benchmark"] = \
    benchmark_util.run_benchmark(setup_method_args='"%s"' % (filename))
  return result


def init(filename):
  global benchmark_dataset, message_class, message_list, counter
  message_list=[]
  counter = 0
  data = open(os.path.dirname(sys.argv[0]) + "/../" + filename).read()
  benchmark_dataset = benchmarks_pb2.BenchmarkDataset()
  benchmark_dataset.ParseFromString(data)

  if benchmark_dataset.message_name == "benchmarks.proto3.GoogleMessage1":
    message_class = benchmark_message1_proto3_pb2.GoogleMessage1
  elif benchmark_dataset.message_name == "benchmarks.proto2.GoogleMessage1":
    message_class = benchmark_message1_proto2_pb2.GoogleMessage1
  elif benchmark_dataset.message_name == "benchmarks.proto2.GoogleMessage2":
    message_class = benchmark_message2_pb2.GoogleMessage2
  elif benchmark_dataset.message_name == "benchmarks.google_message3.GoogleMessage3":
    message_class = benchmark_message3_pb2.GoogleMessage3
  elif benchmark_dataset.message_name == "benchmarks.google_message4.GoogleMessage4":
    message_class = benchmark_message4_pb2.GoogleMessage4
  else:
    raise IOError("Message %s not found!" % (benchmark_dataset.message_name))

  for one_payload in benchmark_dataset.payload:
    temp = message_class()
    temp.ParseFromString(one_payload)
    message_list.append(temp)


def parse_from_benchmark():
  global counter, message_class, benchmark_dataset
  m = message_class().ParseFromString(benchmark_dataset.payload[counter % len(benchmark_dataset.payload)])
  counter = counter + 1


def serialize_to_benchmark():
  global counter, message_list, message_class
  s = message_list[counter % len(benchmark_dataset.payload)].SerializeToString()
  counter = counter + 1


class Benchmark:
  def __init__(self, module=None, test_method=None,
               setup_method=None, full_iteration = 1):
    self.full_iteration = full_iteration
    self.module = module
    self.test_method = test_method
    self.setup_method = setup_method

  def set_test_method(self, test_method):
    self.test_method = test_method

  def full_setup_code(self, setup_method_args=''):
    setup_code = ""
    setup_code += "from %s import %s\n" % (self.module, self.test_method)
    setup_code += "from %s import %s\n" % (self.module, self.setup_method)
    setup_code += "%s(%s)\n" % (self.setup_method, setup_method_args)
    return setup_code

  def dry_run(self, test_method_args='', setup_method_args=''):
    return timeit.timeit(stmt="%s(%s)" % (self.test_method, test_method_args),
                         setup=self.full_setup_code(setup_method_args),
                         number=self.full_iteration);

  def run_benchmark(self, test_method_args='', setup_method_args=''):
    reps = self.full_iteration;
    t = self.dry_run(test_method_args, setup_method_args);
    if t < 3 :
      reps = int(math.ceil(3 / t)) * self.full_iteration
    t = timeit.timeit(stmt="%s(%s)" % (self.test_method, test_method_args),
                      setup=self.full_setup_code(setup_method_args),
                      number=reps);
    return 1.0 * t / reps * (10 ** 9)
  

if __name__ == "__main__":
  results = []
  for file in args.data_files:
    results.append(run_one_test(file))
  
  if args.json != "no":
    print json.dumps(results)
  else:
    for result in results:
      print "Message %s of dataset file %s" % \
          (result["message_name"], result["filename"])
      print "Average time for parse_from_benchmark: %.2f ns" % \
          (result["benchmarks"][ \
                      args.behavior_prefix + "_parse_from_benchmark"])
      print "Average time for serialize_to_benchmark: %.2f ns" % \
          (result["benchmarks"][ \
                      args.behavior_prefix + "_serialize_to_benchmark"])
      print ""
