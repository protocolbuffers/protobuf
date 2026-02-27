// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import protobuf_unittest.UnittestCycle;
import proto2_unittest.UnittestProto;
import proto2_unittest.UnittestProto.TestAllTypes;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class ConcurrentDescriptorsTest {
  public static final int N = 1000;

  static class Worker implements Runnable {
    private final CountDownLatch startSignal;
    private final CountDownLatch doneSignal;
    private final Runnable trigger;

    Worker(CountDownLatch startSignal, CountDownLatch doneSignal, Runnable trigger) {
      this.startSignal = startSignal;
      this.doneSignal = doneSignal;
      this.trigger = trigger;
    }

    @Override
    public void run() {
      try {
        startSignal.await();
        trigger.run();
      } catch (InterruptedException | RuntimeException e) {
        doneSignal.countDown();
        throw new RuntimeException(e); // Rethrow for main thread to handle
      }
      doneSignal.countDown();
    }
  }

  @Test
  public void testSimultaneousStaticInit() throws InterruptedException {
    ExecutorService executor = Executors.newFixedThreadPool(N);
    CountDownLatch startSignal = new CountDownLatch(1);
    CountDownLatch doneSignal = new CountDownLatch(N);
    List<Future<?>> futures = new ArrayList<>(N);
    for (int i = 0; i < N; i++) {
      Future<?> future =
          executor.submit(
              new Worker(
                  startSignal,
                  doneSignal,
                  // Static method invocation triggers static initialization.
                  () -> Assert.assertNotNull(UnittestProto.getDescriptor())));
      futures.add(future);
    }
    startSignal.countDown();
    doneSignal.await();
    System.out.println("Done with all threads...");
    for (int i = 0; i < futures.size(); i++) {
      try {
        futures.get(i).get();
      } catch (ExecutionException e) {
        Assert.fail("Thread " + i + " failed with:" + e.getMessage());
      }
    }
    executor.shutdown();
  }

  @Test
  public void testSimultaneousFeatureAccess() throws InterruptedException {
    ExecutorService executor = Executors.newFixedThreadPool(N);
    CountDownLatch startSignal = new CountDownLatch(1);
    CountDownLatch doneSignal = new CountDownLatch(N);
    List<Future<?>> futures = new ArrayList<>(N);
    for (int i = 0; i < N; i++) {
      Future<?> future =
          executor.submit(
              new Worker(
                  startSignal,
                  doneSignal,
                  // hasPresence() uses the [field_presence] feature.
                  () ->
                      Assert.assertTrue(
                          TestAllTypes.getDescriptor()
                              .findFieldByName("optional_int32")
                              .hasPresence())));
      futures.add(future);
    }
    startSignal.countDown();
    doneSignal.await();
    System.out.println("Done with all threads...");
    for (int i = 0; i < futures.size(); i++) {
      try {
        futures.get(i).get();
      } catch (ExecutionException e) {
        Assert.fail("Thread " + i + " failed with:" + e.getMessage());
      }
    }
    executor.shutdown();
  }

  /**
   * Tests parser initialization with circular message dependencies.
   *
   * <p>Verifies that lazy parser initialization (setting parser variables to null and initializing
   * on first use) prevents deadlocks when messages have circular dependencies (e.g., Foo → Bar →
   * Baz → Foo).
   *
   * <p>This test uses unittest_cycle.proto which defines messages with circular references. Before
   * the fix, eager parser initialization could cause deadlocks during concurrent class loading.
   */
  @Test
  public void testParserInitializationWithCircularDependencies() throws InterruptedException {
    // Use fewer threads (10) since we're testing a specific deadlock scenario
    // rather than stress-testing
    int numThreads = 10;
    ExecutorService executor = Executors.newFixedThreadPool(numThreads);
    CountDownLatch startSignal = new CountDownLatch(1);
    CountDownLatch doneSignal = new CountDownLatch(numThreads);
    List<Future<?>> futures = new ArrayList<>(numThreads);

    // Test concurrent access to parser() methods of circularly dependent messages
    for (int i = 0; i < numThreads; i++) {
      final int threadNum = i;
      Future<?> future =
          executor.submit(
              new Worker(
                  startSignal,
                  doneSignal,
                  () -> {
                    // Access parsers in round-robin to maximize chance of catching deadlock
                    switch (threadNum % 3) {
                      case 0:
                        Assert.assertNotNull(UnittestCycle.Foo.parser());
                        break;
                      case 1:
                        Assert.assertNotNull(UnittestCycle.Bar.parser());
                        break;
                      case 2:
                        Assert.assertNotNull(UnittestCycle.Baz.parser());
                        break;
                    }
                  }));
      futures.add(future);
    }

    startSignal.countDown();
    doneSignal.await();
    System.out.println("Done with all parser initialization threads...");

    for (int i = 0; i < futures.size(); i++) {
      try {
        futures.get(i).get();
      } catch (ExecutionException e) {
        Assert.fail("Thread " + i + " failed with:" + e.getMessage());
      }
    }
    executor.shutdown();
  }
}
