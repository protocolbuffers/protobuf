<?php

class TestMessage extends \Google\Protobuf\Internal\Message
{
    public static $is_initialized = false;
    private $optional_int32 = 0;
    private $msg = NULL;

    public function __construct() {
        $pool = \Google\Protobuf\Internal\DescriptorPool::getGeneratedPool();
        if (static::$is_initialized == false) {
            $pool->internalAddGeneratedFile(hex2bin(
                "0a560a0a746573742e70726f746f22400a0b546573744d65737361676512" .
                "160a0e6f7074696f6e616c5f696e74333218012001280512190a036d7367" .
                "18022001280b320c2e546573744d657373616765620670726f746f33"
            ));
            static::$is_initialized = true;
        }
        parent::__construct();
    }

    public function setOptionalInt32($var)
    {
        $this->optional_int32 = $var;
    }

    public function getOptionalInt32()
    {
        return $this->optional_int32;
    }

    public function setMsg($var)
    {
        $this->msg = $var;
    }

    public function getMsg()
    {
        return $this->msg;
    }
}

$from = new TestMessage();

assert($from->getOptionalInt32() === 0);
$a = $from->getOptionalInt32();
assert($a === 0);
$from->setOptionalInt32(1);
assert($a === 0);
assert($from->getOptionalInt32() === 1);

$from2 = new TestMessage();
assert($from->getOptionalInt32() === 1);
assert($from2->getOptionalInt32() === 0);

# var_dump($from->getTypeUrl());
# var_dump($from);
# $from->setOptionalInt32(1);
# var_dump($from->getOptionalInt32());
# $sub = new TestMessage();
# $sub->setOptionalInt32(2);
# var_dump($sub->getOptionalInt32());
# $from->setMsg($sub);
# $sub2 = $from->getMsg();
# var_dump($sub2->getOptionalInt32());

# $data = $from->serializeToString();
# var_dump(bin2hex($data));

# $to = new TestMessage();
# $to->mergeFromString($data);
 
# $data = $to->serializeToString();
# var_dump(bin2hex($data));
# 
# var_dump($to->getOptionalInt32());
# $msg = $to->getMsg();
# var_dump($msg);
# var_dump($msg->getOptionalInt32());

var_dump("ok");
