// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.UnittestLite.TestAllTypesLite;
import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CyclicBarrier;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Tests that the Lite runtime identifies a message type by its default instance rather than by its
 * class: {@link Protobuf}'s schema lookups, and sub-message fields whose type is given by a default
 * instance in {@code objects[]}.
 */
@RunWith(JUnit4.class)
public class DefaultInstanceKeyedLookupTest {

  /** Message types for {@link #typesSharingAClass_getTheirOwnSchemas}. */
  private static final int SEQUENTIAL_TYPES = 300;

  /** Message types for {@link #concurrentFirstLookups_agree}, never looked up elsewhere. */
  private static final int CONCURRENT_TYPES = 300;

  /**
   * Many message types sharing one class, as a whole-program optimizer may produce by merging
   * generated classes with the same field layout. Type {@code i} is {@code optional int32 value = i
   * + 1;}; each instance knows its type through {@code cid}.
   */
  private static final class SharedClassMessage
      extends GeneratedMessageLite<SharedClassMessage, SharedClassMessage.Builder> {
    private static final SharedClassMessage[] DEFAULT_INSTANCES =
        new SharedClassMessage[SEQUENTIAL_TYPES + CONCURRENT_TYPES];

    static {
      for (int i = 0; i < DEFAULT_INSTANCES.length; i++) {
        DEFAULT_INSTANCES[i] = new SharedClassMessage(i);
        DEFAULT_INSTANCES[i].markImmutable();
      }
      GeneratedMessageLite.registerDefaultInstance(SharedClassMessage.class, DEFAULT_INSTANCES[0]);
    }

    private final int cid;
    private int bitField0_;
    private int value_;

    private SharedClassMessage(int cid) {
      this.cid = cid;
    }

    static final class Builder extends GeneratedMessageLite.Builder<SharedClassMessage, Builder> {
      private Builder(SharedClassMessage defaultInstance) {
        super(defaultInstance);
      }
    }

    @Override
    protected Object dynamicMethod(MethodToInvoke method, Object arg0, Object arg1) {
      SharedClassMessage defaultInstance = DEFAULT_INSTANCES[cid];
      switch (method) {
        case NEW_MUTABLE_INSTANCE:
          return new SharedClassMessage(cid);
        case NEW_BUILDER:
          return new Builder(defaultInstance);
        case GET_DEFAULT_INSTANCE:
          return defaultInstance;
        case GET_PARSER:
          return new DefaultInstanceBasedParser<>(defaultInstance);
        case BUILD_MESSAGE_INFO:
          char number = (char) (cid + 1);
          // flags = 0x1 (PROTO2), fieldCount = 1, oneofCount = 0, hasBitsCount = 1,
          // minFieldNumber = maxFieldNumber = number, numEntries = 1, mapFieldCount = 0,
          // repeatedFieldCount = 0, checkInitialized = 0.
          // Field: number, type = 4 | 0x1000 (INT32 | HAS_HAS_BIT), hasBitIndex = 0.
          String info =
              "\u0001\u0001\u0000\u0001"
                  + number
                  + number
                  + "\u0001\u0000\u0000\u0000"
                  + number
                  + "\u1004\u0000";
          return newMessageInfo(defaultInstance, info, new Object[] {"bitField0_", "value_"});
        default:
          return null;
      }
    }

    static SharedClassMessage of(int cid, int value) throws InvalidProtocolBufferException {
      return GeneratedMessageLite.parseFrom(DEFAULT_INSTANCES[cid], encodeInt32(cid + 1, value));
    }
  }

  /**
   * A message whose sub-message fields all have type {@code SharedClassMessage} 1, which only its
   * default instance identifies: the class alone resolves to type 0.
   *
   * <pre>
   * optional SharedClassMessage1 single = 1;  // Declared as Object, so the type must come from
   *                                           // the default instance after the field name.
   * repeated SharedClassMessage1 list = 2;
   * oneof choice { SharedClassMessage1 choice_message = 3; }
   * optional int32 after = 4;
   * </pre>
   */
  private static final class Holder extends GeneratedMessageLite<Holder, Holder.Builder> {
    private static final Holder DEFAULT_INSTANCE;

    static {
      Holder defaultInstance = new Holder();
      DEFAULT_INSTANCE = defaultInstance;
      GeneratedMessageLite.registerDefaultInstance(Holder.class, defaultInstance);
    }

    private int choiceCase_ = 0;
    private Object choice_;
    private int bitField0_;
    private Object single_;
    private Internal.ProtobufList<SharedClassMessage> list_ = emptyProtobufList();
    private int after_;

    static final class Builder extends GeneratedMessageLite.Builder<Holder, Builder> {
      private Builder() {
        super(DEFAULT_INSTANCE);
      }
    }

    @Override
    protected Object dynamicMethod(MethodToInvoke method, Object arg0, Object arg1) {
      switch (method) {
        case NEW_MUTABLE_INSTANCE:
          return new Holder();
        case NEW_BUILDER:
          return new Builder();
        case GET_DEFAULT_INSTANCE:
          return DEFAULT_INSTANCE;
        case BUILD_MESSAGE_INFO:
          SharedClassMessage type1 = SharedClassMessage.DEFAULT_INSTANCES[1];
          // flags = 0x1 (PROTO2), fieldCount = 4, oneofCount = 1, hasBitsCount = 1,
          // minFieldNumber = 1, maxFieldNumber = 4, numEntries = 4, mapFieldCount = 0,
          // repeatedFieldCount = 1, checkInitialized = 0.
          // Field 1: type = 9 | 0x1000 (MESSAGE | HAS_HAS_BIT), hasBitIndex = 0.
          // Field 2: type = 27 (MESSAGE_LIST).
          // Field 3: type = 9 + 51 (oneof MESSAGE), oneofIndex = 0.
          // Field 4: type = 4 | 0x1000 (INT32 | HAS_HAS_BIT), hasBitIndex = 1.
          String info =
              "\u0001\u0004\u0001\u0001\u0001\u0004\u0004\u0000\u0001\u0000"
                  + "\u0001\u1009\u0000"
                  + "\u0002\u001b"
                  + "\u0003\u003c\u0000"
                  + "\u0004\u1004\u0001";
          Object[] objects = {
            "choice_",
            "choiceCase_",
            "bitField0_",
            "single_",
            type1,
            "list_",
            type1,
            type1,
            "after_"
          };
          return newMessageInfo(DEFAULT_INSTANCE, info, objects);
        default:
          return null;
      }
    }
  }

  @Test
  public void generatedType_allLookupsAgree() throws Exception {
    TestAllTypesLite defaultInstance = TestAllTypesLite.getDefaultInstance();
    TestAllTypesLite message = TestAllTypesLite.newBuilder().setOptionalInt32(42).build();
    Protobuf protobuf = Protobuf.getInstance();

    Schema<TestAllTypesLite> schema = protobuf.schemaForDefaultInstance(defaultInstance);
    assertThat(protobuf.schemaFor(message)).isSameInstanceAs(schema);
    assertThat(protobuf.schemaFor(defaultInstance)).isSameInstanceAs(schema);
    assertThat(protobuf.schemaFor(TestAllTypesLite.class)).isSameInstanceAs(schema);
  }

  @Test
  public void typesSharingAClass_getTheirOwnSchemas() throws Exception {
    Protobuf protobuf = Protobuf.getInstance();
    List<Schema<SharedClassMessage>> schemas = new ArrayList<>();
    // More types than the cache's initial capacity, so it grows while being filled.
    for (int i = 0; i < SEQUENTIAL_TYPES; i++) {
      SharedClassMessage defaultInstance = SharedClassMessage.DEFAULT_INSTANCES[i];
      schemas.add(protobuf.schemaForDefaultInstance(defaultInstance));
    }
    assertThat(identitySetOf(schemas)).hasSize(SEQUENTIAL_TYPES);

    for (int i = 0; i < SEQUENTIAL_TYPES; i++) {
      SharedClassMessage defaultInstance = SharedClassMessage.DEFAULT_INSTANCES[i];
      SharedClassMessage message = SharedClassMessage.of(i, 1000 + i);
      // Lookups still find the entries made before the cache grew.
      assertThat(protobuf.schemaForDefaultInstance(defaultInstance))
          .isSameInstanceAs(schemas.get(i));
      assertThat(protobuf.schemaFor(message)).isSameInstanceAs(schemas.get(i));

      // Each type serializes, parses and compares with its own schema.
      assertThat(message.getDefaultInstanceForType()).isSameInstanceAs(defaultInstance);
      assertThat(message.value_).isEqualTo(1000 + i);
      assertThat(message.toByteArray()).isEqualTo(encodeInt32(i + 1, 1000 + i));
      assertThat(defaultInstance.getParserForType().parseFrom(message.toByteArray()))
          .isEqualTo(message);
      assertThat(message.hashCode()).isEqualTo(SharedClassMessage.of(i, 1000 + i).hashCode());
    }

    // The class only identifies the type its default instance lookup reports.
    assertThat(protobuf.schemaFor(SharedClassMessage.class)).isSameInstanceAs(schemas.get(0));
    // Equal field values don't make messages of different types equal.
    assertThat(SharedClassMessage.DEFAULT_INSTANCES[1])
        .isNotEqualTo(SharedClassMessage.DEFAULT_INSTANCES[2]);
  }

  @Test
  public void concurrentFirstLookups_agree() throws Exception {
    final int threadCount = 16;
    final CyclicBarrier barrier = new CyclicBarrier(threadCount);
    final ConcurrentHashMap<Thread, Throwable> failures = new ConcurrentHashMap<>();
    final Object[][] schemas = new Object[threadCount][CONCURRENT_TYPES];
    Thread[] threads = new Thread[threadCount];
    for (int t = 0; t < threadCount; t++) {
      final int thread = t;
      threads[t] =
          new Thread() {
            @Override
            public void run() {
              try {
                barrier.await();
                // Each thread starts at a different type, so that they race to register types
                // while other threads look types up.
                for (int j = 0; j < CONCURRENT_TYPES; j++) {
                  int i = (j + thread * CONCURRENT_TYPES / threadCount) % CONCURRENT_TYPES;
                  SharedClassMessage defaultInstance =
                      SharedClassMessage.DEFAULT_INSTANCES[SEQUENTIAL_TYPES + i];
                  schemas[thread][i] =
                      Protobuf.getInstance().schemaForDefaultInstance(defaultInstance);
                }
              } catch (Throwable e) {
                failures.put(this, e);
              }
            }
          };
      threads[t].start();
    }
    for (Thread thread : threads) {
      thread.join();
    }
    for (Throwable failure : failures.values()) {
      throw new AssertionError(failure);
    }

    for (int i = 0; i < CONCURRENT_TYPES; i++) {
      SharedClassMessage defaultInstance =
          SharedClassMessage.DEFAULT_INSTANCES[SEQUENTIAL_TYPES + i];
      for (int t = 0; t < threadCount; t++) {
        assertThat(schemas[t][i]).isNotNull();
        assertThat(schemas[t][i])
            .isSameInstanceAs(Protobuf.getInstance().schemaForDefaultInstance(defaultInstance));
      }
    }
  }

  @Test
  public void submessageFields_areTypedByTheirDefaultInstance() throws Exception {
    SharedClassMessage type1 = SharedClassMessage.DEFAULT_INSTANCES[1];
    SharedClassMessage single = SharedClassMessage.of(1, 10);
    SharedClassMessage first = SharedClassMessage.of(1, 20);
    SharedClassMessage second = SharedClassMessage.of(1, 21);
    SharedClassMessage choice = SharedClassMessage.of(1, 30);

    ByteString.Output out = ByteString.newOutput();
    CodedOutputStream output = CodedOutputStream.newInstance(out);
    output.writeMessage(1, single);
    output.writeMessage(2, first);
    output.writeMessage(2, second);
    output.writeMessage(3, choice);
    output.writeInt32(4, 40);
    output.flush();
    byte[] bytes = out.toByteString().toByteArray();

    Holder holder = GeneratedMessageLite.parseFrom(Holder.DEFAULT_INSTANCE, bytes);
    assertThat(holder.single_).isEqualTo(single);
    assertThat(holder.list_).containsExactly(first, second).inOrder();
    assertThat(holder.choiceCase_).isEqualTo(3);
    assertThat(holder.choice_).isEqualTo(choice);
    assertThat(holder.after_).isEqualTo(40);
    assertThat(((SharedClassMessage) holder.single_).getDefaultInstanceForType())
        .isSameInstanceAs(type1);
    assertThat(holder.list_.get(1).getDefaultInstanceForType()).isSameInstanceAs(type1);
    assertThat(((SharedClassMessage) holder.choice_).getDefaultInstanceForType())
        .isSameInstanceAs(type1);

    assertThat(holder.toByteArray()).isEqualTo(bytes);
    assertThat(holder.getSerializedSize()).isEqualTo(bytes.length);
    Holder reparsed = GeneratedMessageLite.parseFrom(Holder.DEFAULT_INSTANCE, holder.toByteArray());
    assertThat(reparsed).isEqualTo(holder);
    assertThat(reparsed.hashCode()).isEqualTo(holder.hashCode());

    // Merging copies sub-messages with their own type's schema.
    Holder merged =
        ((Holder.Builder) Holder.DEFAULT_INSTANCE.newBuilderForType())
            .mergeFrom(holder)
            .mergeFrom(holder)
            .build();
    assertThat(merged.list_).containsExactly(first, second, first, second).inOrder();
    assertThat(((SharedClassMessage) merged.single_).getDefaultInstanceForType())
        .isSameInstanceAs(type1);
    assertThat(((SharedClassMessage) merged.choice_).getDefaultInstanceForType())
        .isSameInstanceAs(type1);
  }

  private static byte[] encodeInt32(int number, int value) {
    try {
      ByteString.Output out = ByteString.newOutput();
      CodedOutputStream output = CodedOutputStream.newInstance(out);
      output.writeInt32(number, value);
      output.flush();
      return out.toByteString().toByteArray();
    } catch (java.io.IOException e) {
      throw new AssertionError(e);
    }
  }

  private static <T> Set<T> identitySetOf(List<T> values) {
    Set<T> set = Collections.newSetFromMap(new IdentityHashMap<T, Boolean>());
    set.addAll(values);
    return set;
  }
}
