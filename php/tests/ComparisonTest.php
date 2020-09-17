<?php

use Comparison\Resource;

require_once('test_base.php');
require_once('test_util.php');


class EncodeDecodeTest extends TestBase
{
    private static $testCases = null;

    private static function loadTestCases()
    {
        if (is_null(self::$testCases)) {
            $testSuite = new \Comparison\TestSuite();
            $testSuite->mergeFromJsonString(file_get_contents(sprintf(
                '%s%stest_cases.json',
                dirname(__FILE__),
                DIRECTORY_SEPARATOR
            )));
            self::$testCases = $testSuite->getTestCases();
        }
        return self::$testCases;
    }

    public function fieldMaskCompareData()
    {
        $testData = [];
        foreach (self::loadTestCases() as $testCase) {
            $testData[] = [
                $testCase->getOriginalResource(),
                $testCase->getModifiedResource()
            ];
        }
        return $testData;
    }

    /**
     * @dataProvider fieldMaskCompareData
     */
    public function testFieldMaskCompare(Resource $original, Resource $modified)
    {
        $originalValue = $original->getFoos();
        $size = count($originalValue);
        print "\nLEFT value ($size):";
        foreach ($originalValue as $item) {
            print " " . $item->serializeToJsonString();
        }
        print "\n";

        $modifiedValue = $modified->getFoos();
        $size = count($modifiedValue);
        print "RIGHT value ($size):";
        foreach ($modifiedValue as $item) {
            print " " . $item->serializeToJsonString();
        }
        print "\n";

        if ($originalValue == $modifiedValue) {
            print "=> same\n";
        } else {
            print "=> different\n";
        }
        $this->assertFalse($originalValue == $modifiedValue);
    }
}
