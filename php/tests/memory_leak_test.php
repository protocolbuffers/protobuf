<?php

# phpunit has memory leak by itself. Thus, it cannot be used to test memory leak.

require_once('generated/NoNamespaceEnum.php');
require_once('generated/NoNamespaceMessage.php');
require_once('generated/NoNamespaceMessage_NestedEnum.php');
require_once('generated/PrefixEmpty.php');
require_once('generated/PrefixTestPrefix.php');
require_once('generated/TestEmptyNamespace.php');
require_once('generated/Bar/TestInclude.php');
require_once('generated/Foo/PBARRAY.php');
require_once('generated/Foo/PBEmpty.php');
require_once('generated/Foo/TestEnum.php');
require_once('generated/Foo/TestIncludeNamespaceMessage.php');
require_once('generated/Foo/TestIncludePrefixMessage.php');
require_once('generated/Foo/TestMessage.php');
require_once('generated/Foo/TestMessage_Empty.php');
require_once('generated/Foo/TestMessage_NestedEnum.php');
require_once('generated/Foo/TestMessage_Sub.php');
require_once('generated/Foo/TestPackedMessage.php');
require_once('generated/Foo/TestPhpDoc.php');
require_once('generated/Foo/TestRandomFieldOrder.php');
require_once('generated/Foo/TestReverseFieldOrder.php');
require_once('generated/Foo/TestUnpackedMessage.php');
require_once('generated/Foo/testLowerCaseMessage.php');
require_once('generated/Foo/testLowerCaseEnum.php');
require_once('generated/GPBMetadata/Proto/Test.php');
require_once('generated/GPBMetadata/Proto/TestEmptyPhpNamespace.php');
require_once('generated/GPBMetadata/Proto/TestInclude.php');
require_once('generated/GPBMetadata/Proto/TestNoNamespace.php');
require_once('generated/GPBMetadata/Proto/TestPhpNamespace.php');
require_once('generated/GPBMetadata/Proto/TestPrefix.php');
require_once('generated/Php/Test/TestNamespace.php');
require_once('test_util.php');

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\GPBType;
use Foo\TestMessage;
use Foo\TestMessage_Sub;

$from = new TestMessage();
TestUtil::setTestMessage($from);
TestUtil::assertTestMessage($from);

$data = $from->serializeToString();

$to = new TestMessage();
$to->mergeFromString($data);

TestUtil::assertTestMessage($to);

$from = new TestMessage();
TestUtil::setTestMessage2($from);

$data = $from->serializeToString();

$to->mergeFromString($data);

// TODO(teboring): This causes following tests fail in php7.
# $from->setRecursive($from);

$arr = new RepeatedField(GPBType::MESSAGE, TestMessage::class);
$arr[] = new TestMessage;
$arr[0]->SetRepeatedRecursive($arr);

// Test oneof fields.
$m = new TestMessage();

$m->setOneofInt32(1);
assert(1 === $m->getOneofInt32());
assert(0.0 === $m->getOneofFloat());
assert('' === $m->getOneofString());
assert(NULL === $m->getOneofMessage());
$data = $m->serializeToString();
$n = new TestMessage();
$n->mergeFromString($data);
assert(1 === $n->getOneofInt32());

$m->setOneofFloat(2.0);
assert(0 === $m->getOneofInt32());
assert(2.0 === $m->getOneofFloat());
assert('' === $m->getOneofString());
assert(NULL === $m->getOneofMessage());
$data = $m->serializeToString();
$n = new TestMessage();
$n->mergeFromString($data);
assert(2.0 === $n->getOneofFloat());

$m->setOneofString('abc');
assert(0 === $m->getOneofInt32());
assert(0.0 === $m->getOneofFloat());
assert('abc' === $m->getOneofString());
assert(NULL === $m->getOneofMessage());
$data = $m->serializeToString();
$n = new TestMessage();
$n->mergeFromString($data);
assert('abc' === $n->getOneofString());

$sub_m = new TestMessage_Sub();
$sub_m->setA(1);
$m->setOneofMessage($sub_m);
assert(0 === $m->getOneofInt32());
assert(0.0 === $m->getOneofFloat());
assert('' === $m->getOneofString());
assert(1 === $m->getOneofMessage()->getA());
$data = $m->serializeToString();
$n = new TestMessage();
$n->mergeFromString($data);
assert(1 === $n->getOneofMessage()->getA());

$m = new TestMessage();
$m->mergeFromString(hex2bin('F80601'));
assert('f80601' === bin2hex($m->serializeToString()));

// Test create repeated field via array.
$str_arr = array("abc");
$m = new TestMessage();
$m->setRepeatedString($str_arr);

// Test create map field via array.
$str_arr = array("abc"=>"abc");
$m = new TestMessage();
$m->setMapStringString($str_arr);

// Test unset
$from = new TestMessage();
TestUtil::setTestMessage($from);
unset($from);

// Test wellknown
$from = new \Google\Protobuf\Timestamp();
$from->setSeconds(1);
assert(1, $from->getSeconds());

$timestamp = new \Google\Protobuf\Timestamp();

date_default_timezone_set('UTC');
$from = new DateTime('2011-01-01T15:03:01.012345UTC');
$timestamp->fromDateTime($from);
assert($from->format('U') == $timestamp->getSeconds());
assert(0 == $timestamp->getNanos());

$to = $timestamp->toDateTime();
assert(\DateTime::class == get_class($to));
assert($from->format('U') == $to->format('U'));

$from = new \Google\Protobuf\Value();
$from->setNumberValue(1);
assert(1, $from->getNumberValue());

// Test discard unknown in message.
$m = new TestMessage();
$from = hex2bin('F80601');
$m->mergeFromString($from);
$m->discardUnknownFields();
$to = $m->serializeToString();
assert("" === bin2hex($to));

// Test clear
$m = new TestMessage();
TestUtil::setTestMessage($m);
$m->clear();

// Test unset map element
$m = new TestMessage();
$map = $m->getMapStringString();
$map[1] = 1;
unset($map[1]);

// Test descriptor
$pool = \Google\Protobuf\DescriptorPool::getGeneratedPool();
$desc = $pool->getDescriptorByClassName("\Foo\TestMessage");
$field = $desc->getField(1);

$from = new TestMessage();
$to = new TestMessage();
TestUtil::setTestMessage($from);
$to->mergeFrom($from);
TestUtil::assertTestMessage($to);
