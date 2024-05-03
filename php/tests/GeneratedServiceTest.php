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
     * @var \ReflectionClass
     */
    private $namespacedServiceClass;

    /**
     * @var array
     */
    private $methodNames = [
        'sayHello',
        'sayHelloAgain'
    ];

    /**
     * Avoid calling setUp, which has void return type (not avalialbe in php7.0).
     *
     * @before
     */
    public function setUpTest()
    {
        $this->serviceClass = new ReflectionClass('Foo\GreeterInterface');

        $this->namespacedServiceClass = new ReflectionClass('Bar\OtherGreeterInterface');
    }

    public function testIsInterface()
    {
        $this->assertTrue($this->serviceClass->isInterface());
    }

    public function testPhpDocForClass()
    {
        $this->assertStringContains(
            'foo.Greeter', $this->serviceClass->getDocComment());
    }

    public function testPhpDocForNamespacedClass()
    {
        $this->assertStringContains(
            'foo.OtherGreeter', $this->namespacedServiceClass->getDocComment());
    }

    public function testServiceMethodsAreGenerated()
    {
        $this->assertCount(
            count($this->methodNames), $this->serviceClass->getMethods());
        foreach ($this->methodNames as $methodName) {
            $this->assertTrue($this->serviceClass->hasMethod($methodName));
        }
    }

    public function testPhpDocForServiceMethod()
    {
        foreach ($this->methodNames as $methodName) {
            $docComment =
                $this->serviceClass->getMethod($methodName)->getDocComment();
            $this->assertStringContains($methodName, $docComment);
            $this->assertStringContains(
                '@param \Foo\HelloRequest $request', $docComment);
            $this->assertStringContains(
                '@return \Foo\HelloReply', $docComment);
        }
    }

    public function testPhpDocForServiceMethodInNamespacedClass()
    {
        foreach ($this->methodNames as $methodName) {
            $docComment =
                $this->namespacedServiceClass->getMethod(
                    $methodName)->getDocComment();
            $this->assertStringContains($methodName, $docComment);
            $this->assertStringContains(
                '@param \Foo\HelloRequest $request', $docComment);
            $this->assertStringContains(
                '@return \Foo\HelloReply', $docComment);
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
	    // ReflectionParameter::getType only exists in PHP 7+, so get the
	    // type from __toString
            $this->assertStringContains(
                'Foo\HelloRequest $request', (string) $param);
        }
    }

    public function testParamForServiceMethodInNamespacedClass()
    {
        foreach ($this->methodNames as $methodName) {
            $method = $this->serviceClass->getMethod($methodName);
            $this->assertCount(1, $method->getParameters());
            $param = $method->getParameters()[0];
            $this->assertFalse($param->isOptional());
            $this->assertSame('request', $param->getName());
	    // ReflectionParameter::getType only exists in PHP 7+, so get the
	    // type from __toString
            $this->assertStringContains(
                'Foo\HelloRequest $request', (string) $param);
        }
    }
}
