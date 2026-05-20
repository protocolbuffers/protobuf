"""Benchmarks for assigning large NumPy data to protobuf fields."""

from collections.abc import Callable
import functools

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
  @google_benchmark.option.arg(100 * 1024 * 1024)
  @functools.wraps(func)
  def wrapper(state: google_benchmark.State) -> None:
    func(state)

  return wrapper


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


@benchmark
def bench_assign_bytes_with_memoryview(state: google_benchmark.State):
  """Benchmarks assigning a NumPy array to a protobuf field using memoryview."""
  arr = make_array(state.range(0))
  msg = unittest_pb2.TestAllTypes()

  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.optional_bytes = memoryview(arr)


if __name__ == '__main__':
  google_benchmark.main()
