<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/**
 * RepeatedField and RepeatedFieldIter are used by generated protocol message
 * classes to manipulate repeated fields.
 */

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\GPBUtil;
use Traversable;

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
     * @ignore
     */
    private $legacy_klass;

    /**
     * Constructs an instance of RepeatedField.
     *
     * @param integer $type Type of the stored element.
     * @param string $klass Message/Enum class name (message/enum fields only).
     * @ignore
     */
    public function __construct($type, $klass = null)
    {
        $this->container = [];
        $this->type = $type;
        if ($this->type == GPBType::MESSAGE) {
            $pool = DescriptorPool::getGeneratedPool();
            $desc = $pool->getDescriptorByClassName($klass);
            if ($desc == NULL) {
                new $klass;  // No msg class instance has been created before.
                $desc = $pool->getDescriptorByClassName($klass);
            }
            $this->klass = $desc->getClass();
            $this->legacy_klass = $desc->getLegacyClass();
        }
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
     * @ignore
     */
    public function getLegacyClass()
    {
        return $this->legacy_klass;
    }

    /**
     * Return the element at the given index.
     *
     * This will also be called for: $ele = $arr[0]
     *
     * @param integer $offset The index of the element to be fetched.
     * @return mixed The stored element at given index.
     * @throws \ErrorException Invalid type for index.
     * @throws \ErrorException Non-existing index.
     * @todo need to add return type mixed (require update php version to 8.0)
     */
    #[\ReturnTypeWillChange]
    public function offsetGet($offset)
    {
        return $this->container[$offset];
    }

    /**
     * Assign the element at the given index.
     *
     * This will also be called for: $arr []= $ele and $arr[0] = ele
     *
     * @param int|null $offset The index of the element to be assigned.
     * @param mixed $value The element to be assigned.
     * @return void
     * @throws \ErrorException Invalid type for index.
     * @throws \ErrorException Non-existing index.
     * @throws \ErrorException Incorrect type of the element.
     * @todo need to add return type void (require update php version to 7.1)
     */
    #[\ReturnTypeWillChange]
    public function offsetSet($offset, $value)
    {
        switch ($this->type) {
            case GPBType::SFIXED32:
            case GPBType::SINT32:
            case GPBType::INT32:
            case GPBType::ENUM:
                GPBUtil::checkInt32($value);
                break;
            case GPBType::FIXED32:
            case GPBType::UINT32:
                GPBUtil::checkUint32($value);
                break;
            case GPBType::SFIXED64:
            case GPBType::SINT64:
            case GPBType::INT64:
                GPBUtil::checkInt64($value);
                break;
            case GPBType::FIXED64:
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
            case GPBType::BYTES:
                GPBUtil::checkString($value, false);
                break;
            case GPBType::STRING:
                GPBUtil::checkString($value, true);
                break;
            case GPBType::MESSAGE:
                if (is_null($value)) {
                    throw new \TypeError("RepeatedField element cannot be null.");
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
     * @param integer $offset The index of the element to be removed.
     * @return void
     * @throws \ErrorException Invalid type for index.
     * @throws \ErrorException The element to be removed is not at the end of the
     * RepeatedField.
     * @todo need to add return type void (require update php version to 7.1)
     */
    #[\ReturnTypeWillChange]
    public function offsetUnset($offset)
    {
        $count = count($this->container);
        if (!is_numeric($offset) || $count === 0 || $offset < 0 || $offset >= $count) {
            trigger_error(
                "Cannot remove element at the given index",
                E_USER_ERROR);
            return;
        }
        array_splice($this->container, $offset, 1);
    }

    /**
     * Check the existence of the element at the given index.
     *
     * This will also be called for: isset($arr)
     *
     * @param integer $offset The index of the element to be removed.
     * @return bool True if the element at the given offset exists.
     * @throws \ErrorException Invalid type for index.
     */
    public function offsetExists($offset): bool
    {
        return isset($this->container[$offset]);
    }

    /**
     * @ignore
     */
    public function getIterator(): Traversable
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
    public function count(): int
    {
        return count($this->container);
    }

    public function __debugInfo()
    {
        return array_map(
            function ($item) {
                if ($item instanceof Message || $item instanceof RepeatedField) {
                    return $item->__debugInfo();
                }
                return $item;
            },
            iterator_to_array($this)
        );
    }
}
