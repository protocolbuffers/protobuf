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

use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\GPBUtil;

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

/**
 * RepeatedField is used by generated protocol message classes to manipulate
 * repeated fields. It can be used like native PHP array.
 */
class RepeatedField implements \ArrayAccess, \IteratorAggregate, \Countable
{

    /**
     * @ignore
     */
    private $container;
    /**
     * @ignore
     */
    private $type;
    /**
     * @ignore
     */
    private $klass;

    /**
     * Constructs an instance of RepeatedField.
     *
     * @param long $type Type of the stored element.
     * @param string $klass Message/Enum class name (message/enum fields only).
     * @ignore
     */
    public function __construct($type, $klass = null)
    {
        $this->container = [];
        $this->type = $type;
        $this->klass = $klass;
    }

    /**
     * @ignore
     */
    public function getType()
    {
        return $this->type;
    }

    /**
     * @ignore
     */
    public function getClass()
    {
        return $this->klass;
    }

    /**
     * Return the element at the given index.
     *
     * This will also be called for: $ele = $arr[0]
     *
     * @param long $offset The index of the element to be fetched.
     * @return object The stored element at given index.
     * @throws ErrorException Invalid type for index.
     * @throws ErrorException Non-existing index.
     */
    public function offsetGet($offset)
    {
        return $this->container[$offset];
    }

    /**
     * Assign the element at the given index.
     *
     * This will also be called for: $arr []= $ele and $arr[0] = ele
     *
     * @param long $offset The index of the element to be assigned.
     * @param object $value The element to be assigned.
     * @return void
     * @throws ErrorException Invalid type for index.
     * @throws ErrorException Non-existing index.
     * @throws ErrorException Incorrect type of the element.
     */
    public function offsetSet($offset, $value)
    {
        switch ($this->type) {
            case GPBType::INT32:
                GPBUtil::checkInt32($value);
                break;
            case GPBType::UINT32:
                GPBUtil::checkUint32($value);
                break;
            case GPBType::INT64:
                GPBUtil::checkInt64($value);
                break;
            case GPBType::UINT64:
                GPBUtil::checkUint64($value);
                break;
            case GPBType::FLOAT:
                GPBUtil::checkFloat($value);
                break;
            case GPBType::DOUBLE:
                GPBUtil::checkDouble($value);
                break;
            case GPBType::BOOL:
                GPBUtil::checkBool($value);
                break;
            case GPBType::STRING:
                GPBUtil::checkString($value, true);
                break;
            case GPBType::MESSAGE:
                if (is_null($value)) {
                  trigger_error("RepeatedField element cannot be null.",
                                E_USER_ERROR);
                }
                GPBUtil::checkMessage($value, $this->klass);
                break;
            default:
                break;
        }
        if (is_null($offset)) {
            $this->container[] = $value;
        } else {
            $count = count($this->container);
            if (!is_numeric($offset) || $offset < 0 || $offset >= $count) {
                trigger_error(
                    "Cannot modify element at the given index",
                    E_USER_ERROR);
                return;
            }
            $this->container[$offset] = $value;
        }
    }

    /**
     * Remove the element at the given index.
     *
     * This will also be called for: unset($arr)
     *
     * @param long $offset The index of the element to be removed.
     * @return void
     * @throws ErrorException Invalid type for index.
     * @throws ErrorException The element to be removed is not at the end of the
     * RepeatedField.
     */
    public function offsetUnset($offset)
    {
        $count = count($this->container);
        if (!is_numeric($offset) || $count === 0 || $offset !== $count - 1) {
            trigger_error(
                "Cannot remove element at the given index",
                E_USER_ERROR);
            return;
        }
        array_pop($this->container);
    }

    /**
     * Check the existence of the element at the given index.
     *
     * This will also be called for: isset($arr)
     *
     * @param long $offset The index of the element to be removed.
     * @return bool True if the element at the given offset exists.
     * @throws ErrorException Invalid type for index.
     */
    public function offsetExists($offset)
    {
        return isset($this->container[$offset]);
    }

    /**
     * @ignore
     */
    public function getIterator()
    {
        return new RepeatedFieldIter($this->container);
    }

    /**
     * Return the number of stored elements.
     *
     * This will also be called for: count($arr)
     *
     * @return integer The number of stored elements.
     */
    public function count()
    {
        return count($this->container);
    }
}
