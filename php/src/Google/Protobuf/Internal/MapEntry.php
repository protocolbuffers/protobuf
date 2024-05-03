<?php

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

namespace Google\Protobuf\Internal;

use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\Message;

class MapEntry extends Message
{
    public $key;
    public $value;

    public function __construct($desc) {
        parent::__construct($desc);
        // For MapEntry, getValue should always return a valid value. Thus, we
        // need to create a default instance value if the value type is
        // message, in case no value is provided in data.
        $value_field = $desc->getFieldByNumber(2);
        if ($value_field->getType() == GPBType::MESSAGE) {
            $klass = $value_field->getMessageType()->getClass();
            $value = new $klass;
            $this->setValue($value);
        }
    }

    public function setKey($key) {
      $this->key = $key;
    }

    public function getKey() {
      return $this->key;
    }

    public function setValue($value) {
      $this->value = $value;
    }

    public function getValue() {
      return $this->value;
    }
}
