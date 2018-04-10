import argparse
import os
import re
import copy
import uuid
import calendar
import time
import big_query_utils
import datetime
import json
# This import depends on the automake rule protoc_middleman, please make sure
# protoc_middleman has been built before run this file.
import os.path, sys
sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir))
import tmp.benchmarks_pb2 as benchmarks_pb2
from click.types import STRING

_PROJECT_ID = 'grpc-testing'
_DATASET = 'protobuf_benchmark_result'
_TABLE = 'opensource_result_v1'
_NOW = "%d%02d%02d" % (datetime.datetime.now().year,
                       datetime.datetime.now().month,
                       datetime.datetime.now().day)

file_size_map = {}

def get_data_size(file_name):
  if file_name in file_size_map:
    return file_size_map[file_name]
  benchmark_dataset = benchmarks_pb2.BenchmarkDataset()
  benchmark_dataset.ParseFromString(
      open(os.path.dirname(os.path.abspath(__file__)) + "/../" + file_name).read())
  size = 0
  count = 0
  for payload in benchmark_dataset.payload:
    size += len(payload)
    count += 1
  file_size_map[file_name] = (size, 1.0 * size / count)
  return size, 1.0 * size / count


def extract_file_name(file_name):
  name_list = re.split("[/\.]", file_name)
  short_file_name = ""
  for name in name_list:
    if name[:14] == "google_message":
      short_file_name = name
  return short_file_name


cpp_result = []
python_result = []
java_result = []
go_result = []


# CPP results example:
# [ 
#   "benchmarks": [ 
#     {
#       "bytes_per_second": int,
#       "cpu_time": int,
#       "name: string,
#       "time_unit: string,
#       ...
#     },
#     ... 
#   ],
#   ... 
# ]
def parse_cpp_result(filename):
  global cpp_result
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename) as f:
    results = json.loads(f.read())
    for benchmark in results["benchmarks"]:
      data_filename = "".join(
          re.split("(_parse_|_serialize)", benchmark["name"])[0])
      behavior = benchmark["name"][len(data_filename) + 1:]
      cpp_result.append({
        "language": "cpp",
        "dataFileName": data_filename,
        "behavior": behavior,
        "throughput": benchmark["bytes_per_second"] / 2.0 ** 20
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
#       "message_name": STRING
#     },
#     ... 
#   ], #pure-python
#   ... 
# ]
def parse_python_result(filename):
  global python_result
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename) as f:
    results_list = json.loads(f.read())
    for results in results_list:
      for result in results:
        _, avg_size = get_data_size(result["filename"])
        for behavior in result["benchmarks"]:
          python_result.append({
            "language": "python",
            "dataFileName": extract_file_name(result["filename"]),
            "behavior": behavior,
            "throughput": avg_size /
                          result["benchmarks"][behavior] * 1e9 / 2 ** 20
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
def parse_java_result(filename):
  global average_bytes_per_message, java_result
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename) as f:
    results = json.loads(f.read())
    for result in results:
      total_weight = 0
      total_value = 0
      for measurement in result["measurements"]:
        total_weight += measurement["weight"]
        total_value += measurement["value"]["magnitude"]
      avg_time = total_value * 1.0 / total_weight
      total_size, _ = get_data_size(
          result["scenario"]["benchmarkSpec"]["parameters"]["dataFile"])
      java_result.append({
        "language": "java",
        "throughput": total_size / avg_time * 1e9 / 2 ** 20,
        "behavior": result["scenario"]["benchmarkSpec"]["methodName"],
        "dataFileName": extract_file_name(
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
def parse_go_result(filename):
  global go_result
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename) as f:
    for line in f:
      result_list = re.split("[\ \t]+", line)
      if result_list[0][:9] != "Benchmark":
        continue
      first_slash_index = result_list[0].find('/')
      last_slash_index = result_list[0].rfind('/')
      full_filename = result_list[0][first_slash_index+4:last_slash_index] # delete ../ prefix
      total_bytes, _ = get_data_size(full_filename)
      behavior_with_suffix = result_list[0][last_slash_index+1:]
      last_dash = behavior_with_suffix.rfind("-")
      if last_dash == -1:
        behavior = behavior_with_suffix
      else:
        behavior = behavior_with_suffix[:last_dash]
      go_result.append({
        "dataFilename": extract_file_name(full_filename),
        "throughput": total_bytes / float(result_list[2]) * 1e9 / 2 ** 20,
        "behavior": behavior,
        "language": "go"
      })


def get_metadata():
  build_number = os.getenv('BUILD_NUMBER')
  build_url = os.getenv('BUILD_URL')
  job_name = os.getenv('JOB_NAME')
  git_commit = os.getenv('GIT_COMMIT')
  # actual commit is the actual head of PR that is getting tested
  git_actual_commit = os.getenv('ghprbActualCommit')

  utc_timestamp = str(calendar.timegm(time.gmtime()))
  metadata = {'created': utc_timestamp}

  if build_number:
    metadata['buildNumber'] = build_number
  if build_url:
    metadata['buildUrl'] = build_url
  if job_name:
    metadata['jobName'] = job_name
  if git_commit:
    metadata['gitCommit'] = git_commit
  if git_actual_commit:
    metadata['gitActualCommit'] = git_actual_commit

  return metadata


def upload_result(result_list, metadata):
  for result in result_list:
    new_result = copy.deepcopy(result)
    new_result['metadata'] = metadata
    bq = big_query_utils.create_big_query()
    row = big_query_utils.make_row(str(uuid.uuid4()), new_result)
    if not big_query_utils.insert_rows(bq, _PROJECT_ID, _DATASET,
                                       _TABLE + "$" + _NOW,
                                       [row]):
      print 'Error when uploading result', new_result


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("-cpp", "--cpp_input_file",
                      help="The CPP benchmark result file's name",
                      default="")
  parser.add_argument("-java", "--java_input_file",
                      help="The Java benchmark result file's name",
                      default="")
  parser.add_argument("-python", "--python_input_file",
                      help="The Python benchmark result file's name",
                      default="")
  parser.add_argument("-go", "--go_input_file",
                      help="The golang benchmark result file's name",
                      default="")
  args = parser.parse_args()

  parse_cpp_result(args.cpp_input_file)
  parse_python_result(args.python_input_file)
  parse_java_result(args.java_input_file)
  parse_go_result(args.go_input_file)

  metadata = get_metadata()
  print "uploading cpp results..."
  upload_result(cpp_result, metadata)
  print "uploading java results..."
  upload_result(java_result, metadata)
  print "uploading python results..."
  upload_result(python_result, metadata)
  print "uploading go results..."
  upload_result(go_result, metadata)
