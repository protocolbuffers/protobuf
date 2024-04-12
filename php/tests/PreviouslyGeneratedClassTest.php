<?php

require_once('test_base.php');
require_once('test_util.php');

class PreviouslyGeneratedClassTest extends TestBase
{
    #########################################################
    # Test compatibility for previously unreserved words.
    #########################################################

    public function testPrefixForReservedWords()
    {
        $m = new \Previous\readonly();

        $this->assertTrue(true);
    }
}
