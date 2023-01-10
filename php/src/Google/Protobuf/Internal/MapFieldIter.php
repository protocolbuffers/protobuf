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
 * MapField and MapFieldIter are used by generated protocol message classes to
 * manipulate map fields.
 */

namespace Google\Protobuf\Internal;

/**
 * MapFieldIter is used to iterate MapField. It is also need for the foreach
 * syntax.
 */
class MapFieldIter implements \Iterator
{

    /**
     * @ignore
     */
    private $container;

    /**
     * @ignore
     */
    private $key_type;

    /**
     * Create iterator instance for MapField.
     *
     * @param array $container
     * @param GPBType $key_type Map key type.
     * @ignore
     */
    public function __construct($container, $key_type)
    {
        $this->container = $container;
        $this->key_type = $key_type;
    }

    /**
     * Reset the status of the iterator
     *
     * @return void
     * @todo need to add return type void (require update php version to 7.1)
     */
    #[\ReturnTypeWillChange]
    public function rewind()
    {
        reset($this->container);
    }

    /**
     * Return the element at the current position.
     *
     * @return object The element at the current position.
     * @todo need to add return type mixed (require update php version to 8.0)
     */
    #[\ReturnTypeWillChange]
    public function current()
    {
        return current($this->container);
    }

    /**
     * Return the current key.
     *
     * @return object The current key.
     * @todo need to add return type mixed (require update php version to 8.0)
     */
    #[\ReturnTypeWillChange]
    public function key()
    {
        $key = key($this->container);
        switch ($this->key_type) {
            case GPBType::INT64:
            case GPBType::UINT64:
            case GPBType::FIXED64:
            case GPBType::SFIXED64:
            case GPBType::SINT64:
                if (PHP_INT_SIZE === 8) {
                    return $key;
                }
                // Intentionally fall through
            case GPBType::STRING:
                // PHP associative array stores int string as int for key.
                return strval($key);
            case GPBType::BOOL:
                // PHP associative array stores bool as integer for key.
                return boolval($key);
            default:
                return $key;
        }
    }

    /**
     * Move to the next position.
     *
     * @return void
     * @todo need to add return type void (require update php version to 7.1)
     */
    #[\ReturnTypeWillChange]
    public function next()
    {
        next($this->container);
    }

    /**
     * Check whether there are more elements to iterate.
     *
     * @return bool True if there are more elements to iterate.
     */
    public function valid(): bool
    {
        return key($this->container) !== null;
    }
}
