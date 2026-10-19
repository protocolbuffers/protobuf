# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for thread safe"""

from concurrent import futures
import gc
import subprocess
import sys
import threading
import time
import timeit
import unittest

from google.protobuf import descriptor_database
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import message_factory
from google.protobuf.message import DecodeError
from google.protobuf.internal import api_implementation
from google.protobuf.internal import test_proto2_pb2
from google.protobuf.internal import testing_refleaks

from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_pb2

# Enable this to run the benchmarks.
ALSO_RUN_BENCHMARKS = False


@testing_refleaks.TestCase
class ThreadSafeTest(unittest.TestCase):

  def setUp(self):
    self.success = 0

  @unittest.skipUnless(api_implementation.Type() == 'upb', 'Native module GC')
  def testModuleTraversesOwnedReferences(self):
    from google._upb import _message

    maps = test_proto2_pb2.MessageWithRepeatedAndMap()
    unknown = _message.UnknownFieldSet(
        unittest_pb2.TestAllTypes.FromString(b'\x80\x3e\x01'))
    owned = gc.get_referents(_message)
    for reference in (
        type(maps.map_items), type(maps.scalar_map), type(unknown[0]),
        unittest_pb2.TestAllTypes._ListFieldsItemKey, DecodeError,
    ):
      self.assertTrue(any(item is reference for item in owned), reference)

  def testNativeAtexitFinalizer(self):
    script = '''
import atexit
import gc
import os
import sys
from google.protobuf import unittest_pb2
from google.protobuf.internal import api_implementation
assert api_implementation.Type() == sys.argv[1]
if sys.argv[2] == "nogil":
  assert not sys._is_gil_enabled()
def unraisable(error, exit_process=os._exit):
  exit_process(91)
sys.unraisablehook = unraisable
class LateFinalizer:
  def __init__(self):
    self.cycle = self
  def __del__(self, cls=unittest_pb2.TestAllTypes, write=os.write):
    value = cls.FromString(b"\\x08\\x01")
    assert value.optional_int32 == 1
    assert value.SerializeToString() == b"\\x08\\x01"
    write(1, b"native shutdown finalizer completed\\n")
late = LateFinalizer()
def collect_at_exit(namespace=globals(), collect=gc.collect):
  namespace.pop("late")
  collect()
atexit.register(collect_at_exit)
'''
    nogil = hasattr(sys, '_is_gil_enabled') and not sys._is_gil_enabled()
    result = subprocess.run(
        [sys.executable, '-S', '-c', script, api_implementation.Type(),
         'nogil' if nogil else 'gil'],
        capture_output=True, text=True, timeout=60, check=False,
    )
    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    self.assertEqual(result.stdout.count('native shutdown finalizer completed'), 1)

  def testConcurrentColdImports(self):
    # A fresh process is essential: this module has already imported the
    # generated types. Propagate every worker exception to the parent test.
    script = '''
from concurrent import futures
import importlib
import sys
import threading
barrier = threading.Barrier(8)
names = ("any", "duration", "empty", "field_mask", "struct", "timestamp",
         "type", "wrappers")
def load(index):
  barrier.wait(timeout=30)
  # Distinct module locks must not accidentally serialize registration into
  # the shared default descriptor pool.
  importlib.import_module("google.protobuf." + names[index] + "_pb2")
  modules = [importlib.import_module(name) for name in (
      "google.protobuf.descriptor_pb2", "google.protobuf.json_format",
      "google.protobuf.internal.test_proto2_pb2")]
  return modules[0].FileDescriptorProto(name="cold.proto").SerializeToString()
with futures.ThreadPoolExecutor(max_workers=8) as executor:
  results = list(executor.map(load, range(8)))
assert all(result == results[0] for result in results)
from google.protobuf.internal import api_implementation
assert api_implementation.Type() == sys.argv[1]
if sys.argv[2] == "nogil":
  assert not sys._is_gil_enabled()
'''
    nogil = hasattr(sys, '_is_gil_enabled') and not sys._is_gil_enabled()
    result = subprocess.run(
        [sys.executable, '-S', '-c', script, api_implementation.Type(),
         'nogil' if nogil else 'gil'],
        capture_output=True, text=True, timeout=90, check=False,
    )
    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

  def testConcurrentDescriptorPoolLifetimes(self):
    barrier = threading.Barrier(8)

    def ExercisePools(worker):
      barrier.wait(timeout=30)
      for iteration in range(32):
        pool = descriptor_pool.DescriptorPool()
        proto = descriptor_pb2.FileDescriptorProto(
            name=f'lifetime_{worker}_{iteration}.proto'
        )
        proto.message_type.add(name='LifetimeMessage').field.add(
            name='value', number=1, type=5, label=1
        )
        pool.Add(proto)
        desc = pool.FindMessageTypeByName('LifetimeMessage')
        cls = message_factory.GetMessageClass(desc)
        value = cls(value=iteration)
        # Keep descendants alive while releasing their originating references.
        field = desc.fields_by_name['value']
        del pool, desc, cls
        if iteration % 8 == 0:
          gc.collect()
        self.assertEqual(field.containing_type.name, 'LifetimeMessage')
        self.assertEqual(
            type(value).FromString(value.SerializeToString()), value
        )
      return True

    with futures.ThreadPoolExecutor(max_workers=8) as executor:
      self.assertTrue(all(executor.map(ExercisePools, range(8))))

  def testConcurrentDescriptorPoolRegistrationAndLookup(self):
    pool = descriptor_pool.DescriptorPool()
    seed = descriptor_pb2.FileDescriptorProto(name='seed.proto')
    seed.message_type.add(name='Seed')
    pool.Add(seed)
    barrier = threading.Barrier(8)

    def AccessPool(worker):
      barrier.wait(timeout=30)
      for iteration in range(64):
        if worker < 4:
          name = f'Registered_{worker}_{iteration}'
          proto = descriptor_pb2.FileDescriptorProto(name=name + '.proto')
          proto.message_type.add(name=name)
          pool.Add(proto)
          self.assertEqual(pool.FindMessageTypeByName(name).name, name)
        else:
          self.assertEqual(pool.FindFileByName('seed.proto').name, 'seed.proto')
          self.assertEqual(pool.FindMessageTypeByName('Seed').name, 'Seed')
      return True

    with futures.ThreadPoolExecutor(max_workers=8) as executor:
      self.assertTrue(all(executor.map(AccessPool, range(8))))

  @unittest.skipUnless(api_implementation.Type() == 'upb', 'upb lookup semantics')
  def testDescriptorTreeLookupsDuringRegistration(self):
    pool = descriptor_pool.DescriptorPool()
    proto = descriptor_pb2.FileDescriptorProto(
        name='tree.proto', package='lookup', syntax='proto2'
    )
    outer = proto.message_type.add(name='Outer')
    outer.nested_type.add(name='Inner').nested_type.add(name='Deep')
    outer.enum_type.add(name='Kind').value.add(name='NESTED_VALUE', number=0)
    outer.extension_range.add(start=100, end=200)
    outer.extension.add(
        name='extra', number=100, type=5, label=1, extendee='.lookup.Outer'
    )
    proto.service.add(name='Example')
    pool.Add(proto)
    desc = pool.FindMessageTypeByName('lookup.Outer')
    cls = message_factory.GetMessageClass(desc)
    file = desc.file
    barrier = threading.Barrier(8)

    def AccessTree(worker):
      barrier.wait(timeout=30)
      for iteration in range(128):
        if worker < 4:
          added = descriptor_pb2.FileDescriptorProto(
              name=f'tree_add_{worker}_{iteration}.proto'
          )
          added.message_type.add(name=f'TreeAdded_{worker}_{iteration}')
          pool.Add(added)
        else:
          inner = desc.nested_types_by_name['Inner']
          deep = desc.nested_types_by_name['Inner.Deep']
          self.assertIs(deep.containing_type, inner)
          self.assertIs(file.message_types_by_name['Outer.Inner'], inner)
          self.assertIs(file.enum_types_by_name['Outer.Kind'],
                        desc.enum_types_by_name['Kind'])
          self.assertIs(file.extensions_by_name['Outer.extra'],
                        desc.extensions_by_name['extra'])
          self.assertEqual(file.services_by_name['Example'].name, 'Example')
          self.assertEqual(cls.NESTED_VALUE, 0)
          self.assertIsNone(getattr(cls, 'missing_attribute', None))
      return True

    with futures.ThreadPoolExecutor(max_workers=8) as executor:
      self.assertTrue(all(executor.map(AccessTree, range(8))))

  @unittest.skipUnless(api_implementation.Type() == 'upb', 'upb lookup semantics')
  def testLongDescriptorLookupNamesAndKinds(self):
    for package in ('', 'long_package_' * 24):
      with self.subTest(package=package):
        pool = descriptor_pool.DescriptorPool()
        proto = descriptor_pb2.FileDescriptorProto(name='long.proto',
                                                   package=package)
        outer = proto.message_type.add(name='Outer')
        outer.nested_type.add(name='Child')
        outer.enum_type.add(name='Kind').value.add(name='VALUE', number=0)
        pool.Add(proto)
        name = (package + '.' if package else '') + 'Outer'
        desc = pool.FindMessageTypeByName(name)
        child = desc.nested_types_by_name['Child']
        self.assertIs(desc.file.message_types_by_name['Outer.Child'], child)
        self.assertIs(child.containing_type, desc)
        cls = message_factory.GetMessageClass(desc)
        self.assertIs(cls.Child.DESCRIPTOR, child)
        self.assertEqual(cls.Kind.Name(0), 'VALUE')
        for mapping, key in ((desc.nested_types_by_name, 'Kind'),
                             (desc.enum_types_by_name, 'Child'),
                             (desc.file.message_types_by_name, 'Outer.Kind')):
          with self.assertRaises(KeyError):
            mapping[key]

  def testParsingExtensionsDuringRegistration(self):
    pool = descriptor_pool.DescriptorPool()
    proto = descriptor_pb2.FileDescriptorProto(name='ext_base.proto',
                                               syntax='proto2')
    proto.message_type.add(name='ExtTarget').extension_range.add(
        start=100, end=1000000
    )
    pool.Add(proto)
    seed = descriptor_pb2.FileDescriptorProto(name='seed_ext.proto',
                                              dependency=['ext_base.proto'])
    seed.extension.add(name='seed', number=100, type=5, label=1,
                       extendee='.ExtTarget')
    pool.Add(seed)
    extension = pool.FindExtensionByName('seed')
    cls = message_factory.GetMessageClass(
        pool.FindMessageTypeByName('ExtTarget')
    )
    message = cls()
    message.Extensions[extension] = 123
    payload = message.SerializeToString()
    barrier = threading.Barrier(8)

    def UseRegistry(worker):
      barrier.wait(timeout=30)
      if worker < 4:
        for iteration in range(128):
          name = f'extension_{worker}_{iteration}'
          added = descriptor_pb2.FileDescriptorProto(
              name=name + '.proto', dependency=['ext_base.proto']
          )
          added.extension.add(name=name, number=101 + worker * 1000 + iteration,
                              type=5, label=1, extendee='.ExtTarget')
          pool.Add(added)
      else:
        local = cls()
        for iteration in range(2000):
          local.ParseFromString(payload)
          self.assertEqual(local.Extensions[extension], 123)
          if iteration % 100 == 0 and api_implementation.Type() == 'upb':
            self.assertIs(local.Extensions._FindExtensionByName('seed'), extension)
            self.assertIs(local.Extensions._FindExtensionByNumber(100), extension)
      return True

    with futures.ThreadPoolExecutor(max_workers=8) as executor:
      self.assertTrue(all(executor.map(UseRegistry, range(8))))

  @unittest.skipUnless(api_implementation.Type() == 'upb', 'upb rollback semantics')
  def testFailedExtensionRegistrationDuringParsing(self):
    pool = descriptor_pool.DescriptorPool()
    base = descriptor_pb2.FileDescriptorProto(name='rollback_base.proto')
    base.message_type.add(name='RollbackTarget').extension_range.add(
        start=100, end=1000
    )
    base.extension.add(name='known', number=100, type=5, label=1,
                       extendee='.RollbackTarget')
    pool.Add(base)
    known = pool.FindExtensionByName('known')
    cls = message_factory.GetMessageClass(
        pool.FindMessageTypeByName('RollbackTarget')
    )
    seed = cls()
    seed.Extensions[known] = 123
    # Field 101 must remain unknown: its declaration always belongs to a
    # rejected file whose storage can be reclaimed immediately after failure.
    payload = seed.SerializeToString() + b'\xa8\x06\x01'
    barrier = threading.Barrier(4)

    def AccessRegistry(worker):
      barrier.wait(timeout=30)
      if worker == 0:
        for _ in range(128):
          failed = descriptor_pb2.FileDescriptorProto(
              name='rejected.proto', dependency=['rollback_base.proto']
          )
          failed.extension.add(name='rejected', number=101, type=5, label=1,
                               extendee='.RollbackTarget')
          failed.extension.add(name='duplicate', number=100, type=5, label=1,
                               extendee='.RollbackTarget')
          with self.assertRaisesRegex(TypeError, 'duplicate extension'):
            pool.Add(failed)
      else:
        for _ in range(2000):
          local = cls.FromString(payload)
          self.assertEqual(local.Extensions[known], 123)
          self.assertEqual(len(local.ListFields()), 1)
          self.assertEqual(local.SerializeToString(), payload)
      return True

    with futures.ThreadPoolExecutor(max_workers=4) as executor:
      self.assertTrue(all(executor.map(AccessRegistry, range(4))))
    with self.assertRaises(KeyError):
      pool.FindExtensionByName('rejected')

  def testDefaultPoolOptionsDuringRegistration(self):
    pool = descriptor_pool.Default()
    prefix = f'options_registration_{time.time_ns()}'
    proto = descriptor_pb2.FileDescriptorProto(name=prefix + '.proto',
                                               package=prefix)
    for i in range(256):
      proto.message_type.add(name=f'Options{i}').options.deprecated = True
    pool.Add(proto)
    descriptors = [pool.FindMessageTypeByName(f'{prefix}.Options{i}')
                   for i in range(256)]
    barrier = threading.Barrier(6)

    def AccessOptions(worker):
      barrier.wait(timeout=30)
      if worker < 2:
        for iteration in range(128):
          name = f'{prefix}_{worker}_{iteration}'
          added = descriptor_pb2.FileDescriptorProto(name=name + '.proto')
          added.message_type.add(name=name)
          pool.Add(added)
      else:
        for desc in descriptors[worker - 2::4]:
          self.assertTrue(desc.GetOptions().deprecated)
      return True

    with futures.ThreadPoolExecutor(max_workers=6) as executor:
      self.assertTrue(all(executor.map(AccessOptions, range(6))))

  def testParsingBytearrayDuringResize(self):
    data = b'x' * 65536
    payload = unittest_pb2.TestAllTypes(optional_bytes=data).SerializeToString()
    shared = bytearray(payload)
    barrier = threading.Barrier(4)

    def UseBuffer(worker):
      barrier.wait(timeout=30)
      if worker == 0:
        for _ in range(1000):
          shared.clear()
          shared.extend(payload)
      else:
        for _ in range(700):
          local = unittest_pb2.TestAllTypes.FromString(shared)
          self.assertIn(local.optional_bytes, (b'', data))
      return True

    with futures.ThreadPoolExecutor(max_workers=4) as executor:
      self.assertTrue(all(executor.map(UseBuffer, range(4))))

  def testParsingMemoryviewDuringExporterMutation(self):
    values = (b'x' * 65536, b'y' * 65536)
    payloads = [unittest_pb2.TestAllTypes(optional_bytes=value).SerializeToString()
                for value in values]
    shared = bytearray(payloads[0])
    view = memoryview(shared).toreadonly()
    barrier = threading.Barrier(4)

    def UseView(worker):
      barrier.wait(timeout=30)
      if worker == 0:
        for iteration in range(1000):
          shared[:] = payloads[iteration % 2]
      else:
        for _ in range(700):
          parsed = unittest_pb2.TestAllTypes.FromString(view)
          self.assertIn(parsed.optional_bytes, values)
      return True

    with futures.ThreadPoolExecutor(max_workers=4) as executor:
      self.assertTrue(all(executor.map(UseView, range(4))))
    view.release()
    shared.clear()  # No parse export may survive the call.

  def testParsingMemoryviewShapesAndFailureCleanup(self):
    payload = unittest_pb2.TestAllTypes(optional_bytes=b'buffer').SerializeToString()
    expanded = bytearray(len(payload) * 2)
    expanded[::2] = payload
    views = (memoryview(payload), memoryview(expanded)[::2],
             memoryview(payload[::-1])[::-1])
    for view in views:
      with self.subTest(strides=view.strides):
        parsed = unittest_pb2.TestAllTypes.FromString(view)
        self.assertEqual(parsed.optional_bytes, b'buffer')
        view.release()
    expanded.clear()
    bad = bytearray(b'\x80')
    view = memoryview(bad)
    with self.assertRaises(DecodeError):
      unittest_pb2.TestAllTypes.FromString(view)
    view.release()
    bad.clear()
    with self.assertRaises(ValueError):
      unittest_pb2.TestAllTypes.FromString(view)

  def testDescriptorDatabaseCallbackAllowsOtherThread(self):
    entered = threading.Event()
    finished = threading.Event()

    class Database(descriptor_database.DescriptorDatabase):

      def FindFileByName(self, name):
        if name == 'first.proto':
          entered.set()
          if not finished.wait(timeout=30):
            raise RuntimeError('Pool critical section blocked database callback')
        return descriptor_pb2.FileDescriptorProto(name=name)

    pool = descriptor_pool.DescriptorPool(Database())

    def FindSecond():
      if not entered.wait(timeout=30):
        raise RuntimeError('Database callback did not start')
      try:
        return pool.FindFileByName('second.proto').name
      finally:
        finished.set()

    with futures.ThreadPoolExecutor(max_workers=2) as executor:
      second = executor.submit(FindSecond)
      first = executor.submit(pool.FindFileByName, 'first.proto')
      self.assertEqual(first.result(timeout=60).name, 'first.proto')
      self.assertEqual(second.result(timeout=60), 'second.proto')

  def testConcurrentOptionsCacheIdentity(self):
    pool = descriptor_pool.DescriptorPool()
    proto = descriptor_pb2.FileDescriptorProto(name='option_identity.proto')
    proto.message_type.add(name='OptionsMessage').options.deprecated = True
    pool.Add(proto)
    desc = pool.FindMessageTypeByName('OptionsMessage')
    barrier = threading.Barrier(8)

    def GetOptions(unused):
      barrier.wait(timeout=30)
      options = desc.GetOptions()
      for _ in range(100):
        self.assertIs(options, desc.GetOptions())
        self.assertTrue(options.deprecated)
      return options

    with futures.ThreadPoolExecutor(max_workers=8) as executor:
      options = list(executor.map(GetOptions, range(8)))
    for value in options:
      self.assertIs(value, options[0])

  def testConcurrentCreationReturnsOneMessageClass(self):
    pool = descriptor_pool.DescriptorPool()
    file_proto = descriptor_pb2.FileDescriptorProto(name='class_identity.proto')
    for i in range(32):
      file_proto.message_type.add(name=f'IdentityMessage{i}')
    pool.Add(file_proto)
    with futures.ThreadPoolExecutor(max_workers=8) as executor:
      for i in range(32):
        desc = pool.FindMessageTypeByName(f'IdentityMessage{i}')
        barrier = threading.Barrier(8)

        def CreateClass():
          barrier.wait(timeout=30)
          return message_factory.GetMessageClass(desc)

        pending = [executor.submit(CreateClass) for _ in range(8)]
        classes = [result.result(timeout=30) for result in pending]
        for cls in classes:
          self.assertIs(cls, classes[0])
        self.assertIs(desc._concrete_class, classes[0])

  def testConcurrentMessageObjectIdentity(self):
    msg = unittest_pb2.TestAllTypes()
    sub_objs = [None, None]

    def GetSub(idx):
      sub_objs[idx] = msg.optional_nested_message

    t1 = threading.Thread(target=GetSub, args=(0,))
    t2 = threading.Thread(target=GetSub, args=(1,))
    t1.start()
    t2.start()
    t1.join()
    t2.join()

    self.assertIs(sub_objs[0], sub_objs[1])

  def testConcurrentExtensionWrapperIdentityAndRetirement(self):
    for _ in range(16):
      shared = unittest_pb2.TestAllExtensions()
      barrier = threading.Barrier(8)

      def ReadExtensions(worker):
        barrier.wait(timeout=30)
        wrappers = [shared.Extensions for _ in range(32)]
        self.assertTrue(all(wrapper is wrappers[0] for wrapper in wrappers))
        return wrappers[0]

      with futures.ThreadPoolExecutor(max_workers=8) as executor:
        wrappers = list(executor.map(ReadExtensions, range(8)))
      self.assertTrue(all(wrapper is wrappers[0] for wrapper in wrappers))
      del wrappers

      # Repeated destruction/recreation must leave neither stale references
      # nor a losing candidate that can evict the published wrapper.
      def RetireExtensions(worker):
        for _ in range(128):
          self.assertEqual(len(shared.Extensions), 0)
        return True

      with futures.ThreadPoolExecutor(max_workers=8) as executor:
        self.assertTrue(all(executor.map(RetireExtensions, range(8))))

  def testExtensionWrapperOwnsMessage(self):
    shared = unittest_pb2.TestAllExtensions()
    wrappers = shared.Extensions
    del shared
    gc.collect()
    wrappers[unittest_pb2.optional_int32_extension] = 42
    self.assertEqual(wrappers[unittest_pb2.optional_int32_extension], 42)

  def testConcurrentDescriptorObjectIdentity(self):
    desc_objs = [None, None]

    def GetDesc(idx):
      desc_objs[idx] = unittest_pb2.TestAllTypes.DESCRIPTOR

    t1 = threading.Thread(target=GetDesc, args=(0,))
    t2 = threading.Thread(target=GetDesc, args=(1,))
    t1.start()
    t2.start()
    t1.join()
    t2.join()

    self.assertIs(desc_objs[0], desc_objs[1])

  def testFieldDecodersDataRace(self):
    msg = unittest_pb2.TestAllTypes(optional_int32=1)
    serialized_data = msg.SerializeToString()
    lock = threading.Lock()

    def ParseMessage():
      parsed_msg = unittest_pb2.TestAllTypes()
      time.sleep(0.005)
      parsed_msg.ParseFromString(serialized_data)
      with lock:
        if msg == parsed_msg:
          self.success += 1

    field_des = unittest_pb2.TestAllTypes.DESCRIPTOR.fields_by_name[
        'optional_int32'
    ]
    count = 1000
    for x in range(0, count):
      # delete the _decoders because only the first time parse the field
      # may cause data race.
      if hasattr(field_des, '_decoders'):
        delattr(field_des, '_decoders')
      thread1 = threading.Thread(target=ParseMessage)
      thread2 = threading.Thread(target=ParseMessage)
      thread1.start()
      thread2.start()
      thread1.join()
      thread2.join()

    self.assertEqual(count * 2, self.success)

  # This caused a Dealloc()/Dealloc() race.
  def testGetType(self):

    def GetType():
      msg = unittest_proto3_pb2.TestAllTypes(
          optional_nested_message=unittest_proto3_pb2.TestAllTypes.NestedMessage(
              bb=1000
          ),
          optional_nested_enum=unittest_proto3_pb2.TestAllTypes.NestedEnum.ZERO,
      )
      msges = [msg] * 100
      for m in msges:
        # Fails in this line:
        unittest_proto3_pb2.TestAllTypes.NestedEnum.Name(m.optional_nested_enum)

    threads = []
    for i in range(100):
      thread = threading.Thread(target=GetType)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()

  # This caused a race between constructing and using the type.
  def testInitType(self):

    def InitType():
      array = []
      for i in range(100):
        array.append(
            unittest_proto3_pb2.TestAllTypes(
                optional_nested_message=unittest_proto3_pb2.TestAllTypes.NestedMessage(
                    bb=1000
                ),
                optional_nested_enum=unittest_proto3_pb2.TestAllTypes.NestedEnum.FOO,
            )
        )

    threads = []
    for i in range(100):
      thread = threading.Thread(target=InitType)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()

  def testConcurrentSubMessageAccess(self):
    msg = unittest_proto3_pb2.TestAllTypes(
        optional_nested_message=unittest_proto3_pb2.TestAllTypes.NestedMessage(
            bb=1000
        )
    )

    def AccessSubMessage():
      for _ in range(100):
        _ = msg.optional_nested_message.bb

    threads = []
    for i in range(100):
      thread = threading.Thread(target=AccessSubMessage)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()

  def testConcurrentRepeatedMessageAccess(self):
    variable = unittest_proto3_pb2.TestAllTypes()

    def UseVariable():
      for _ in range(1000):
        _ = variable.repeated_nested_message

    threads = []
    for i in range(100):
      thread = threading.Thread(target=UseVariable)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()

  def testConcurrentRepeatedPrimitiveAccess(self):
    variable = unittest_proto3_pb2.TestAllTypes()
    variable.repeated_float.append(1.0)

    def UseVariable():
      for _ in range(1000):
        _ = variable.repeated_float

    threads = []
    for i in range(100):
      thread = threading.Thread(target=UseVariable)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()

  def testConcurrentSingularFieldAccess(self):
    variable = unittest_proto3_pb2.TestAllTypes()

    def UseVariable():
      for _ in range(1000):
        _ = variable.optional_int32
        _ = variable.optional_string

    threads = []
    for i in range(100):
      thread = threading.Thread(target=UseVariable)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()

  def testConcurrentRepeatedMessageAccess2(self):
    msg = unittest_proto3_pb2.TestAllTypes(
        repeated_nested_message=[
            unittest_proto3_pb2.TestAllTypes.NestedMessage(bb=1)
        ]
    )

    def UseVariable():
      for _ in range(1000):
        for nested in msg.repeated_nested_message:
          pass

    threads = []
    for _ in range(100):
      thread = threading.Thread(target=UseVariable)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()

  def testConcurrentMergeFromAndContainerDealloc(self):
    msg = test_proto2_pb2.TestProto2()
    other = test_proto2_pb2.TestProto2(optional_int32=1)
    lock = threading.Lock()

    def MergeLoop():
      for _ in range(1000):
        with lock:
          msg.MergeFrom(other)

    def AccessAndDropLoop():
      for _ in range(1000):
        with lock:
          containers = (
              msg.repeated_int32,
              msg.repeated_nested_message,
          )
        del containers

    threads = [threading.Thread(target=MergeLoop)]
    for _ in range(4):
      threads.append(threading.Thread(target=AccessAndDropLoop))
    for thread in threads:
      thread.start()
    for thread in threads:
      thread.join()


class FreeThreadingTest(unittest.TestCase):

  def RunThreads(self, thread_size, func):
    threads = []
    for i in range(0, thread_size):
      threads.append(threading.Thread(target=func))
    for thread in threads:
      thread.start()
    for thread in threads:
      thread.join()

  def testDoNothing(self):
    thread_size = 10

    def DoNothing():
      return

    self.RunThreads(thread_size, DoNothing)

  def testDescriptorPoolMap(self):
    thread_size = 20
    self.success_count = 0
    lock = threading.Lock()

    def CreatePool():
      def DoCreate():
        pool = descriptor_pool.DescriptorPool()
        file_proto = descriptor_pb2.FileDescriptorProto(name='foo')
        message_proto = file_proto.message_type.add(name='SomeMessage')
        message_proto.field.add(
            name='int_field',
            number=1,
            type=descriptor_pb2.FieldDescriptorProto.TYPE_INT32,
            label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
        )
        pool.Add(file_proto)
        desc = pool.FindMessageTypeByName('SomeMessage')
        msg = message_factory.GetMessageClass(desc)()
        msg.int_field = 1

      DoCreate()
      with lock:
        self.success_count += 1

    self.RunThreads(thread_size, CreatePool)
    self.assertEqual(thread_size, self.success_count)

  def testConcurrentGetFieldValueRace(self):
    """Reproduces a data race in GetFieldValue due to lazy initialization."""

    def AccessFields(msg, barrier) -> None:
      barrier.wait()
      # This access triggers GetFieldValue and lazy initialization
      # of the composite_fields map in CMessage.
      _ = msg.optional_nested_message

    for _ in range(100):
      threads = []
      msg = unittest_proto3_pb2.TestAllTypes()

      # Use a barrier to ensure all threads hit the GetFieldValue call
      # at nearly the same time, maximizing the race window.
      barrier = threading.Barrier(10)

      for _ in range(10):
        thread = threading.Thread(target=AccessFields, args=(msg, barrier))
        threads.append(thread)
        thread.start()

      for thread in threads:
        thread.join()

  def testConcurrentGetOptionsRace(self):
    """Reproduces a data race in GetOptions."""

    def AccessOptions(barrier):
      barrier.wait()
      _ = unittest_proto3_pb2.TestAllTypes.DESCRIPTOR.GetOptions()

    for _ in range(100):
      threads = []
      barrier = threading.Barrier(20)

      for _ in range(20):
        thread = threading.Thread(target=AccessOptions, args=(barrier,))
        threads.append(thread)
        thread.start()

      for thread in threads:
        thread.join()

  def testConcurrentGetAndRegisterMessageClassDataRace(self):
    """Reproduces the data race in GetMessageClass/RegisterMessageClass."""
    pool = descriptor_pool.DescriptorPool()

    # Create and register a base Message class to look up
    file_proto = descriptor_pb2.FileDescriptorProto(name='base.proto')
    file_proto.message_type.add(name='BaseMessage')
    pool.Add(file_proto)
    base_desc = pool.FindMessageTypeByName('BaseMessage')
    message_factory.GetMessageClass(base_desc)

    # Pre-create descriptors for modification
    num_descriptors = 500
    descriptors = []
    for i in range(num_descriptors):
      name = f'DynamicMessage_{i}'
      f_proto = descriptor_pb2.FileDescriptorProto(name=f'{name}.proto')
      f_proto.message_type.add(name=name)
      pool.Add(f_proto)
      descriptors.append(pool.FindMessageTypeByName(name))

    barrier = threading.Barrier(10)

    def Task(thread_id: int):
      barrier.wait()
      if thread_id % 2 == 0:
        # Reader thread: repeatedly looks up the existing message class
        for _ in range(200):
          message_factory.GetMessageClass(base_desc)
      else:
        # Writer thread: registers new message classes concurrently
        start_idx = (thread_id // 2) * 100
        for i in range(start_idx, start_idx + 100):
          message_factory.GetMessageClass(descriptors[i])

    threads = []
    for i in range(10):
      threads.append(threading.Thread(target=Task, args=(i,)))
    for thread in threads:
      thread.start()
    for thread in threads:
      thread.join()

  def testConcurrentDescriptorFileAccessDataRace(self):
    """Reproduces the data race in PyFileDescriptor_FromDescriptorWithSerializedPb."""
    pool = descriptor_pool.DescriptorPool()
    num_messages = 500
    descriptors = []
    for i in range(num_messages):
      f_proto = descriptor_pb2.FileDescriptorProto(name=f'race_{i}.proto')
      f_proto.message_type.add(name=f'Message_{i}')
      pool.Add(f_proto)
      descriptors.append(pool.FindMessageTypeByName(f'Message_{i}'))

    barrier = threading.Barrier(10)

    def Worker():
      barrier.wait()
      for desc in descriptors:
        _ = getattr(getattr(desc, 'file', None), 'name', '')

    threads = [threading.Thread(target=Worker) for _ in range(10)]
    for t in threads:
      t.start()
    for t in threads:
      t.join()

  @unittest.skipIf(
      api_implementation.Type() != 'cpp',
      'BuildFileErrorCollector only exists in the cpp implementation.',
  )
  @unittest.skipIf(
      getattr(sys, '_is_gil_enabled', lambda: True)(),
      'Requires free-threaded Python (GIL-enabled runs deadlock in'
      ' PyDescriptorDatabase).',
  )
  def testConcurrentBuildFileErrorCollectorRaisesKeyError(self):
    db = descriptor_database.DescriptorDatabase()
    num_threads = 10
    for i in range(num_threads):
      f_proto = descriptor_pb2.FileDescriptorProto(
          name=f'bad_{i}.proto', package=f'pkg_{i}'
      )
      msg = f_proto.message_type.add(name='BadMessage')
      msg.field.add(
          name='bad_field',
          number=1,
          label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
          type=descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE,
          type_name='UndefinedSubMessage',
      )
      db.Add(f_proto)

    pool = descriptor_pool.DescriptorPool(db)
    barrier = threading.Barrier(num_threads)
    errors = []

    def Worker(thread_id):
      barrier.wait()
      try:
        for i in range(20):
          idx = (thread_id + i) % num_threads
          bad_file = f'bad_{idx}.proto'
          with self.assertRaises(KeyError) as exc:
            pool.FindFileByName(bad_file)
          self.assertIn('Invalid proto descriptor', str(exc.exception))
          self.assertIn(bad_file, str(exc.exception))

          with self.assertRaises(KeyError) as exc:
            pool.FindMessageTypeByName(f'pkg_{idx}.BadMessage')
          self.assertIn('Invalid proto descriptor', str(exc.exception))
          self.assertIn(bad_file, str(exc.exception))

          with self.assertRaises(KeyError) as exc:
            pool.FindFileByName(f'missing_{idx}.proto')
          self.assertNotIn('Invalid proto descriptor', str(exc.exception))

          with self.assertRaises(KeyError) as exc:
            pool.FindMessageTypeByName(f'pkg_{idx}.MissingMessage')
          self.assertNotIn('Invalid proto descriptor', str(exc.exception))
      except Exception as e:  # pylint: disable=broad-exception-caught
        errors.append(e)

    threads = [
        threading.Thread(target=Worker, args=(i,)) for i in range(num_threads)
    ]
    for t in threads:
      t.start()
    for t in threads:
      t.join()
    self.assertEqual([], errors)

  def testConcurrentDescriptorDeallocRace(self):
    """Tests descriptor cache interning under concurrent deallocation."""
    pool = descriptor_pool.DescriptorPool()
    file_proto = descriptor_pb2.FileDescriptorProto(name='race.proto')
    file_proto.message_type.add(name='RaceMessage')
    pool.Add(file_proto)

    barrier = threading.Barrier(10)
    errors = []

    def Worker():
      barrier.wait()
      for _ in range(500):
        try:
          d1 = pool.FindMessageTypeByName('RaceMessage')
          d2 = pool.FindMessageTypeByName('RaceMessage')
          if d1 is not d2:
            errors.append('Descriptor interning broken')
            break
          # Explicitly delete local references to trigger concurrent tp_dealloc
          del d1
          del d2
        except Exception as e:
          errors.append(str(e))
          break

    threads = [threading.Thread(target=Worker) for _ in range(10)]
    for t in threads:
      t.start()
    for t in threads:
      t.join()
    self.assertEqual([], errors)

  def testConcurrentSubmessageDeallocRace(self):
    """Tests child submessage wrapper interning under concurrent deallocation."""
    msg = unittest_proto3_pb2.TestAllTypes()
    msg.repeated_nested_message.add(bb=123)

    barrier = threading.Barrier(10)
    errors = []

    def Worker():
      barrier.wait()
      for _ in range(500):
        try:
          m1 = msg.repeated_nested_message[0]
          m2 = msg.repeated_nested_message[0]
          if m1 is not m2:
            errors.append('Child submessage interning broken')
            break
          del m1
          del m2
        except Exception as e:
          errors.append(str(e))
          break

    threads = [threading.Thread(target=Worker) for _ in range(10)]
    for t in threads:
      t.start()
    for t in threads:
      t.join()
    self.assertEqual([], errors)

  def testConcurrentCompositeFieldDeallocRace(self):
    """Tests composite field wrapper interning under concurrent deallocation."""
    msg = unittest_proto3_pb2.TestAllTypes()

    barrier = threading.Barrier(10)
    errors = []

    def Worker():
      barrier.wait()
      for _ in range(500):
        try:
          # Test singular composite field wrapper
          sub1 = msg.optional_nested_message
          sub2 = msg.optional_nested_message
          if sub1 is not sub2:
            errors.append('Singular composite field interning broken')
            break
          del sub1
          del sub2

          # Test repeated container wrapper
          rep1 = msg.repeated_int32
          rep2 = msg.repeated_int32
          if rep1 is not rep2:
            errors.append('Repeated container interning broken')
            break
          del rep1
          del rep2
        except Exception as e:
          errors.append(str(e))
          break

    threads = [threading.Thread(target=Worker) for _ in range(10)]
    for t in threads:
      t.start()
    for t in threads:
      t.join()
    self.assertEqual([], errors)

  def testConcurrentClearAndSubObjectDeletionRace(self):
    """Reproduces a dangling pointer dereference race between Clear() and sub-object deallocation."""

    def ClearMsg(msg, barrier):
      barrier.wait()
      msg.Clear()

    def DeleteSub(container, barrier):
      barrier.wait()
      container.clear()

    for _ in range(500):
      msg = unittest_proto3_pb2.TestAllTypes()
      container = [msg.optional_nested_message]
      barrier = threading.Barrier(2)

      thread1 = threading.Thread(target=ClearMsg, args=(msg, barrier))
      thread2 = threading.Thread(target=DeleteSub, args=(container, barrier))
      thread1.start()
      thread2.start()
      thread1.join()
      thread2.join()

  @unittest.skipIf(not ALSO_RUN_BENCHMARKS, 'Benchmarks are disabled.')
  def testConcurrentGetOptionsBenchmark(self):
    """Benchmarks concurrent GetOptions calls."""
    if ALSO_RUN_BENCHMARKS:

      def AccessOptions():
        for _ in range(1000000):
          _ = unittest_proto3_pb2.TestAllTypes.DESCRIPTOR.GetOptions()

      def RunAllThreads():
        self.RunThreads(20, AccessOptions)

      duration = timeit.timeit(RunAllThreads, number=10)
      duration_ms = duration * 1000
      print(
          'ConcurrentGetOptionsBenchmark (20 threads x 1000000 calls x 10'
          f' runs): {duration_ms:.2f}ms'
      )
    else:
      print('Skipping benchmark in non-benchmark mode.')

  def testConcurrentLazyUnpackAndRead(self):
    # 1. Create a template proto containing a lazy sub-message
    template = test_proto2_pb2.ReproMessageForLazy()
    template.lazy_field.value = 'repro_value'
    serialized_bytes = template.SerializeToString()

    # 2. Helper to run concurrent read/write loops on shared unparsed instances
    def RunRace():
      # Parse a fresh unparsed message instance
      shared_msg = test_proto2_pb2.ReproMessageForLazy.FromString(
          serialized_bytes
      )

      barrier = threading.Barrier(2)

      def ThreadWriter():
        barrier.wait()
        # Access the lazy field for the first time.
        # This forces the C++ protobuf library to unpack the lazy field,
        _ = shared_msg.lazy_field.value

      def ThreadReader():
        barrier.wait()
        # Concurrently read field presence or format to string.
        _ = shared_msg.HasField('lazy_field')
        _ = str(shared_msg)

      t1 = threading.Thread(target=ThreadWriter)
      t2 = threading.Thread(target=ThreadReader)

      t1.start()
      t2.start()
      t1.join()
      t2.join()

    # 3. Run in a loop to reliably trigger
    for _ in range(500):
      RunRace()

  def testConcurrentRepeatedCompositeSubscript(self):
    msg = test_proto2_pb2.ContainerForRepeatedComposite()
    msg.submessage.items.add(value='foo')
    msg.submessage.items.add(value='bar')
    serialized = msg.SerializeToString()

    def RunRace():
      shared_msg = test_proto2_pb2.ContainerForRepeatedComposite.FromString(
          serialized
      )
      barrier = threading.Barrier(2)

      def Thread1():
        barrier.wait()
        _ = shared_msg.submessage.items[0].value

      def Thread2():
        barrier.wait()
        _ = shared_msg.submessage.items[1].value

      t1 = threading.Thread(target=Thread1)
      t2 = threading.Thread(target=Thread2)
      t1.start()
      t2.start()
      t1.join()
      t2.join()

    for _ in range(500):
      RunRace()

  def testLazyFieldRepeatedChildMutation(self):
    msg = test_proto2_pb2.ContainerForLazyRepeatedAndMap()
    msg.lazy_field.repeated_items.add(value='initial')
    serialized = msg.SerializeToString()

    parsed = test_proto2_pb2.ContainerForLazyRepeatedAndMap.FromString(
        serialized
    )
    item = parsed.lazy_field.repeated_items[0]
    item.value = 'updated'

    reserialized = parsed.SerializeToString()
    reparsed = test_proto2_pb2.ContainerForLazyRepeatedAndMap.FromString(
        reserialized
    )
    self.assertEqual(reparsed.lazy_field.repeated_items[0].value, 'updated')

  def testLazyFieldMapChildMutation(self):
    msg = test_proto2_pb2.ContainerForLazyRepeatedAndMap()
    msg.lazy_field.map_items['key'].value = 'initial'
    serialized = msg.SerializeToString()

    parsed = test_proto2_pb2.ContainerForLazyRepeatedAndMap.FromString(
        serialized
    )
    item = parsed.lazy_field.map_items['key']
    item.value = 'updated'

    reserialized = parsed.SerializeToString()
    reparsed = test_proto2_pb2.ContainerForLazyRepeatedAndMap.FromString(
        reserialized
    )
    self.assertEqual(reparsed.lazy_field.map_items['key'].value, 'updated')

  def testConcurrentMapSubscriptReadAndSerialization(self):
    msg = test_proto2_pb2.MessageWithRepeatedAndMap()
    msg.map_items['k1'].value = 'v1'
    msg.map_items['k2'].value = 'v2'
    msg.scalar_map['s1'] = 'val1'
    msg.scalar_map['s2'] = 'val2'
    serialized = msg.SerializeToString()

    def RunRace():
      shared_msg = test_proto2_pb2.MessageWithRepeatedAndMap.FromString(
          serialized
      )
      barrier = threading.Barrier(3)
      errors = []

      def Thread1():
        try:
          barrier.wait()
          self.assertEqual(shared_msg.map_items['k1'].value, 'v1')
          self.assertEqual(shared_msg.map_items.get('k1').value, 'v1')
          self.assertIsNone(shared_msg.map_items.get('missing'))
          self.assertEqual(shared_msg.scalar_map['s1'], 'val1')
          self.assertEqual(shared_msg.scalar_map.get('s1'), 'val1')
          self.assertIsNone(shared_msg.scalar_map.get('missing'))
        except Exception as e:
          errors.append(e)

      def Thread2():
        try:
          barrier.wait()
          self.assertEqual(shared_msg.map_items['k2'].value, 'v2')
          self.assertEqual(shared_msg.map_items.get('k2').value, 'v2')
          self.assertIsNone(shared_msg.map_items.get('missing2'))
          self.assertEqual(shared_msg.scalar_map['s2'], 'val2')
          self.assertEqual(shared_msg.scalar_map.get('s2'), 'val2')
          self.assertIsNone(shared_msg.scalar_map.get('missing2'))
        except Exception as e:
          errors.append(e)

      def Thread3():
        try:
          barrier.wait()
          parsed_msg = test_proto2_pb2.MessageWithRepeatedAndMap.FromString(
              shared_msg.SerializeToString()
          )
          self.assertEqual(parsed_msg, msg)
        except Exception as e:
          errors.append(e)

      threads = [
          threading.Thread(target=Thread1),
          threading.Thread(target=Thread2),
          threading.Thread(target=Thread3),
      ]
      for t in threads:
        t.start()
      for t in threads:
        t.join()
      for err in errors:
        raise err

    for _ in range(200):
      RunRace()

  def testConcurrentMapIterationAndSerialization(self):
    msg = test_proto2_pb2.MessageWithRepeatedAndMap()
    msg.map_items['k1'].value = 'v1'
    msg.map_items['k2'].value = 'v2'
    msg.scalar_map['s1'] = 'val1'
    msg.scalar_map['s2'] = 'val2'
    serialized = msg.SerializeToString()

    def RunRace():
      shared_msg = test_proto2_pb2.MessageWithRepeatedAndMap.FromString(
          serialized
      )
      barrier = threading.Barrier(3)
      errors = []

      def Thread1():
        try:
          barrier.wait()
          items = dict(shared_msg.map_items.items())
          self.assertEqual(items['k1'].value, 'v1')
          self.assertEqual(items['k2'].value, 'v2')
          scalar_items = dict(shared_msg.scalar_map.items())
          self.assertEqual(scalar_items['s1'], 'val1')
          self.assertEqual(scalar_items['s2'], 'val2')
        except Exception as e:
          errors.append(e)

      def Thread2():
        try:
          barrier.wait()
          keys = list(shared_msg.map_items)
          self.assertCountEqual(keys, ['k1', 'k2'])
          scalar_keys = list(shared_msg.scalar_map)
          self.assertCountEqual(scalar_keys, ['s1', 's2'])
        except Exception as e:
          errors.append(e)

      def Thread3():
        try:
          barrier.wait()
          parsed_msg = test_proto2_pb2.MessageWithRepeatedAndMap.FromString(
              shared_msg.SerializeToString()
          )
          self.assertEqual(parsed_msg, msg)
        except Exception as e:
          errors.append(e)

      threads = [
          threading.Thread(target=Thread1),
          threading.Thread(target=Thread2),
          threading.Thread(target=Thread3),
      ]
      for t in threads:
        t.start()
      for t in threads:
        t.join()
      for err in errors:
        raise err

    for _ in range(200):
      RunRace()

  def testConcurrentMapMultiWorkerContention(self):
    """Verifies thread-safety under sustained multi-worker contention."""
    msg = test_proto2_pb2.MessageWithRepeatedAndMap()
    for i in range(10):
      msg.map_items[f'key_{i}'].value = f'val_{i}'
      msg.scalar_map[f'key_{i}'] = f'val_{i}'

    errors = []

    def ReaderWorker():
      try:
        for _ in range(50):
          for i in range(10):
            val = msg.map_items[f'key_{i}'].value
            if val != f'val_{i}':
              errors.append(f'Unexpected map_items value: {val}')
            val_get = msg.map_items.get(f'key_{i}').value
            if val_get != f'val_{i}':
              errors.append(f'Unexpected map_items get value: {val_get}')
            sval = msg.scalar_map[f'key_{i}']
            if sval != f'val_{i}':
              errors.append(f'Unexpected scalar_map value: {sval}')
            sval_get = msg.scalar_map.get(f'key_{i}')
            if sval_get != f'val_{i}':
              errors.append(f'Unexpected scalar_map get value: {sval_get}')
          for k, v in msg.map_items.items():
            _ = k, v.value
          for k, v in msg.scalar_map.items():
            _ = k, v
      except Exception as e:
        errors.append(str(e))

    def SerializerWorker():
      try:
        for _ in range(50):
          data = msg.SerializeToString()
          parsed = test_proto2_pb2.MessageWithRepeatedAndMap.FromString(data)
          if parsed != msg:
            errors.append(
                'Serialized message mismatch during concurrent map read'
            )
      except Exception as e:
        errors.append(str(e))

    threads = []
    for _ in range(8):
      threads.append(threading.Thread(target=ReaderWorker))
      threads.append(threading.Thread(target=SerializerWorker))

    for t in threads:
      t.start()
    for t in threads:
      t.join()

    self.assertEqual(errors, [])


if __name__ == '__main__':
  print(
      'protobuf thread tests: backend=%s GIL=%s extension=%s'
      % (
          api_implementation.Type(),
          getattr(sys, '_is_gil_enabled', lambda: True)(),
          getattr(api_implementation._c_module, '__file__', None),
      ),
      file=sys.stderr,
      flush=True,
  )
  unittest.main()
