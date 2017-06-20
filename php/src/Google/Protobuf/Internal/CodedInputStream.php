<?php

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

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\Uint64;

class CodedInputStream
{

    private $buffer;
    private $buffer_size_after_limit;
    private $buffer_end;
    private $current;
    private $current_limit;
    private $legitimate_message_end;
    private $recursion_budget;
    private $recursion_limit;
    private $total_bytes_limit;
    private $total_bytes_read;

    const MAX_VARINT_BYTES = 10;
    const DEFAULT_RECURSION_LIMIT = 100;
    const DEFAULT_TOTAL_BYTES_LIMIT = 33554432; // 32 << 20, 32MB

    public function __construct($buffer)
    {
        $start = 0;
        $end = strlen($buffer);
        $this->buffer = $buffer;
        $this->buffer_size_after_limit = 0;
        $this->buffer_end = $end;
        $this->current = $start;
        $this->current_limit = $end;
        $this->legitimate_message_end = false;
        $this->recursion_budget = self::DEFAULT_RECURSION_LIMIT;
        $this->recursion_limit = self::DEFAULT_RECURSION_LIMIT;
        $this->total_bytes_limit = self::DEFAULT_TOTAL_BYTES_LIMIT;
        $this->total_bytes_read = $end - $start;
    }

    private function advance($amount)
    {
        $this->current += $amount;
    }

    public function bufferSize()
    {
        return $this->buffer_end - $this->current;
    }

    private function current()
    {
        return $this->total_bytes_read -
            ($this->buffer_end - $this->current +
            $this->buffer_size_after_limit);
    }

    private function recomputeBufferLimits()
    {
        $this->buffer_end += $this->buffer_size_after_limit;
        $closest_limit = min($this->current_limit, $this->total_bytes_limit);
        if ($closest_limit < $this->total_bytes_read) {
            // The limit position is in the current buffer.  We must adjust the
            // buffer size accordingly.
            $this->buffer_size_after_limit = $this->total_bytes_read -
                $closest_limit;
            $this->buffer_end -= $this->buffer_size_after_limit;
        } else {
            $this->buffer_size_after_limit = 0;
        }
    }

    private function consumedEntireMessage()
    {
        return $this->legitimate_message_end;
    }

    /**
     * Read uint32 into $var. Advance buffer with consumed bytes. If the
     * contained varint is larger than 32 bits, discard the high order bits.
     * @param $var.
     */
    public function readVarint32(&$var)
    {
        if (!$this->readVarint64($var)) {
            return false;
        }

        if (PHP_INT_SIZE == 4) {
            $var = bcmod($var, 4294967296);
        } else {
            $var &= 0xFFFFFFFF;
        }

        // Convert large uint32 to int32.
        if ($var > 0x7FFFFFFF) {
            if (PHP_INT_SIZE === 8) {
                $var = $var | (0xFFFFFFFF << 32);
            } else {
                $var = bcsub($var, 4294967296);
            }
        }

        $var = intval($var);
        return true;
    }

    /**
     * Read Uint64 into $var. Advance buffer with consumed bytes.
     * @param $var.
     */
    public function readVarint64(&$var)
    {
        $count = 0;

        if (PHP_INT_SIZE == 4) {
            $high = 0;
            $low = 0;
            $b = 0;

            do {
                if ($this->current === $this->buffer_end) {
                    return false;
                }
                if ($count === self::MAX_VARINT_BYTES) {
                    return false;
                }
                $b = ord($this->buffer[$this->current]);
                $bits = 7 * $count;
                if ($bits >= 32) {
                    $high |= (($b & 0x7F) << ($bits - 32));
                } else if ($bits > 25){
                    // $bits is 28 in this case.
                    $low |= (($b & 0x7F) << 28);
                    $high = ($b & 0x7F) >> 4;
                } else {
                    $low |= (($b & 0x7F) << $bits);
                }

                $this->advance(1);
                $count += 1;
            } while ($b & 0x80);

            $var = GPBUtil::combineInt32ToInt64($high, $low);
            if (bccomp($var, 0) < 0) {
                $var = bcadd($var, "18446744073709551616");
            }
        } else {
            $result = 0;
            $shift = 0;

            do {
                if ($this->current === $this->buffer_end) {
                    return false;
                }
                if ($count === self::MAX_VARINT_BYTES) {
                    return false;
                }

                $byte = ord($this->buffer[$this->current]);
                $result |= ($byte & 0x7f) << $shift;
                $shift += 7;
                $this->advance(1);
                $count += 1;
            } while ($byte > 0x7f);

            $var = $result;
        }

        return true;
    }

    /**
     * Read int into $var. If the result is larger than the largest integer, $var
     * will be -1. Advance buffer with consumed bytes.
     * @param $var.
     */
    public function readVarintSizeAsInt(&$var)
    {
        if (!$this->readVarint64($var)) {
            return false;
        }
        $var = (int)$var;
        return true;
    }

    /**
     * Read 32-bit unsiged integer to $var. If the buffer has less than 4 bytes,
     * return false. Advance buffer with consumed bytes.
     * @param $var.
     */
    public function readLittleEndian32(&$var)
    {
        $data = null;
        if (!$this->readRaw(4, $data)) {
            return false;
        }
        $var = unpack('V', $data);
        $var = $var[1];
        return true;
    }

    /**
     * Read 64-bit unsiged integer to $var. If the buffer has less than 8 bytes,
     * return false. Advance buffer with consumed bytes.
     * @param $var.
     */
    public function readLittleEndian64(&$var)
    {
        $data = null;
        if (!$this->readRaw(4, $data)) {
            return false;
        }
        $low = unpack('V', $data)[1];
        if (!$this->readRaw(4, $data)) {
            return false;
        }
        $high = unpack('V', $data)[1];
        if (PHP_INT_SIZE == 4) {
            $var = GPBUtil::combineInt32ToInt64($high, $low);
        } else {
            $var = ($high << 32) | $low;
        }
        return true;
    }

    /**
     * Read tag into $var. Advance buffer with consumed bytes.
     * @param $var.
     */
    public function readTag()
    {
        if ($this->current === $this->buffer_end) {
            // Make sure that it failed due to EOF, not because we hit
            // total_bytes_limit, which, unlike normal limits, is not a valid
            // place to end a message.
            $current_position = $this->total_bytes_read -
                $this->buffer_size_after_limit;
            if ($current_position >= $this->total_bytes_limit) {
                // Hit total_bytes_limit_.  But if we also hit the normal limit,
                // we're still OK.
                $this->legitimate_message_end =
                    ($this->current_limit === $this->total_bytes_limit);
            } else {
                $this->legitimate_message_end = true;
            }
            return 0;
        }

        $result = 0;
        // The larget tag is 2^29 - 1, which can be represented by int32.
        $success = $this->readVarint32($result);
        if ($success) {
            return $result;
        } else {
            return 0;
        }
    }

    public function readRaw($size, &$buffer)
    {
        $current_buffer_size = 0;
        if ($this->bufferSize() < $size) {
            return false;
        }

        $buffer = substr($this->buffer, $this->current, $size);
        $this->advance($size);

        return true;
    }

    /* Places a limit on the number of bytes that the stream may read, starting
     * from the current position.  Once the stream hits this limit, it will act
     * like the end of the input has been reached until popLimit() is called.
     *
     * As the names imply, the stream conceptually has a stack of limits.  The
     * shortest limit on the stack is always enforced, even if it is not the top
     * limit.
     *
     * The value returned by pushLimit() is opaque to the caller, and must be
     * passed unchanged to the corresponding call to popLimit().
     *
     * @param integer $byte_limit
     * @throws Exception Fail to push limit.
     */
    public function pushLimit($byte_limit)
    {
        // Current position relative to the beginning of the stream.
        $current_position = $this->current();
        $old_limit = $this->current_limit;

        // security: byte_limit is possibly evil, so check for negative values
        // and overflow.
        if ($byte_limit >= 0 &&
            $byte_limit <= PHP_INT_MAX - $current_position &&
            $byte_limit <= $this->current_limit - $current_position) {
            $this->current_limit = $current_position + $byte_limit;
            $this->recomputeBufferLimits();
        } else {
            throw new GPBDecodeException("Fail to push limit.");
        }

        return $old_limit;
    }

    /* The limit passed in is actually the *old* limit, which we returned from
     * PushLimit().
     *
     * @param integer $byte_limit
     */
    public function popLimit($byte_limit)
    {
        $this->current_limit = $byte_limit;
        $this->recomputeBufferLimits();
        // We may no longer be at a legitimate message end.  ReadTag() needs to
        // be called again to find out.
        $this->legitimate_message_end = false;
    }

    public function incrementRecursionDepthAndPushLimit(
        $byte_limit, &$old_limit, &$recursion_budget)
    {
        $old_limit = $this->pushLimit($byte_limit);
        $recursion_limit = --$this->recursion_limit;
    }

    public function decrementRecursionDepthAndPopLimit($byte_limit)
    {
        $result = $this->consumedEntireMessage();
        $this->popLimit($byte_limit);
        ++$this->recursion_budget;
        return $result;
    }

    public function bytesUntilLimit()
    {
        if ($this->current_limit === PHP_INT_MAX) {
            return -1;
        }
        return $this->current_limit - $this->current;
    }
}
