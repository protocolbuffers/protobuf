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
        // In newer versions of PHP, we cannot reference the old class name.
        if (version_compare(phpversion(), '8.1.0', '>=')) return;

        // For older versions of PHP, verify that we can reference the
        // original class name.
        eval('
          $m = new \Previous\readonly();
          $this->assertTrue(true);
        ');
    }
}
