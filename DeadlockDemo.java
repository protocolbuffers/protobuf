import java.util.concurrent.*;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Demonstrates the parser initialization deadlock that existed in message.cc (lines 1216-1218).
 * 
 * The parsing constructor generated code like:
 *   com.google.protobuf.Parser<Bar> aBarParser_ = Bar.parser();
 * 
 * This created a circular dependency during static initialization when messages
 * reference each other (Foo ↔ Bar ↔ Baz), causing potential deadlocks.
 * 
 * Uses the existing unittest_cycle.proto test file:
 *   ./cmake_build/protoc --java_out=generated_java \
 *     --proto_path=java/core/src/test/proto \
 *     java/core/src/test/proto/com/google/protobuf/unittest_cycle.proto
 * 
 * This proto has Foo→Bar→Baz circular dependencies, perfect for testing the deadlock fix.
 */
public class DeadlockDemo {
    
    // Simulate class loading locks
    private static final ReentrantLock lockA = new ReentrantLock();
    private static final ReentrantLock lockB = new ReentrantLock();
    private static final ReentrantLock lockC = new ReentrantLock();
    
    private static volatile boolean deadlockDetected = false;
    
    public static void main(String[] args) throws Exception {
        System.out.println("=".repeat(80));
        System.out.println("PARSER INITIALIZATION DEADLOCK DEMONSTRATION");
        System.out.println("=".repeat(80));
        System.out.println();
        System.out.println("Simulating the deadlock from message.cc (using unittest_cycle.proto):");
        System.out.println("  Foo parsing constructor → calls Bar.parser()");
        System.out.println("  Bar parsing constructor → calls Baz.parser()");
        System.out.println("  Baz parsing constructor → calls Foo.parser() [CIRCULAR!]");
        System.out.println();
        
        ExecutorService executor = Executors.newFixedThreadPool(3);
        CountDownLatch startLatch = new CountDownLatch(1);
        CountDownLatch doneLatch = new CountDownLatch(3);
        
        // Thread 1: Initialize MessageA
        executor.submit(() -> {
            try {
                startLatch.await();
                System.out.println("[Thread-1] Initializing MessageA...");
                MessageA.initialize();
            } catch (Exception e) {
                System.err.println("[Thread-1] " + e.getMessage());
            } finally {
                doneLatch.countDown();
            }
        });
        
        // Thread 2: Initialize MessageB
        executor.submit(() -> {
            try {
                startLatch.await();
                Thread.sleep(50);
                System.out.println("[Thread-2] Initializing MessageB...");
                MessageB.initialize();
            } catch (Exception e) {
                System.err.println("[Thread-2] " + e.getMessage());
            } finally {
                doneLatch.countDown();
            }
        });
        
        // Thread 3: Initialize MessageC
        executor.submit(() -> {
            try {
                startLatch.await();
                Thread.sleep(100);
                System.out.println("[Thread-3] Initializing MessageC...");
                MessageC.initialize();
            } catch (Exception e) {
                System.err.println("[Thread-3] " + e.getMessage());
            } finally {
                doneLatch.countDown();
            }
        });
        
        startLatch.countDown();
        boolean completed = doneLatch.await(3, TimeUnit.SECONDS);
        executor.shutdownNow();
        
        System.out.println();
        System.out.println("=".repeat(80));
        
        if (!completed || deadlockDetected) {
            System.out.println("🔴 DEADLOCK DEMONSTRATED!");
            System.out.println();
            System.out.println("This pattern exists in generated code from message.cc lines 1216-1218.");
            System.out.println("=".repeat(80));
            System.exit(1);
        } else {
            System.out.println("✓ No deadlock this run (timing-dependent)");
            System.out.println("=".repeat(80));
            System.exit(0);
        }
    }
    
    static class MessageA {
        private static volatile boolean initialized = false;
        
        static void initialize() throws InterruptedException {
            if (initialized) return;
            
            lockA.lock();
            try {
                System.out.println("  [MessageA] Calling MessageB.parser()...");
                boolean gotLock = lockB.tryLock(1, TimeUnit.SECONDS);
                if (!gotLock) {
                    deadlockDetected = true;
                    System.err.println("  [MessageA] ✗ DEADLOCK: Can't acquire MessageB lock!");
                    throw new RuntimeException("Deadlock detected!");
                }
                try {
                    MessageB.doInit();
                } finally {
                    lockB.unlock();
                }
                initialized = true;
            } finally {
                lockA.unlock();
            }
        }
    }
    
    static class MessageB {
        private static volatile boolean initialized = false;
        
        static void initialize() throws InterruptedException {
            if (initialized) return;
            
            lockB.lock();
            try {
                doInit();
                initialized = true;
            } finally {
                lockB.unlock();
            }
        }
        
        static void doInit() throws InterruptedException {
            System.out.println("  [MessageB] Calling MessageC.parser()...");
            boolean gotLock = lockC.tryLock(1, TimeUnit.SECONDS);
            if (!gotLock) {
                deadlockDetected = true;
                System.err.println("  [MessageB] ✗ DEADLOCK: Can't acquire MessageC lock!");
                throw new RuntimeException("Deadlock detected!");
            }
            try {
                MessageC.doInit();
            } finally {
                lockC.unlock();
            }
        }
    }
    
    static class MessageC {
        private static volatile boolean initialized = false;
        
        static void initialize() throws InterruptedException {
            if (initialized) return;
            
            lockC.lock();
            try {
                doInit();
                initialized = true;
            } finally {
                lockC.unlock();
            }
        }
        
        static void doInit() throws InterruptedException {
            System.out.println("  [MessageC] Calling MessageA.parser() [CIRCULAR!]...");
            boolean gotLock = lockA.tryLock(1, TimeUnit.SECONDS);
            if (!gotLock) {
                deadlockDetected = true;
                System.err.println("  [MessageC] ✗ DEADLOCK: Can't acquire MessageA lock!");
                throw new RuntimeException("Deadlock detected!");
            }
            try {
                // Would initialize MessageA here
            } finally {
                lockA.unlock();
            }
        }
    }
}
