from __future__ import print_function
from __future__ import absolute_import
import argparse
import os
import re
import copy
import uuid
import calendar
import time
import datetime

from util import big_query_utils
from util import result_parser

_PROJECT_ID = 'grpc-testing'
_DATASET = 'protobuf_benchmark_result'
_TABLE = 'opensource_result_v2'
_NOW = "%d%02d%02d" % (datetime.datetime.now().year,
                       datetime.datetime.now().month,
                       datetime.datetime.now().day)

_INITIAL_TIME = calendar.timegm(time.gmtime())

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
    new_result = {}
    new_result["metric"] = "throughput"
    new_result["value"] = result["throughput"]
    new_result["unit"] = "MB/s"
    new_result["test"] = "protobuf_benchmark"
    new_result["product_name"] = "protobuf"
    labels_string = ""
    for key in result:
      labels_string += ",|%s:%s|" % (key, result[key])
    new_result["labels"] = labels_string[1:]
    new_result["timestamp"] = _INITIAL_TIME
    print(labels_string)

    bq = big_query_utils.create_big_query()
    row = big_query_utils.make_row(str(uuid.uuid4()), new_result)
    if not big_query_utils.insert_rows(bq, _PROJECT_ID, _DATASET,
                                        _TABLE + "$" + _NOW,
                                        [row]):
      print('Error when uploading result', new_result)


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
  parser.add_argument("-node", "--node_input_file",
                      help="The node.js benchmark result file's name",
                      default="")
  parser.add_argument("-php", "--php_input_file",
                      help="The pure php benchmark result file's name",
                      default="")
  parser.add_argument("-php_c", "--php_c_input_file",
                      help="The php with c ext benchmark result file's name",
                      default="")
  args = parser.parse_args()

  metadata = get_metadata()
  print("uploading results...")
  upload_result(result_parser.get_result_from_file(
      cpp_file=args.cpp_input_file,
      java_file=args.java_input_file,
      python_file=args.python_input_file,
      go_file=args.go_input_file,
      node_file=args.node_input_file,
      php_file=args.php_input_file,
      php_c_file=args.php_c_input_file,
  ), metadata)
