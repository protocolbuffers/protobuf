const ConformanceRequest = goog.require('proto.conformance.ConformanceRequest');
const {doTest} = goog.require('javascript.protobuf.conformance');
const fs = require('fs');


/**
 * Reads a buffer of N bytes.
 * @param {number} bytes Number of bytes to read.
 * @return {!Buffer} Buffer which contains data.
 */
function readBuffer(bytes) {
  // Linux cannot use process.stdin.fd (which isn't set up as sync)
  const buf = new Buffer.alloc(bytes);
  const fd = fs.openSync('/dev/stdin', 'r');
  fs.readSync(fd, buf, 0, bytes);
  fs.closeSync(fd);
  return buf;
}

/**
 * Writes all data in buffer.
 * @param {!Buffer} buffer Buffer which contains data.
 */
function writeBuffer(buffer) {
  // Under linux, process.stdout.fd is async. Needs to open stdout in a synced
  // way for sync write.
  const fd = fs.openSync('/dev/stdout', 'w');
  fs.writeSync(fd, buffer, 0, buffer.length);
  fs.closeSync(fd);
}

/**
 * Returns true if the test ran successfully, false on legitimate EOF.
 * @return {boolean} Whether to continue test.
 */
function runConformanceTest() {
  const requestLengthBuf = readBuffer(4);
  const requestLength = requestLengthBuf.readInt32LE(0);
  if (!requestLength) {
    return false;
  }

  const serializedRequest = readBuffer(requestLength);
  const array = new Uint8Array(serializedRequest);
  const request = ConformanceRequest.deserialize(array.buffer);
  const response = doTest(request);

  const serializedResponse = response.serialize();

  const responseLengthBuf = new Buffer.alloc(4);
  responseLengthBuf.writeInt32LE(serializedResponse.byteLength, 0);
  writeBuffer(responseLengthBuf);
  writeBuffer(new Buffer.from(serializedResponse));

  return true;
}

while (true) {
  if (!runConformanceTest()) {
    break;
  }
}
