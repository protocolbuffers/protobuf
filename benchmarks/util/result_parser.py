# This import depends on the automake rule protoc_middleman, please make sure
# protoc_middleman has been built before run this file.
import json
import re
import os.path
# BEGIN OPENSOURCE
import sys
sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir))
# END OPENSOURCE
import tmp.benchmarks_pb2 as benchmarks_pb2

__file_size_map = {}

def __get_data_size(filename):
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + "/../" + filename
  if filename in __file_size_map:
    return __file_size_map[filename]
  benchmark_dataset = benchmarks_pb2.BenchmarkDataset()
  benchmark_dataset.ParseFromString(
      open(filename, "rb").read())
  size = 0
  count = 0
  for payload in benchmark_dataset.payload:
    size += len(payload)
    count += 1
  __file_size_map[filename] = (size, 1.0 * size / count)
  return size, 1.0 * size / count


def __extract_file_name(file_name):
  name_list = re.split(r"[/\.]", file_name)
  short_file_name = ""
  for name in name_list:
    if name[:14] == "google_message":
      short_file_name = name
  return short_file_name


__results = []


# CPP results example:
# [
#   "benchmarks": [
#     {
#       "bytes_per_second": int,
#       "cpu_time_ns": double,
#       "iterations": int,
#       "name: string,
#       "real_time_ns: double,
#       ...
#     },
#     ...
#   ],
#   ...
# ]
def __parse_cpp_result(filename):
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename, "rb") as f:
    results = json.loads(f.read())
    for benchmark in results["benchmarks"]:
      data_filename = "".join(
          re.split("(_parse_|_serialize)", benchmark["name"])[0])
      behavior = benchmark["name"][len(data_filename) + 1:]
      if data_filename[:2] == "BM":
        data_filename = data_filename[3:]
      __results.append({
        "language": "cpp",
        "dataFilename": data_filename,
        "behavior": behavior,
        "throughput": benchmark["bytes_per_second"] / 2.0 ** 20
      })


# Synthetic benchmark results example:
# [
#   "benchmarks": [
#     {
#       "cpu_time_ns": double,
#       "iterations": int,
#       "name: string,
#       "real_time_ns: double,
#       ...
#     },
#     ...
#   ],
#   ...
# ]
def __parse_synthetic_result(filename):
  if filename == "":
    return
  if filename[0] != "/":
    filename = os.path.dirname(os.path.abspath(__file__)) + "/" + filename
  with open(filename, "rb") as f:
    results = json.loads(f.read())
    for benchmark in results["benchmarks"]:
      __results.append({
          "language": "cpp",
          "dataFilename": "",
          "behavior": "synthetic",
          "throughput": 10.0**9 / benchmark["cpu_time_ns"]
      })


# Python results example:
# [
#   [
#     {
#       "filename": string,
#       "benchmarks": {
#         behavior: results,
#         ...
#       },
#     },
#     ...
#   ], #pure-python
#   ...
# ]
def __parse_python_result(filename):
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename, "rb") as f:
    results_list = json.loads(f.read())
    for results in results_list:
      for result in results:
        _, avg_size = __get_data_size(result["filename"])
        for behavior in result["benchmarks"]:
          __results.append({
            "language": "python",
            "dataFilename": __extract_file_name(result["filename"]),
            "behavior": behavior,
            "throughput": result["benchmarks"][behavior]
          })


# Java results example:
# [
#   {
#     "id": string,
#     "instrumentSpec": {...},
#     "measurements": [
#       {
#         "weight": float,
#         "value": {
#           "magnitude": float,
#           "unit": string
#         },
#         ...
#       },
#       ...
#     ],
#     "run": {...},
#     "scenario": {
#       "benchmarkSpec": {
#         "methodName": string,
#         "parameters": {
#            defined parameters in the benchmark: parameters value
#         },
#         ...
#       },
#       ...
#     }
#
#   },
#   ...
# ]
def __parse_java_result(filename):
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename, "rb") as f:
    results = json.loads(f.read())
    for result in results:
      total_weight = 0
      total_value = 0
      for measurement in result["measurements"]:
        total_weight += measurement["weight"]
        total_value += measurement["value"]["magnitude"]
      avg_time = total_value * 1.0 / total_weight
      total_size, _ = __get_data_size(
          result["scenario"]["benchmarkSpec"]["parameters"]["dataFile"])
      __results.append({
        "language": "java",
        "throughput": total_size / avg_time * 1e9 / 2 ** 20,
        "behavior": result["scenario"]["benchmarkSpec"]["methodName"],
        "dataFilename": __extract_file_name(
            result["scenario"]["benchmarkSpec"]["parameters"]["dataFile"])
      })


# Go benchmark results:
#
# goos: linux
# goarch: amd64
# Benchmark/.././datasets/google_message2/dataset.google_message2.pb/Unmarshal-12               3000      705784 ns/op
# Benchmark/.././datasets/google_message2/dataset.google_message2.pb/Marshal-12                 2000      634648 ns/op
# Benchmark/.././datasets/google_message2/dataset.google_message2.pb/Size-12                    5000      244174 ns/op
# Benchmark/.././datasets/google_message2/dataset.google_message2.pb/Clone-12                    300     4120954 ns/op
# Benchmark/.././datasets/google_message2/dataset.google_message2.pb/Merge-12                    300     4108632 ns/op
# PASS
# ok    _/usr/local/google/home/yilunchong/mygit/protobuf/benchmarks  124.173s
def __parse_go_result(filename):
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename, "rb") as f:
    for line in f:
      result_list = re.split(r"[\ \t]+", line)
      if result_list[0][:9] != "Benchmark":
        continue
      first_slash_index = result_list[0].find('/')
      last_slash_index = result_list[0].rfind('/')
      full_filename = result_list[0][first_slash_index+1:last_slash_index]
      total_bytes, _ = __get_data_size(full_filename)
      behavior_with_suffix = result_list[0][last_slash_index+1:]
      last_dash = behavior_with_suffix.rfind("-")
      if last_dash == -1:
        behavior = behavior_with_suffix
      else:
        behavior = behavior_with_suffix[:last_dash]
      __results.append({
        "dataFilename": __extract_file_name(full_filename),
        "throughput": total_bytes / float(result_list[2]) * 1e9 / 2 ** 20,
        "behavior": behavior,
        "language": "go"
      })


# Self built json results example:
#
# [
#   {
#     "filename": string,
#     "benchmarks": {
#       behavior: results,
#       ...
#     },
#   },
#   ...
# ]
def __parse_custom_result(filename, language):
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename, "rb") as f:
    results = json.loads(f.read())
    for result in results:
      _, avg_size = __get_data_size(result["filename"])
      for behavior in result["benchmarks"]:
        __results.append({
          "language": language,
          "dataFilename": __extract_file_name(result["filename"]),
          "behavior": behavior,
          "throughput": result["benchmarks"][behavior]
        })


def __parse_js_result(filename, language):
  return __parse_custom_result(filename, language)

def __parse_php_result(filename, language):
  return __parse_custom_result(filename, language)


def get_result_from_file(cpp_file="",
                         java_file="",
                         python_file="",
                         go_file="",
                         synthetic_file="",
                         node_file="",
                         php_c_file="",
                         php_file=""):
  results = {}
  if cpp_file != "":
    __parse_cpp_result(cpp_file)
  if java_file != "":
    __parse_java_result(java_file)
  if python_file != "":
    __parse_python_result(python_file)
  if go_file != "":
    __parse_go_result(go_file)
  if synthetic_file != "":
    __parse_synthetic_result(synthetic_file)
  if node_file != "":
    __parse_js_result(node_file, "node")
  if php_file != "":
    __parse_php_result(php_file, "php")
  if php_c_file != "":
    __parse_php_result(php_c_file, "php")

  return __results
