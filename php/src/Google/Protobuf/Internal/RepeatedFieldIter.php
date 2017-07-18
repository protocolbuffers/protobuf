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

/**
 * RepeatedField and RepeatedFieldIter are used by generated protocol message
 * classes to manipulate repeated fields.
 */

namespace Google\Protobuf\Internal;

/**
 * RepeatedFieldIter is used to iterate RepeatedField. It is also need for the
 * foreach syntax.
 */
class RepeatedFieldIter implements \Iterator
{

    /**
     * @ignore
     */
    private $position;
    /**
     * @ignore
     */
    private $container;

    /**
     * Create iterator instance for RepeatedField.
     *
     * @param RepeatedField The RepeatedField instance for which this iterator
     * is created.
     * @ignore
     */
    public function __construct($container)
    {
        $this->position = 0;
        $this->container = $container;
    }

    /**
     * Reset the status of the iterator
     *
     * @return void
     */
    public function rewind()
    {
        $this->position = 0;
    }

    /**
     * Return the element at the current position.
     *
     * @return object The element at the current position.
     */
    public function current()
    {
        return $this->container[$this->position];
    }

    /**
     * Return the current position.
     *
     * @return integer The current position.
     */
    public function key()
    {
        return $this->position;
    }

    /**
     * Move to the next position.
     *
     * @return void
     */
    public function next()
    {
        ++$this->position;
    }

    /**
     * Check whether there are more elements to iterate.
     *
     * @return bool True if there are more elements to iterate.
     */
    public function valid()
    {
        return isset($this->container[$this->position]);
    }
}
