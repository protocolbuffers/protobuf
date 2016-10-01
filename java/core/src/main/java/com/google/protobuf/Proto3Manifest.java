// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import java.nio.ByteBuffer;
import java.util.List;

/** Container for the field metadata of a single proto3 schema. */
final class Proto3Manifest {
  static final int INT_LENGTH = 4;
  static final int LONG_LENGTH = INT_LENGTH * 2;
  static final int LONGS_PER_FIELD = 2;
  /**
   * Note that field length is always a power of two so that we can use bit shifting (rather than
   * division) to find the location of a field when parsing.
   */
  static final int FIELD_LENGTH = LONGS_PER_FIELD * LONG_LENGTH;

  static final int FIELD_SHIFT = 4 /* 2^4 = 16 */;
  static final int OFFSET_BITS = 20;
  static final int OFFSET_MASK = 0XFFFFF;
  static final long EMPTY_LONG = 0xFFFFFFFFFFFFFFFFL;

  /**
   * Holds all information for accessing the message fields. The layout is as follows (field
   * positions are relative to the offset of the start of the field in the buffer):
   *
   * <p>
   *
   * <pre>
   * [ 0 -   3] unused
   * [ 4 -  31] field number
   * [32 -  37] unused
   * [38 -  43] field type
   * [44 -  63] field offset
   * [64 - 127] unused
   * </pre>
   */
  final ByteBuffer buffer;

  final long address;
  final long limit;
  final int numFields;

  final int minFieldNumber;
  final int maxFieldNumber;

  private Proto3Manifest(
      ByteBuffer buffer,
      long address,
      long limit,
      int numFields,
      int minFieldNumber,
      int maxFieldNumber) {
    this.buffer = buffer;
    this.address = address;
    this.limit = limit;
    this.numFields = numFields;
    this.minFieldNumber = minFieldNumber;
    this.maxFieldNumber = maxFieldNumber;
  }

  boolean isFieldInRange(int fieldNumber) {
    return fieldNumber >= minFieldNumber && fieldNumber <= maxFieldNumber;
  }

  long tablePositionForFieldNumber(int fieldNumber) {
    if (fieldNumber < minFieldNumber || fieldNumber > maxFieldNumber) {
      return -1;
    }

    return indexToAddress(fieldNumber - minFieldNumber);
  }

  long lookupPositionForFieldNumber(int fieldNumber) {
    int min = 0;
    int max = numFields - 1;
    while (min <= max) {
      // Find the midpoint address.
      int mid = (max + min) >>> 1;
      long midAddress = indexToAddress(mid);
      int midFieldNumber = numberAt(midAddress);
      if (fieldNumber == midFieldNumber) {
        // Found the field.
        return midAddress;
      }
      if (fieldNumber < midFieldNumber) {
        // Search the lower half.
        max = mid - 1;
      } else {
        // Search the upper half.
        min = mid + 1;
      }
    }
    return -1;
  }

  int numberAt(long pos) {
    return UnsafeUtil.getInt(pos);
  }

  int typeAndOffsetAt(long pos) {
    return UnsafeUtil.getInt(pos + INT_LENGTH);
  }

  private long indexToAddress(int index) {
    return address + (index << FIELD_SHIFT);
  }

  static byte type(int value) {
    return (byte) (value >>> OFFSET_BITS);
  }

  static long offset(int value) {
    return value & OFFSET_MASK;
  }

  static Proto3Manifest newTableManfiest(MessageInfo descriptor) {
    List<FieldInfo> fds = descriptor.getFields();
    if (fds.isEmpty()) {
      throw new IllegalArgumentException("Table-based schema requires at least one field");
    }

    // Set up the buffer for direct indexing by field number.
    final int minFieldNumber = fds.get(0).getFieldNumber();
    final int maxFieldNumber = fds.get(fds.size() - 1).getFieldNumber();
    final int numEntries = (maxFieldNumber - minFieldNumber) + 1;

    int bufferLength = numEntries * FIELD_LENGTH;
    ByteBuffer buffer = ByteBuffer.allocateDirect(bufferLength + LONG_LENGTH);
    long tempAddress = UnsafeUtil.addressOffset(buffer);
    if ((tempAddress & 7L) != 0) {
      // Make sure that the memory address is 8-byte aligned.
      tempAddress = (tempAddress & ~7L) + LONG_LENGTH;
    }
    final long address = tempAddress;
    final long limit = address + bufferLength;

    // Fill in the manifest data from the descriptors.
    int fieldIndex = 0;
    FieldInfo fd = fds.get(fieldIndex++);
    for (int bufferIndex = 0; bufferIndex < bufferLength; bufferIndex += FIELD_LENGTH) {
      final int fieldNumber = fd.getFieldNumber();
      if (bufferIndex < ((fieldNumber - minFieldNumber) << FIELD_SHIFT)) {
        // Mark this entry as "empty".
        long skipLimit = address + bufferIndex + FIELD_LENGTH;
        for (long skipPos = address + bufferIndex; skipPos < skipLimit; skipPos += LONG_LENGTH) {
          UnsafeUtil.putLong(skipPos, EMPTY_LONG);
        }
        continue;
      }

      // We found the entry for the next field. Store the entry in the manifest for
      // this field and increment the field index.
      FieldType type = fd.getType();
      UnsafeUtil.putInt(address + bufferIndex, fieldNumber);
      UnsafeUtil.putInt(
          address + bufferIndex + INT_LENGTH,
          (type.id() << OFFSET_BITS) | (int) UnsafeUtil.objectFieldOffset(fd.getField()));

      // Advance to the next field, unless we're at the end.
      if (fieldIndex < fds.size()) {
        fd = fds.get(fieldIndex++);
      }
    }

    return new Proto3Manifest(buffer, address, limit, fds.size(), minFieldNumber, maxFieldNumber);
  }

  static Proto3Manifest newLookupManifest(MessageInfo descriptor) {
    List<FieldInfo> fds = descriptor.getFields();

    final int numFields = fds.size();
    int bufferLength = numFields * FIELD_LENGTH;
    final ByteBuffer buffer = ByteBuffer.allocateDirect(bufferLength + LONG_LENGTH);
    long tempAddress = UnsafeUtil.addressOffset(buffer);
    if ((tempAddress & 7L) != 0) {
      // Make sure that the memory address is 8-byte aligned.
      tempAddress = (tempAddress & ~7L) + LONG_LENGTH;
    }
    final long address = tempAddress;
    final long limit = address + bufferLength;

    // Allocate and populate the data buffer.
    long pos = address;
    for (int i = 0; i < fds.size(); ++i, pos += FIELD_LENGTH) {
      FieldInfo fd = fds.get(i);
      UnsafeUtil.putInt(pos, fd.getFieldNumber());
      UnsafeUtil.putInt(
          pos + INT_LENGTH,
          (fd.getType().id() << OFFSET_BITS) | (int) UnsafeUtil.objectFieldOffset(fd.getField()));
    }

    if (numFields > 0) {
      return new Proto3Manifest(
          buffer,
          address,
          limit,
          numFields,
          fds.get(0).getFieldNumber(),
          fds.get(numFields - 1).getFieldNumber());
    }
    return new Proto3Manifest(buffer, address, limit, numFields, -1, -1);
  }
}
