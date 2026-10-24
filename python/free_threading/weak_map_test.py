# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC. All rights reserved.
# Use of this source code is governed by a BSD-style license in LICENSE.
"""Subprocess-bounded native lock reentrancy probes; accepts helper .so path."""
import importlib.util
import subprocess
import sys

def child(path, scenario):
  spec = importlib.util.spec_from_file_location('_weak_map_test', path)
  native = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(native)
  assert not sys._is_gil_enabled()
  cache = native.new()
  value = []
  native.add(cache, 1, value)
  if scenario == 'iterator':
    assert native.iterate(cache, lambda: native.count(cache)) == 1
    assert native.count(cache) == 0
  elif scenario == 'replacement':
    replacement = []
    def replace():
      assert native.erase(cache, 1, value)
      native.add(cache, 1, replacement)
      return 42
    assert native.iterate(cache, replace) == 42
    assert native.count(cache) == 1
    assert native.erase(cache, 1, replacement)
  elif scenario == 'retirement':
    assert native.erase(cache, 1, value)
    events = []
    class Value:
      def __del__(self):
        events.append(native.count(cache))
    holder = [Value()]
    native.add(cache, 1, holder[0])
    def retire():
      holder.clear()
      assert events == []
      return 42
    assert native.iterate(cache, retire) == 42
    assert events == [0], events
    assert native.iterate(cache, lambda: 1 / 0) is None
  elif scenario in ('exception', 'snapshot_failure'):
    events = []
    class ReenterError(Exception):
      def __del__(self):
        events.append(native.count(cache))
    try:
      if scenario == 'exception':
        native.fail_add(cache, value, ReenterError)
      else:
        native.fail_begin(cache, ReenterError)
    except MemoryError:
      pass
    else:
      raise AssertionError('Expected injected MemoryError')
    assert len(events) == 1, events
    # The map remains usable after failure and finalizer reentry.
    native.add(cache, 2, value)
    assert native.count(cache) >= 1
  print(scenario + ' passed', flush=True)

if __name__ == '__main__':
  if len(sys.argv) == 4 and sys.argv[1] == '--child':
    child(sys.argv[2], sys.argv[3])
  else:
    for scenario in ('iterator', 'exception', 'replacement', 'retirement',
                     'snapshot_failure'):
      subprocess.run([sys.executable, '-S', __file__, '--child',
                      sys.argv[1], scenario], check=True, timeout=30)
