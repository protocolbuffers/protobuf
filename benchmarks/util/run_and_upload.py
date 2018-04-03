import argparse
import os
import re
import copy
import uuid
import calendar
import time
import big_query_utils
import datetime


_PROJECT_ID = 'grpc-testing'
_DATASET = 'protobuf_benchmark_result'
_TABLE = 'opensource_result_v1'
_NOW = "%d%02d%02d"%(datetime.datetime.now().year,
                     datetime.datetime.now().month,
                     datetime.datetime.now().day)


average_bytes_per_message = {
  "google_message2": 84570.0,
  "google_message1_proto2": 228.0,
  "google_message1_proto3": 228.0,
  "google_message4": 18687.277215502345,
  "google_message3_4": 16161.58529111338,
  "google_message3_3": 75.3473942530787,
  "google_message3_5": 20.979602347823803,
  "google_message3_2": 366554.9166666667,
  "google_message3_1": 200567668
}

total_bytes = {
  "google_message2": 84570.0,
  "google_message1_proto2": 228.0,
  "google_message1_proto3": 228.0,
  "google_message4": 75702160.0,
  "google_message3_4": 31644384.0,
  "google_message3_3": 8443429,
  "google_message3_5": 45540234,
  "google_message3_2": 105567816,
  "google_message3_1": 200567668
}

cpp_result = []
python_result = []
java_result = []
go_result = []


def parse_cpp_result(filename):
  global cpp_result
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename) as f:
    for line in f:
      result_list = re.split("[\ \t]+", line)
      benchmark_name_list = re.findall("google_message[0-9]+.*$",
                                       result_list[0])
      if len(benchmark_name_list) == 0:
        continue
      filename = re.split("(_parse_|_serialize)", benchmark_name_list[0])[0]
      behavior = benchmark_name_list[0][len(filename) + 1:]
      throughput_with_unit = re.split('/s', result_list[-1])[0]
      if throughput_with_unit[-2:] == "GB":
        throughput = float(throughput_with_unit[:-2]) * 1024
      else:
        throughput = float(throughput_with_unit[:-2])
      cpp_result.append({
        "dataFilename": filename,
        "throughput": throughput,
        "behavior": behavior,
        "language": "cpp"
      })


def parse_python_result(filename):
  global average_bytes_per_message, python_result
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename) as f:
    result = {"language": "python"}
    for line in f:
      if line.find("./python") != -1:
        result["behavior"] = re.split("[ \t\n]+", line)[0][9:]
      elif line.find("dataset file") != -1:
        result["dataFileName"] = re.split(
            "\.",
            re.findall("google_message[0-9]+.*$", line)[0])[-2]
      elif line.find("Average time for ") != -1:
        elements = re.split("[ \t]+", line)
        new_result = copy.deepcopy(result)
        new_result["behavior"] += '_' + elements[3][:-1]
        new_result["throughput"] = \
          average_bytes_per_message[new_result["dataFileName"]] / \
          float(elements[4]) * 1e9 / 1024 / 1024
        python_result.append(new_result)


def parse_java_result(filename):
  global average_bytes_per_message, java_result
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename) as f:
    result = {"language": "java"}
    for line in f:
      if line.find("dataFile=") != -1:
        result["dataFileName"] = re.split(
            "\.",
            re.findall("google_message[0-9]+.*$", line)[0])[-2]
      if line.find("benchmarkMethod=") != -1:
        for element in re.split("[ \t,]+", line):
          if element[:16] == "benchmarkMethod=":
            result["behavior"] = element[16:]
      if line.find("median=") != -1:
        for element in re.split("[ \t,]+", line):
          if element[:7] == "median=":
            result["throughput"] = \
              average_bytes_per_message[result["dataFileName"]] / \
              float(element[7:]) * 1e9 / 1024 / 1024
            java_result.append(copy.deepcopy(result))
            continue


def parse_go_result(filename):
  global average_bytes_per_message, go_result
  if filename == "":
    return
  if filename[0] != '/':
    filename = os.path.dirname(os.path.abspath(__file__)) + '/' + filename
  with open(filename) as f:
    for line in f:
      result_list = re.split("[\ \t]+", line)
      benchmark_name_list = re.split("[/\.]+", result_list[0])
      filename = ""
      for s in benchmark_name_list:
        if s[:14] == "google_message":
          filename = s
      if filename == "":
        continue
      behavior = benchmark_name_list[-1]
      throughput = \
        total_bytes[filename] / \
        float(result_list[-2]) * 1e9 / 1024 / 1024
      go_result.append({
        "dataFilename": filename,
        "throughput": throughput,
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
