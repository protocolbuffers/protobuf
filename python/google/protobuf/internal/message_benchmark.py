"""Benchmarks for assigning large NumPy data to protobuf fields."""

from collections.abc import Callable
import functools
import sys

import google_benchmark
import numpy as np

from google.protobuf import unittest_pb2


@functools.cache
def make_array(num_bytes: int):
  return np.zeros(shape=(num_bytes,), dtype=np.uint8)


def benchmark(
    func: Callable[[google_benchmark.State], None],
) -> Callable[[google_benchmark.State], None]:
  """Decorates a function for benchmarking."""

  @google_benchmark.register
  @google_benchmark.option.unit(google_benchmark.kMillisecond)
  @google_benchmark.option.arg_names(['num_bytes'])
  @google_benchmark.option.arg(1024 * 1024 * 100)
  @functools.wraps(func)
  def wrapper(state: google_benchmark.State) -> None:
    func(state)
    state.bytes_processed = state.iterations * state.range(0)

  return wrapper


@benchmark
def bench_extend_int32(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  msg = unittest_pb2.TestAllTypes()
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_int32.extend(arr)


@benchmark
def bench_extend_int64(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int64)
  msg = unittest_pb2.TestAllTypes()
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_int64.extend(arr)


@benchmark
def bench_extend_float(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.float32)
  msg = unittest_pb2.TestAllTypes()
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_float.extend(arr)


@benchmark
def bench_extend_double(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.float64)
  msg = unittest_pb2.TestAllTypes()
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_double.extend(arr)


@benchmark
def bench_assign_bytes(state: google_benchmark.State):
  arr = make_array(state.range(0))
  msg = unittest_pb2.TestAllTypes()
  arr_bytes = arr.tobytes()
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.optional_bytes = arr_bytes


@benchmark
def bench_assign_bytes_with_conversion(state: google_benchmark.State):
  arr = make_array(state.range(0))
  msg = unittest_pb2.TestAllTypes()
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.optional_bytes = arr.tobytes()


if __name__ == '__main__':
  if __name__ == '__main__':
    if any(arg.startswith('--benchmark_filter') for arg in sys.argv):
      google_benchmark.main()
    else:
      print('No benchmark filter specified. Skipping benchmarks.')
