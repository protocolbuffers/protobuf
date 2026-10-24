# Native Python/upb lock review — second pass, 2026-10-21

Scope: the experimental CPython 3.14t native upb backend, its C implementation,
and native test fixtures. This does not audit the legacy C++ Python backend or
arbitrary application locks. The initial review of b630c7965 is retained in
Git history; the second pass resolves its cache/exception reentrancy concern.

## Findings and corrections

Two bounded native probes reproduced hangs against the original weak-map
implementation. Both exceeded a 10-second subprocess timeout:

1. A weak-map iterator retained its non-recursive mutex while its consumer
   reentered the map. Real iterator consumers perform reification, native
   allocation and Python exception construction; holding the mutex across
   those operations exposed an avoidable reentrancy boundary.
2. Failed cache insertion constructed MemoryError under the mutex. A native
   fault-injection probe installed a pending exception whose finalizer
   reentered the map; replacing that exception deadlocked.

These are controlled native reentrancy/failure-path reproductions. They do
not establish that a public Python API workload has triggered either hang,
and the pending-exception injection deliberately exercises a defensive edge.

The implementation now snapshots live cache entries with strong references
under the mutex and releases it before running any iterator consumer. Snapshot
cleanup decrefs without a mutex. Deletion checks key and object identity so a
newer cache entry cannot be removed by an older snapshot. Cache insertion now
constructs MemoryError only after unlocking. Snapshot allocation failures also
unlock before constructing an exception.

This removes the unset-cache → arena-cache nested acquisition previously
present during reification. No consumer, Python exception constructor, or
reference retirement remains inside a raw weak-map mutex region.

## Synchronization audit

| Area | Mechanism | Assessment |
| --- | --- | --- |
| Mutable arena access | Atomic reader count / exclusive writer sentinel | Writes and upgrades fail immediately on conflict. Reader CAS retries on state changes; lock-free, not wait-free or fair. No blocking arena-owner acquisition. |
| Copy/merge across arenas | Read/source and write/destination admission, or source serialization before target mutation | Partial admission is unwound on failure; alias paths avoid duplicate acquisition. Opposing copies are covered by bounded tests. |
| Wrapper caches | PyMutex around native table operations and snapshot acquisition | No Python call, exception creation, decref or other cache acquisition while held. TryIncRef uses CPython atomic reference operations. The runtime cache allocator is native malloc/realloc, not a Python callback. |
| Stub reification | Strong-reference snapshot plus existing arena admission | Cache mutex is released before reification. Snapshot entries keep wrappers alive; callers retain the owner. Delete is identity-checked under a short separate lock. |
| Descriptor pools | CPython suspendable critical sections | Database callbacks and import waits can suspend outer sections. Opposing pool callbacks are tested. No raw cache mutex is held across those callbacks. |
| Shared iterators | CPython critical sections around cursor changes | Arena admission is taken before the iterator section where required and retired on exit. Conflicting writers fail rather than waiting on readers. |
| Lazy publication | Atomic pointer CAS | Initialization precedes publication; loser retirement runs without a publication mutex. |
| Destruction and module ownership | Strong arena/pool/module ownership; short cache erasure | Parent/arena decrefs follow cache unlock. Database and weakref finalizers run without a cache mutex held. Existing teardown, module-removal, subinterpreter and wrapper-deallocation tests remain enabled. |
| Native arena fusion | Atomic root linking | upb orders roots by address to avoid parent cycles. No additional binding-level blocking lock is introduced. |

## Regression coverage

The test-only `free_threading/_weak_map_test.so` fixture is excluded from
distribution targets and incompatible with the ordinary nonexperimental
configuration. Each probe runs in a separate subprocess, bounded at 30 seconds;
the allocation hook is used only in these isolated test processes.

Probes cover iterator reentry, exception-finalizer reentry on failed insertion,
replacement preservation, deferred snapshot-object retirement, empty snapshots,
and reentry on snapshot allocation failure. Existing tests cover opposing
descriptor pool callbacks, opposing copies, scalar callbacks, shared iterator
cursors, cache churn, GC, atexit and native module lifetime.

The original 10-second timeout evidence is preserved separately. An optional
hang-dump experiment using faulthandler triggered a CPython traceback-reader TSan
diagnostic; it is retained as diagnostic evidence, not attributed to protobuf.
Regression tests use parent-process timeouts, without that in-process timer or
any new sanitizer suppression.

## Qualification limits

No remaining blocking cycle was identified in the reviewed binding paths after
these changes. This is not proof of deadlock freedom for every interleaving,
every interpreter implementation, or arbitrary user callbacks.

Injected allocation failures cover cache insertion and snapshot allocation;
they do not exhaust every allocation site in protobuf or CPython. Application
locks or callbacks that wait indefinitely can still deadlock application code.
Arena admission makes no fairness guarantee.

The older 18-hour campaign tests b630c7965 and cannot qualify this runtime
change. A fresh pinned campaign, including the native reentrancy probes, must
record its own final result. Prior benchmark numbers also describe b630c7965,
not the changed runtime.

## Validation of the correction

| Suite | TSan | ASan | Classic CPython 3.11 / limited API |
| --- | --- | --- | --- |
| Native reentrancy/failure probes | 5 passed | 5 passed | Not applicable (no raw cache mutex) |
| Python concurrency | 69 tests, 2 skipped, passed | 69 tests, 2 skipped, passed | 69 tests, 12 skipped, passed |
| Message behavior | 367 tests, 11 skipped, passed | 367 tests, 11 skipped, passed | 367 tests, 11 skipped, passed |
| Descriptors | 122 tests, 4 skipped, passed | Not rerun | Not rerun |
| Descriptor pools | 135 tests, 7 skipped, passed | Not rerun | Not rerun |

TSan uses instrumented CPython and extension; ASan uses an instrumented extension
with Python leak detection disabled. No force-GIL-off override was used.
The first classic build attempt selected missing system Python headers; the
successful build explicitly selected the existing Python 3.10 limited-API
headers and ran under CPython 3.11.

Build the standalone native fixture with the experimental flag and full Python
headers, then run it with the matching free-threaded interpreter:

```sh
bazel build --//python:limited_api=false \
  --//python:experimental_free_threading=true \
  //python:free_threading/_weak_map_test.so
python3.14t -S python/free_threading/weak_map_test.py \
  bazel-bin/python/free_threading/_weak_map_test.so
```

Sanitizer builds additionally require their matching interpreter/runtime
environment. A distribution wheel and the fresh sustained campaign are separate
qualification gates; the bounded results above do not imply their completion.
