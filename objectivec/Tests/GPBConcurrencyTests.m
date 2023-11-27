// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"

#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestObjc.pbobjc.h"

static const int kNumThreads = 100;
static const int kNumMessages = 100;

// NOTE: Most of these tests don't "fail" in the sense that the XCTAsserts
// trip.  Rather, the asserts simply exercise the apis, and if there is
// a concurancy issue, the NSAsserts in the runtime code fire and/or the
// code just crashes outright.

@interface ConcurrencyTests : GPBTestCase
@end

@implementation ConcurrencyTests

- (NSArray *)createThreadsWithSelector:(SEL)selector object:(id)object {
  NSMutableArray *array = [NSMutableArray array];
  for (NSUInteger i = 0; i < kNumThreads; i++) {
    NSThread *thread = [[NSThread alloc] initWithTarget:self selector:selector object:object];
    [array addObject:thread];
    [thread release];
  }
  return array;
}

- (NSArray *)createMessagesWithType:(Class)msgType {
  NSMutableArray *array = [NSMutableArray array];
  for (NSUInteger i = 0; i < kNumMessages; i++) {
    [array addObject:[msgType message]];
  }
  return array;
}

- (void)startThreads:(NSArray *)threads {
  for (NSThread *thread in threads) {
    [thread start];
  }
}

- (void)joinThreads:(NSArray *)threads {
  for (NSThread *thread in threads) {
    while (![thread isFinished])
      ;
  }
}

- (void)readForeignMessage:(NSArray *)messages {
  for (NSUInteger i = 0; i < 10; i++) {
    for (TestAllTypes *message in messages) {
      XCTAssertEqual(message.optionalForeignMessage.c, 0);
    }
  }
}

- (void)testConcurrentReadOfUnsetMessageField {
  NSArray *messages = [self createMessagesWithType:[TestAllTypes class]];
  NSArray *threads = [self createThreadsWithSelector:@selector(readForeignMessage:)
                                              object:messages];
  [self startThreads:threads];
  [self joinThreads:threads];
  for (TestAllTypes *message in messages) {
    XCTAssertFalse(message.hasOptionalForeignMessage);
  }
}

- (void)readRepeatedInt32:(NSArray *)messages {
  for (int i = 0; i < 10; i++) {
    for (TestAllTypes *message in messages) {
      XCTAssertEqual([message.repeatedInt32Array count], (NSUInteger)0);
    }
  }
}

- (void)testConcurrentReadOfUnsetRepeatedIntField {
  NSArray *messages = [self createMessagesWithType:[TestAllTypes class]];
  NSArray *threads = [self createThreadsWithSelector:@selector(readRepeatedInt32:) object:messages];
  [self startThreads:threads];
  [self joinThreads:threads];
  for (TestAllTypes *message in messages) {
    XCTAssertEqual([message.repeatedInt32Array count], (NSUInteger)0);
  }
}

- (void)readRepeatedString:(NSArray *)messages {
  for (int i = 0; i < 10; i++) {
    for (TestAllTypes *message in messages) {
      XCTAssertEqual([message.repeatedStringArray count], (NSUInteger)0);
    }
  }
}

- (void)testConcurrentReadOfUnsetRepeatedStringField {
  NSArray *messages = [self createMessagesWithType:[TestAllTypes class]];
  NSArray *threads = [self createThreadsWithSelector:@selector(readRepeatedString:)
                                              object:messages];
  [self startThreads:threads];
  [self joinThreads:threads];
  for (TestAllTypes *message in messages) {
    XCTAssertEqual([message.repeatedStringArray count], (NSUInteger)0);
  }
}

- (void)readInt32Int32Map:(NSArray *)messages {
  for (int i = 0; i < 10; i++) {
    for (TestRecursiveMessageWithRepeatedField *message in messages) {
      XCTAssertEqual([message.iToI count], (NSUInteger)0);
    }
  }
}

- (void)testConcurrentReadOfUnsetInt32Int32MapField {
  NSArray *messages = [self createMessagesWithType:[TestRecursiveMessageWithRepeatedField class]];
  NSArray *threads = [self createThreadsWithSelector:@selector(readInt32Int32Map:) object:messages];
  [self startThreads:threads];
  [self joinThreads:threads];
  for (TestRecursiveMessageWithRepeatedField *message in messages) {
    XCTAssertEqual([message.iToI count], (NSUInteger)0);
  }
}

- (void)readStringStringMap:(NSArray *)messages {
  for (int i = 0; i < 10; i++) {
    for (TestRecursiveMessageWithRepeatedField *message in messages) {
      XCTAssertEqual([message.strToStr count], (NSUInteger)0);
    }
  }
}

- (void)testConcurrentReadOfUnsetStringStringMapField {
  NSArray *messages = [self createMessagesWithType:[TestRecursiveMessageWithRepeatedField class]];
  NSArray *threads = [self createThreadsWithSelector:@selector(readStringStringMap:)
                                              object:messages];
  [self startThreads:threads];
  [self joinThreads:threads];
  for (TestRecursiveMessageWithRepeatedField *message in messages) {
    XCTAssertEqual([message.strToStr count], (NSUInteger)0);
  }
}

- (void)readOptionalForeignMessageExtension:(NSArray *)messages {
  for (int i = 0; i < 10; i++) {
    for (TestAllExtensions *message in messages) {
      ForeignMessage *foreign =
          [message getExtension:[UnittestRoot optionalForeignMessageExtension]];
      XCTAssertEqual(foreign.c, 0);
    }
  }
}

- (void)testConcurrentReadOfUnsetExtensionField {
  NSArray *messages = [self createMessagesWithType:[TestAllExtensions class]];
  SEL sel = @selector(readOptionalForeignMessageExtension:);
  NSArray *threads = [self createThreadsWithSelector:sel object:messages];
  [self startThreads:threads];
  [self joinThreads:threads];
  GPBExtensionDescriptor *extension = [UnittestRoot optionalForeignMessageExtension];
  for (TestAllExtensions *message in messages) {
    XCTAssertFalse([message hasExtension:extension]);
  }
}

@end
