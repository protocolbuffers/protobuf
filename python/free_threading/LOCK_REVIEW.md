# Native Python/upb lock review — 2026-10-21

Runtime reviewed: b630c7965485eed019e5afd3e6fa67176e3123bd.
Scope: the experimental CPython 3.14t native upb backend, its C implementation,
and the native C++ admission tests. This is not an audit of the separate legacy
C++ Python backend or of arbitrary application callbacks.

## Findings

No blocking lock cycle was identified in the reviewed normal operation paths.
This is a bounded code review and test result, not a proof of deadlock freedom.
The runtime remains experimental.

The first sustained campaign stopped after 1,486 successful steps at
2026-10-21 16:42:43 UTC. The failing step completed in 33 ms and reported
`reads == 0` in NativeStorageIsProtectedUnderContention; it did not report a
timeout or sanitizer diagnostic. Fail-fast admission has no fairness guarantee:
all finite attempts by either readers or writers can be rejected. The test now
requires aggregate progress, verifies both admission modes after workers join,
and retains the deterministic 2,000-iteration writer/reader handoff test.
No runtime implementation changes were required for this test correction.

## Synchronization and ordering

| Area | Mechanism | Ordering/lifetime assessment |
| --- | --- | --- |
| Mutable arena access (`free_threading/access.c`) | Atomic reader count and exclusive writer sentinel | Writer and upgrade attempts return immediately on conflict. Reader CAS retries only while the state changes; this is lock-free, not wait-free. There is no fairness or bounded per-thread progress guarantee. |
| Cross-message copy/merge (`message.c`) | Source read and destination write admissions | Failure releases admissions already acquired; aliases avoid duplicate admission. These are not blocking acquisitions, so opposite copy directions cannot wait on each other's arena ownership. |
| Wrapper caches (`free_threading/weak_map.c`) | Non-recursive PyMutex | Ordinary get/add/erase operations release the mutex before dropping cached or losing wrapper references. Iteration retains visited wrappers and defers their decrefs until after unlock. |
| Stub reification (`message.c`, `map.c`, `repeated.c`) | Unset-subobject cache mutex may nest an arena-cache mutex | Reviewed direction is unset cache → arena cache. Ordinary cache operations release their mutex before parent/arena reference retirement; no reverse overlapping acquisition was found. SyncStubSubobjs explicitly pins the parent through iterator cleanup; Clear retains its call receiver. |
| Descriptor pools (`descriptor_pool.c`) | CPython suspendable object critical sections | Database callbacks and nested imports may suspend the outer critical section when blocking. No raw cache mutex is held across the database callback. Opposing two-pool callbacks are now tested explicitly. |
| Shared iterators (`descriptor_containers.c`, `map.c`, `extension_dict.c`) | Object critical section around cursor changes; arena read admission where applicable | Read admission precedes iterator critical section and is retired on exit. Competing writers fail instead of waiting for readers. |
| Lazy publication (`free_threading/lazy_ptr.c`) | Atomic pointer publication | Initialization occurs before CAS; losing values are freed without a publication mutex held. Reentrancy does not wait for an initialization owner. |
| Destruction/module ownership | Strong references to arenas, pools and defining modules | Message/container cache erasure returns before parent/arena decref. Pool database retirement and weakref callbacks occur outside cache mutex ownership. Existing module-removal, subinterpreter, atexit, and concurrent wrapper-deallocation tests cover representative lifetimes. |

## Callback behavior

Scalar conversion callbacks running under arena admission must receive
RuntimeError for conflicting reentry rather than waiting. Existing bounded
callback tests exercise that behavior. WKT/public initialization and sorting
have explicitly separated callback phases; immutable snapshots or admissions
protect native accesses around those phases.

New subprocess-bounded tests exercise:
- Two simultaneous descriptor database callbacks entering the opposite pool.
- 4,000 opposing CopyFrom attempts, with successful post-contention reuse.
- Database destruction that waits for a worker to look up another pool.

## Remaining limits

- User callbacks can create their own application-lock or executor dependency
  cycles. The package cannot guarantee progress for arbitrary callback code.
- Allocation-failure paths have not been systematically fault-injected here.
  MemoryError construction is still present inside cache/reification lock
  regions; this review does not establish safety for arbitrary allocator or GC
  reentrancy under memory exhaustion. This is a residual review concern, not a
  reproduced deadlock, and prevents an unconditional deadlock-free claim.
- Sanitizers and finite stress runs do not enumerate all interleavings.
  Instrumented CPython is used for TSan; ASan instruments the extension, with
  Python leak detection disabled. Native ASan tests retain leak detection.
- The initial campaign is failed, not passed. A fresh 18-hour run must record its
  own final outcome; elapsed time from the stopped run is not credited to it.

## Immediate validation

- Expanded Python concurrency suite: 69 tests, 2 skips, passed under both TSan and ASan with natural GIL-off.
- Native admission suite: all 7 tests passed for 500 repetitions under each sanitizer (7,000 native test executions total).
- Runtime implementation and pinned wheel remain unchanged; this revision changes tests and review documentation only.
