<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
