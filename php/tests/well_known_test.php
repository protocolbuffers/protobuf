<?php

use Google\Protobuf\GPBEmpty;

class WellKnownTest extends PHPUnit_Framework_TestCase {

    public function testNone() {
      $msg = new GPBEmpty();
    }

}
