// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "Conformance.pbobjc.h"
#import "google/protobuf/TestMessagesProto2.pbobjc.h"
#import "google/protobuf/TestMessagesProto3.pbobjc.h"

static void Die(NSString *format, ...) __dead2;

static BOOL verbose = NO;
static int32_t testCount = 0;

static void Die(NSString *format, ...) {
  va_list args;
  va_start(args, format);
  NSString *msg = [[NSString alloc] initWithFormat:format arguments:args];
  NSLog(@"%@", msg);
  va_end(args);
  [msg release];
  exit(66);
}

static NSData *CheckedReadDataOfLength(NSFileHandle *handle, NSUInteger numBytes) {
  NSData *data = [handle readDataOfLength:numBytes];
  NSUInteger dataLen = data.length;
  if (dataLen == 0) {
    return nil;  // EOF.
  }
  if (dataLen != numBytes) {
    Die(@"Failed to read the request length (%d), only got: %@", numBytes, data);
  }
  return data;
}

static ConformanceResponse *DoTest(ConformanceRequest *request) {
  ConformanceResponse *response = [ConformanceResponse message];
  GPBMessage *testMessage = nil;

  switch (request.payloadOneOfCase) {
    case ConformanceRequest_Payload_OneOfCase_GPBUnsetOneOfCase:
      response.runtimeError =
          [NSString stringWithFormat:@"Request didn't have a payload: %@", request];
      break;

    case ConformanceRequest_Payload_OneOfCase_ProtobufPayload: {
      Class msgClass = nil;
      if ([request.messageType isEqual:@"protobuf_test_messages.proto3.TestAllTypesProto3"]) {
        msgClass = [Proto3TestAllTypesProto3 class];
      } else if ([request.messageType
                     isEqual:@"protobuf_test_messages.proto2.TestAllTypesProto2"]) {
        msgClass = [Proto2TestAllTypesProto2 class];
      } else {
        response.runtimeError =
            [NSString stringWithFormat:@"Protobuf request had an unknown message_type: %@",
                                       request.messageType];
        break;
      }
      NSError *error = nil;
      testMessage = [msgClass parseFromData:request.protobufPayload error:&error];
      if (!testMessage) {
        response.parseError = [NSString stringWithFormat:@"Parse error: %@", error];
      }
      break;
    }

    case ConformanceRequest_Payload_OneOfCase_JsonPayload:
      response.skipped = @"ObjC doesn't support parsing JSON";
      break;

    case ConformanceRequest_Payload_OneOfCase_JspbPayload:
      response.skipped = @"ConformanceRequest had a jspb_payload ConformanceRequest.payload;"
                          " those aren't supposed to happen with opensource.";
      break;

    case ConformanceRequest_Payload_OneOfCase_TextPayload:
      response.skipped = @"ObjC doesn't support parsing TextFormat";
      break;
  }

  if (testMessage) {
    switch (request.requestedOutputFormat) {
      case ConformanceWireFormat_GPBUnrecognizedEnumeratorValue:
      case ConformanceWireFormat_Unspecified:
        response.runtimeError =
            [NSString stringWithFormat:@"Unrecognized/unspecified output format: %@", request];
        break;

      case ConformanceWireFormat_Protobuf:
        response.protobufPayload = testMessage.data;
        if (!response.protobufPayload) {
          response.serializeError =
              [NSString stringWithFormat:@"Failed to make data from: %@", testMessage];
        }
        break;

      case ConformanceWireFormat_Json:
        response.skipped = @"ObjC doesn't support generating JSON";
        break;

      case ConformanceWireFormat_Jspb:
        response.skipped =
            @"ConformanceRequest had a requested_output_format of JSPB WireFormat; that"
             " isn't supposed to happen with opensource.";
        break;

      case ConformanceWireFormat_TextFormat:
        // ObjC only has partial objc generation, so don't attempt any tests that need
        // support.
        response.skipped = @"ObjC doesn't support generating TextFormat";
        break;
    }
  }

  return response;
}

static uint32_t UInt32FromLittleEndianData(NSData *data) {
  if (data.length != sizeof(uint32_t)) {
    Die(@"Data not the right size for uint32_t: %@", data);
  }
  uint32_t value;
  memcpy(&value, data.bytes, sizeof(uint32_t));
  return CFSwapInt32LittleToHost(value);
}

static NSData *UInt32ToLittleEndianData(uint32_t num) {
  uint32_t value = CFSwapInt32HostToLittle(num);
  return [NSData dataWithBytes:&value length:sizeof(uint32_t)];
}

static BOOL DoTestIo(NSFileHandle *input, NSFileHandle *output) {
  // See conformance_test_runner.cc for the wire format.
  NSData *data = CheckedReadDataOfLength(input, sizeof(uint32_t));
  if (!data) {
    // EOF.
    return NO;
  }
  uint32_t numBytes = UInt32FromLittleEndianData(data);
  data = CheckedReadDataOfLength(input, numBytes);
  if (!data) {
    Die(@"Failed to read request");
  }

  NSError *error = nil;
  ConformanceRequest *request = [ConformanceRequest parseFromData:data error:&error];
  if (!request) {
    Die(@"Failed to parse the message data: %@", error);
  }

  ConformanceResponse *response = DoTest(request);
  if (!response) {
    Die(@"Failed to make a reply from %@", request);
  }

  data = response.data;
  [output writeData:UInt32ToLittleEndianData((int32_t)data.length)];
  [output writeData:data];

  if (verbose) {
    NSLog(@"Request: %@", request);
    NSLog(@"Response: %@", response);
  }

  ++testCount;
  return YES;
}

int main(int argc, const char *argv[]) {
  @autoreleasepool {
    NSFileHandle *input = [[NSFileHandle fileHandleWithStandardInput] retain];
    NSFileHandle *output = [[NSFileHandle fileHandleWithStandardOutput] retain];

    BOOL notDone = YES;
    while (notDone) {
      @autoreleasepool {
        notDone = DoTestIo(input, output);
      }
    }

    NSLog(@"Received EOF from test runner after %d tests, exiting.", testCount);
  }
  return 0;
}
