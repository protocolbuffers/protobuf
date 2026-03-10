<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf;

class DescriptorPool
{
    private static $pool;

    private $internal_pool;

    /**
     * @return DescriptorPool
     */
    public static function getGeneratedPool()
    {
        if (!isset(self::$pool)) {
            self::$pool = new DescriptorPool(\Google\Protobuf\Internal\DescriptorPool::getGeneratedPool());
        }
        return self::$pool;
    }

    private function __construct($internal_pool)
    {
        $this->internal_pool = $internal_pool;
    }

    /**
     * @param string $className A fully qualified protobuf class name
     * @return Descriptor
     */
    public function getDescriptorByClassName($className)
    {
        $desc = $this->internal_pool->getDescriptorByClassName($className);
        return is_null($desc) ? null : $desc->getPublicDescriptor();
    }

    /**
     * @param string $className A fully qualified protobuf class name
     * @return EnumDescriptor
     */
    public function getEnumDescriptorByClassName($className)
    {
        $desc = $this->internal_pool->getEnumDescriptorByClassName($className);
        return is_null($desc) ? null : $desc->getPublicDescriptor();
    }
}
