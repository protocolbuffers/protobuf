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
  @google_benchmark.option.arg(1024 * 1024 * 20)
  @google_benchmark.option.arg(1024 * 1024 * 100)
  @functools.wraps(func)
  def wrapper(state: google_benchmark.State) -> None:
    func(state)
    state.bytes_processed = state.iterations * state.range(0)

  return wrapper


@benchmark
def bench_build_message_via_slice(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  while state:
    msg = unittest_pb2.TestAllTypes()
    msg.repeated_int32[:] = arr


@benchmark
def bench_build_message(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  while state:
    _ = unittest_pb2.TestAllTypes(repeated_int32=arr)


@benchmark
def bench_build_message_nested_via_slice(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  while state:
    msg = unittest_pb2.NestedTestAllTypes()
    msg.payload.repeated_int32[:] = arr


@benchmark
def bench_build_nested_message_dict(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  while state:
    _ = unittest_pb2.NestedTestAllTypes(payload=dict(repeated_int32=arr))


@benchmark
def bench_build_nested_message_int32(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  while state:
    _ = unittest_pb2.NestedTestAllTypes(
        payload=unittest_pb2.TestAllTypes(repeated_int32=arr)
    )


@benchmark
def bench_build_nested_message_cord(state: google_benchmark.State):
  chunk_size = state.range(0) // 1000
  strings = ['a' * chunk_size] * 1000
  while state:
    _ = unittest_pb2.NestedTestAllTypes(
        payload=unittest_pb2.TestAllTypes(repeated_cord=strings)
    )


@benchmark
def bench_build_nested_message_string_piece(state: google_benchmark.State):
  chunk_size = state.range(0) // 1000
  strings = ['a' * chunk_size] * 1000
  while state:
    _ = unittest_pb2.NestedTestAllTypes(
        payload=unittest_pb2.TestAllTypes(repeated_string_piece=strings)
    )


@benchmark
def bench_build_nested_message_nested_message(state: google_benchmark.State):
  subs = [unittest_pb2.TestAllTypes.NestedMessage(bb=123)] * (
      state.range(0) // 8
  )
  while state:
    _ = unittest_pb2.NestedTestAllTypes(
        payload=unittest_pb2.TestAllTypes(repeated_nested_message=subs)
    )


@benchmark
def bench_assign_repeated_float(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.float32)
  msg = unittest_pb2.TestAllTypes()
  msg_source = unittest_pb2.TestAllTypes()
  msg_source.repeated_float.extend(arr)
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_float[:] = msg_source.repeated_float


@benchmark
def bench_assign_repeated_int64_to_int32(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int64)
  msg = unittest_pb2.TestAllTypes()
  msg_source = unittest_pb2.TestAllTypes()
  msg_source.repeated_int64.extend(arr)
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_int32[:] = msg_source.repeated_int64


@benchmark
def bench_assign_repeated_double_to_float(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.float64)
  msg = unittest_pb2.TestAllTypes()
  msg_source = unittest_pb2.TestAllTypes()
  msg_source.repeated_double.extend(arr)
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_float[:] = msg_source.repeated_double


@benchmark
def bench_assign_numpy_int64_to_int32(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int64)
  msg = unittest_pb2.TestAllTypes()
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_int32[:] = arr


@benchmark
def bench_assign_numpy_double_to_float(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.float64)
  msg = unittest_pb2.TestAllTypes()
  while state:
    state.pause_timing()
    msg.Clear()
    state.resume_timing()
    msg.repeated_float[:] = arr


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
def bench_decode_from_bytes(state: google_benchmark.State):
  arr = make_array(state.range(0))
  msg = unittest_pb2.TestAllTypes()
  msg.optional_bytes = memoryview(arr)
  msg_bytes = msg.SerializeToString()
  while state:
    _ = unittest_pb2.TestAllTypes.FromString(msg_bytes)


@benchmark
def bench_decode_from_memoryview(state: google_benchmark.State):
  arr = make_array(state.range(0))
  msg = unittest_pb2.TestAllTypes()
  msg.optional_bytes = memoryview(arr)
  msg_bytes = memoryview(msg.SerializeToString())
  while state:
    _ = unittest_pb2.TestAllTypes.FromString(msg_bytes)


@benchmark
def bench_encode_into_bytes(state: google_benchmark.State):
  arr = make_array(state.range(0))
  msg = unittest_pb2.TestAllTypes()
  msg.optional_bytes = memoryview(arr)
  while state:
    _ = msg.SerializeToString()


@benchmark
def bench_assign_int32(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  msg = unittest_pb2.TestAllTypes()
  while state:
    msg.repeated_int32[:] = arr
    state.pause_timing()
    msg.Clear()
    state.resume_timing()


@benchmark
def bench_assign_extend_int32(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  msg = unittest_pb2.TestAllTypes()
  msg.repeated_int32[:] = arr
  while state:
    msg.repeated_int32[len(arr) :] = arr
    state.pause_timing()
    msg.repeated_int32[:] = arr
    state.resume_timing()


@benchmark
def bench_assign_slice_int32(state: google_benchmark.State):
  arr = make_array(state.range(0)).view(dtype=np.int32)
  half_arr = arr[: len(arr) // 2]
  msg = unittest_pb2.TestAllTypes()
  msg.repeated_int32.extend(arr)
  while state:
    msg.repeated_int32[::2] = half_arr


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
    msg.optional_bytes = memoryview(arr)


if __name__ == '__main__':
  if any(arg.startswith('--benchmark_filter') for arg in sys.argv):
    google_benchmark.main()
  else:
    print('No benchmark filter specified. Skipping benchmarks.')
