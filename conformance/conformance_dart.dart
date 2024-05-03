import 'dart:io';

import 'package:pb_runtime/ffi/bytes.dart';
import 'package:pb_runtime/pb_runtime.dart' as pb;
import 'package:third_party.protobuf/test_messages_proto2.upb.dart';
import 'package:third_party.protobuf/test_messages_proto3.upb.dart';
import 'package:third_party.protobuf.conformance/conformance.upb.dart';

List<int>? readFromStdin(int numBytes) {
  final bytesOut = <int>[];
  for (var i = 0; i < numBytes; i++) {
    final byte = stdin.readByteSync();
    if (byte == -1) return null;
    bytesOut.add(byte);
  }
  return bytesOut;
}

int readLittleEndianIntFromStdin() {
  final buf = readFromStdin(4);
  if (buf == null) return -1;
  return (buf[0] & 0xff) |
      ((buf[1] & 0xff) << 8) |
      ((buf[2] & 0xff) << 16) |
      ((buf[3] & 0xff) << 24);
}

void writeLittleEndianIntToStdout(int value) {
  final buf = <int>[];
  buf.add(value & 0xff);
  buf.add((value >> 8) & 0xff);
  buf.add((value >> 16) & 0xff);
  buf.add((value >> 24) & 0xff);
  stdout.add(buf);
}

ConformanceResponse doTest(ConformanceRequest request) {
  final response = ConformanceResponse();
  pb.GeneratedMessage? testMessage;
  final messageType = request.messageType;
  final isProto3 =
      messageType == 'protobuf_test_messages.proto3.TestAllTypesProto3';

  if (!isProto3 &&
      messageType != 'protobuf_test_messages.proto2.TestAllTypesProto2') {
    throw ArgumentError('Invalid message type $messageType');
  }

  switch (request.whichPayload) {
    case ConformanceRequest_payload.protobufPayload:
      try {
        testMessage = isProto3
            ? TestAllTypesProto3.fromBinary(request.protobufPayload.data)
            : TestAllTypesProto2.fromBinary(request.protobufPayload.data);
      } catch (e) {
        final parseErrorResponse = ConformanceResponse();
        parseErrorResponse.parseError = '$e';
        return parseErrorResponse;
      }
      break;
    default:
      response.skipped = 'Only protobuf payload input is supported';
      return response;
  }

  switch (request.requestedOutputFormat) {
    case WireFormat.PROTOBUF:
      try {
        response.protobufPayload =
            Bytes(pb.GeneratedMessage.toBinary(testMessage));
      } catch (e) {
        response.serializeError = '$e';
      }
      break;
    default:
      response.skipped = 'Only protobuf payload output is supported';
  }

  return response;
}

Future<bool> doTestIo() async {
  final msgLength = readLittleEndianIntFromStdin();
  if (msgLength == -1) return false; // EOF
  final serializedMsg = readFromStdin(msgLength);
  if (serializedMsg == null) {
    throw 'Unexpected EOF from test program.';
  }
  final request = ConformanceRequest.fromBinary(serializedMsg);
  final response = doTest(request);
  final serializedOutput = pb.GeneratedMessage.toBinary(response);
  writeLittleEndianIntToStdout(serializedOutput.length);
  stdout.add(serializedOutput);
  await stdout.flush();
  return true;
}

void main() async {
  await pb.initGlobalInstance();
  var testCount = 0;
  while (await doTestIo()) {
    testCount++;
  }
  stderr.writeln(
      'ConformanceDart: received EOF from test runner after $testCount tests');
}
