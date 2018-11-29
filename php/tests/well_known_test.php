<?php

require_once('test_base.php');
require_once('test_util.php');

use Foo\TestMessage;
use Google\Protobuf\Any;
use Google\Protobuf\Api;
use Google\Protobuf\BoolValue;
use Google\Protobuf\BytesValue;
use Google\Protobuf\DoubleValue;
use Google\Protobuf\Duration;
use Google\Protobuf\Enum;
use Google\Protobuf\EnumValue;
use Google\Protobuf\Field;
use Google\Protobuf\FieldMask;
use Google\Protobuf\Field\Cardinality;
use Google\Protobuf\Field\Kind;
use Google\Protobuf\FloatValue;
use Google\Protobuf\GPBEmpty;
use Google\Protobuf\Int32Value;
use Google\Protobuf\Int64Value;
use Google\Protobuf\ListValue;
use Google\Protobuf\Method;
use Google\Protobuf\Mixin;
use Google\Protobuf\NullValue;
use Google\Protobuf\Option;
use Google\Protobuf\SourceContext;
use Google\Protobuf\StringValue;
use Google\Protobuf\Struct;
use Google\Protobuf\Syntax;
use Google\Protobuf\Timestamp;
use Google\Protobuf\Type;
use Google\Protobuf\UInt32Value;
use Google\Protobuf\UInt64Value;
use Google\Protobuf\Value;

class NotMessage {}

class WellKnownTest extends TestBase {

    public function testEmpty()
    {
        $msg = new GPBEmpty();
        $this->assertTrue($msg instanceof \Google\Protobuf\Internal\Message);
    }

    public function testImportDescriptorProto()
    {
        $msg = new TestImportDescriptorProto();
    }

    public function testAny()
    {
        // Create embed message
        $embed = new TestMessage();
        $this->setFields($embed);
        $data = $embed->serializeToString();

        // Set any via normal setter.
        $any = new Any();

        $this->assertSame(
            $any, $any->setTypeUrl("type.googleapis.com/foo.TestMessage"));
        $this->assertSame("type.googleapis.com/foo.TestMessage",
                          $any->getTypeUrl());

        $this->assertSame($any, $any->setValue($data));
        $this->assertSame($data, $any->getValue());

        // Test unpack.
        $msg = $any->unpack();
        $this->assertTrue($msg instanceof TestMessage);
        $this->expectFields($msg);

        // Test pack.
        $any = new Any();
        $any->pack($embed);
        $this->assertSame($data, $any->getValue());
        $this->assertSame("type.googleapis.com/foo.TestMessage", $any->getTypeUrl());

        // Test is.
        $this->assertTrue($any->is(TestMessage::class));
        $this->assertFalse($any->is(Any::class));
    }

    /**
     * @expectedException Exception
     */
    public function testAnyUnpackInvalidTypeUrl()
    {
        $any = new Any();
        $any->setTypeUrl("invalid");
        $any->unpack();
    }

    /**
     * @expectedException Exception
     */
    public function testAnyUnpackMessageNotAdded()
    {
        $any = new Any();
        $any->setTypeUrl("type.googleapis.com/MessageNotAdded");
        $any->unpack();
    }

    /**
     * @expectedException Exception
     */
    public function testAnyUnpackDecodeError()
    {
        $any = new Any();
        $any->setTypeUrl("type.googleapis.com/foo.TestMessage");
        $any->setValue("abc");
        $any->unpack();
    }

    public function testApi()
    {
        $m = new Api();

        $m->setName("a");
        $this->assertSame("a", $m->getName());

        $m->setMethods([new Method()]);
        $this->assertSame(1, count($m->getMethods()));

        $m->setOptions([new Option()]);
        $this->assertSame(1, count($m->getOptions()));

        $m->setVersion("a");
        $this->assertSame("a", $m->getVersion());

        $m->setSourceContext(new SourceContext());
        $this->assertFalse(is_null($m->getSourceContext()));

        $m->setMixins([new Mixin()]);
        $this->assertSame(1, count($m->getMixins()));

        $m->setSyntax(Syntax::SYNTAX_PROTO2);
        $this->assertSame(Syntax::SYNTAX_PROTO2, $m->getSyntax());

        $m = new Method();

        $m->setName("a");
        $this->assertSame("a", $m->getName());

        $m->setRequestTypeUrl("a");
        $this->assertSame("a", $m->getRequestTypeUrl());

        $m->setRequestStreaming(true);
        $this->assertSame(true, $m->getRequestStreaming());

        $m->setResponseTypeUrl("a");
        $this->assertSame("a", $m->getResponseTypeUrl());

        $m->setResponseStreaming(true);
        $this->assertSame(true, $m->getResponseStreaming());

        $m->setOptions([new Option()]);
        $this->assertSame(1, count($m->getOptions()));

        $m = new Mixin();

        $m->setName("a");
        $this->assertSame("a", $m->getName());

        $m->setRoot("a");
        $this->assertSame("a", $m->getRoot());
    }

    public function testEnum()
    {
        $m = new Enum();

        $m->setName("a");
        $this->assertSame("a", $m->getName());

        $m->setEnumvalue([new EnumValue()]);
        $this->assertSame(1, count($m->getEnumvalue()));

        $m->setOptions([new Option()]);
        $this->assertSame(1, count($m->getOptions()));

        $m->setSourceContext(new SourceContext());
        $this->assertFalse(is_null($m->getSourceContext()));

        $m->setSyntax(Syntax::SYNTAX_PROTO2);
        $this->assertSame(Syntax::SYNTAX_PROTO2, $m->getSyntax());
    }

    public function testEnumValue()
    {
        $m = new EnumValue();

        $m->setName("a");
        $this->assertSame("a", $m->getName());

        $m->setNumber(1);
        $this->assertSame(1, $m->getNumber());

        $m->setOptions([new Option()]);
        $this->assertSame(1, count($m->getOptions()));
    }

    public function testField()
    {
        $m = new Field();

        $m->setKind(Kind::TYPE_DOUBLE);
        $this->assertSame(Kind::TYPE_DOUBLE, $m->getKind());

        $m->setCardinality(Cardinality::CARDINALITY_OPTIONAL);
        $this->assertSame(Cardinality::CARDINALITY_OPTIONAL, $m->getCardinality());

        $m->setNumber(1);
        $this->assertSame(1, $m->getNumber());

        $m->setName("a");
        $this->assertSame("a", $m->getName());

        $m->setTypeUrl("a");
        $this->assertSame("a", $m->getTypeUrl());

        $m->setOneofIndex(1);
        $this->assertSame(1, $m->getOneofIndex());

        $m->setPacked(true);
        $this->assertSame(true, $m->getPacked());

        $m->setOptions([new Option()]);
        $this->assertSame(1, count($m->getOptions()));

        $m->setJsonName("a");
        $this->assertSame("a", $m->getJsonName());

        $m->setDefaultValue("a");
        $this->assertSame("a", $m->getDefaultValue());
    }

    public function testFieldMask()
    {
        $m = new FieldMask();
        $m->setPaths(["a"]);
        $this->assertSame(1, count($m->getPaths()));
    }

    public function testOption()
    {
        $m = new Option();

        $m->setName("a");
        $this->assertSame("a", $m->getName());

        $m->setValue(new Any());
        $this->assertFalse(is_null($m->getValue()));
    }

    public function testSourceContext()
    {
        $m = new SourceContext();
        $m->setFileName("a");
        $this->assertSame("a", $m->getFileName());
    }

    public function testStruct()
    {
        $m = new ListValue();
        $m->setValues([new Value()]);
        $this->assertSame(1, count($m->getValues()));

        $m = new Value();

        $m->setNullValue(NullValue::NULL_VALUE);
        $this->assertSame(NullValue::NULL_VALUE, $m->getNullValue());
        $this->assertSame("null_value", $m->getKind());

        $m->setNumberValue(1.0);
        $this->assertSame(1.0, $m->getNumberValue());
        $this->assertSame("number_value", $m->getKind());

        $m->setStringValue("a");
        $this->assertSame("a", $m->getStringValue());
        $this->assertSame("string_value", $m->getKind());

        $m->setBoolValue(true);
        $this->assertSame(true, $m->getBoolValue());
        $this->assertSame("bool_value", $m->getKind());

        $m->setStructValue(new Struct());
        $this->assertFalse(is_null($m->getStructValue()));
        $this->assertSame("struct_value", $m->getKind());

        $m->setListValue(new ListValue());
        $this->assertFalse(is_null($m->getListValue()));
        $this->assertSame("list_value", $m->getKind());

        $m = new Struct();
        $m->setFields(array("a"=>new Value()));
        $this->assertSame(1, count($m->getFields()));
    }

    public function testTimestamp()
    {
        $timestamp = new Timestamp();

        $timestamp->setSeconds(1);
        $timestamp->setNanos(2);
        $this->assertEquals(1, $timestamp->getSeconds());
        $this->assertSame(2, $timestamp->getNanos());

        date_default_timezone_set('UTC');
        $from = new DateTime('2011-01-01T15:03:01.012345UTC');
        $timestamp->fromDateTime($from);
        $this->assertEquals($from->format('U'), $timestamp->getSeconds());
        $this->assertEquals(1000 * $from->format('u'), $timestamp->getNanos());

        $to = $timestamp->toDateTime();
        $this->assertSame(\DateTime::class, get_class($to));
        $this->assertSame($from->format('U'), $to->format('U'));
        $this->assertSame($from->format('u'), $to->format('u'));
    }

    public function testType()
    {
        $m = new Type();

        $m->setName("a");
        $this->assertSame("a", $m->getName());

        $m->setFields([new Field()]);
        $this->assertSame(1, count($m->getFields()));

        $m->setOneofs(["a"]);
        $this->assertSame(1, count($m->getOneofs()));

        $m->setOptions([new Option()]);
        $this->assertSame(1, count($m->getOptions()));

        $m->setSourceContext(new SourceContext());
        $this->assertFalse(is_null($m->getSourceContext()));

        $m->setSyntax(Syntax::SYNTAX_PROTO2);
        $this->assertSame(Syntax::SYNTAX_PROTO2, $m->getSyntax());
    }

    public function testDuration()
    {
        $duration = new Duration();

        $duration->setSeconds(1);
        $duration->setNanos(2);
        $this->assertEquals(1, $duration->getSeconds());
        $this->assertSame(2, $duration->getNanos());
    }

    public function testWrappers()
    {
        $m = new DoubleValue();
        $m->setValue(1.0);
        $this->assertSame(1.0, $m->getValue());

        $m = new FloatValue();
        $m->setValue(1.0);
        $this->assertSame(1.0, $m->getValue());

        $m = new Int64Value();
        $m->setValue(1);
        $this->assertEquals(1, $m->getValue());

        $m = new UInt64Value();
        $m->setValue(1);
        $this->assertEquals(1, $m->getValue());

        $m = new Int32Value();
        $m->setValue(1);
        $this->assertSame(1, $m->getValue());

        $m = new UInt32Value();
        $m->setValue(1);
        $this->assertSame(1, $m->getValue());

        $m = new BoolValue();
        $m->setValue(true);
        $this->assertSame(true, $m->getValue());

        $m = new StringValue();
        $m->setValue("a");
        $this->assertSame("a", $m->getValue());

        $m = new BytesValue();
        $m->setValue("a");
        $this->assertSame("a", $m->getValue());
    }

    /**
     * @dataProvider enumNameValueConversionDataProvider
     */
    public function testEnumNameValueConversion($class)
    {
        $reflectionClass = new ReflectionClass($class);
        $constants = $reflectionClass->getConstants();
        foreach ($constants as $k => $v) {
            $this->assertSame($k, $class::name($v));
            $this->assertSame($v, $class::value($k));
        }
    }

    public function enumNameValueConversionDataProvider()
    {
        return [
            ['\Google\Protobuf\Field\Cardinality'],
            ['\Google\Protobuf\Field\Kind'],
            ['\Google\Protobuf\NullValue'],
            ['\Google\Protobuf\Syntax'],
        ];
    }
}
