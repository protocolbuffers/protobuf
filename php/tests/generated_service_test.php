<?php

require_once('test_base.php');
require_once('test_util.php');

use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\MapField;
use Google\Protobuf\Internal\GPBType;
use Foo\Greeter;
use Foo\HelloRequest;
use Foo\HelloReply;

class GeneratedServiceTest extends TestBase
{
    /**
     * @var \ReflectionClass
     */
    private $serviceClass;

    /**
     * @var array
     */
    private $methodNames = [
        'sayHello',
        'sayHelloAgain'
    ];

    public function setUp()
    {
        parent::setUp();

        $this->serviceClass = new ReflectionClass('Foo\Greeter');
    }

    public function testIsInterface()
    {
        $this->assertTrue($this->serviceClass->isInterface());
    }

    public function testPhpDocForClass()
    {
        $this->assertContains('foo.Greeter', $this->serviceClass->getDocComment());
    }

    public function testServiceMethodsAreGenerated()
    {
        $this->assertCount(count($this->methodNames), $this->serviceClass->getMethods());
        foreach ($this->methodNames as $methodName) {
            $this->assertTrue($this->serviceClass->hasMethod($methodName));
        }
    }

    public function testPhpDocForServiceMethod()
    {
        foreach ($this->methodNames as $methodName) {
            $docComment = $this->serviceClass->getMethod($methodName)->getDocComment();
            $this->assertContains($methodName, $docComment);
            $this->assertContains('@param HelloRequest $request', $docComment);
            $this->assertContains('@return HelloReply', $docComment);
        }
    }

    public function testParamForServiceMethod()
    {
        foreach ($this->methodNames as $methodName) {
            $method = $this->serviceClass->getMethod($methodName);
            $this->assertCount(1, $method->getParameters());
            $param = $method->getParameters()[0];
            $this->assertFalse($param->isOptional());
            $this->assertSame('request', $param->getName());
            // ReflectionParameter::getType only exists in PHP 7+, so get the type from __toString
            $this->assertContains('Foo\HelloRequest $request', (string) $param);
        }
    }
}
