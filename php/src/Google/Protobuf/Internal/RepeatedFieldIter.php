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
     * @param array $container
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
     * @todo need to add return type void (require update php version to 7.1)
     */
    #[\ReturnTypeWillChange]
    public function rewind()
    {
        $this->position = 0;
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
        return $this->container[$this->position];
    }

    /**
     * Return the current position.
     *
     * @return integer The current position.
     * @todo need to add return type mixed (require update php version to 8.0)
     */
    #[\ReturnTypeWillChange]
    public function key()
    {
        return $this->position;
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
        ++$this->position;
    }

    /**
     * Check whether there are more elements to iterate.
     *
     * @return bool True if there are more elements to iterate.
     */
    public function valid(): bool
    {
        return isset($this->container[$this->position]);
    }
}
