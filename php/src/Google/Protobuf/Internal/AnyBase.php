<?php

namespace Google\Protobuf\Internal;

/**
 * Base class for Google\Protobuf\Any, this contains hand-written convenience
 * methods like pack() and unpack().
 */
class AnyBase extends \Google\Protobuf\Internal\Message
{
    const TYPE_URL_PREFIX = 'type.googleapis.com/';

    /**
     * This method will try to resolve the type_url in Any message to get the
     * targeted message type. If failed, an error will be thrown. Otherwise,
     * the method will create a message of the targeted type and fill it with
     * the decoded value in Any.
     * @return Message unpacked message
     * @throws \Exception Type url needs to be type.googleapis.com/fully-qualified.
     * @throws \Exception Class hasn't been added to descriptor pool.
     * @throws \Exception cannot decode data in value field.
     */
    public function unpack()
    {
        // Get fully qualified name from type url.
        $url_prifix_len = strlen(GPBUtil::TYPE_URL_PREFIX);
        if (substr($this->type_url, 0, $url_prifix_len) !=
                GPBUtil::TYPE_URL_PREFIX) {
            throw new \Exception(
                "Type url needs to be type.googleapis.com/fully-qulified");
        }
        $fully_qualified_name =
            substr($this->type_url, $url_prifix_len);

        // Create message according to fully qualified name.
        $pool = \Google\Protobuf\Internal\DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByProtoName($fully_qualified_name);
        if (is_null($desc)) {
            throw new \Exception("Class ".$fully_qualified_name
                                     ." hasn't been added to descriptor pool");
        }
        $klass = $desc->getClass();
        $msg = new $klass();

        // Merge data into message.
        $msg->mergeFromString($this->value);
        return $msg;
    }

    /**
     * The type_url will be created according to the given message’s type and
     * the value is encoded data from the given message..
     * @param Message $msg A proto message.
     */
    public function pack($msg)
    {
        if (!$msg instanceof Message) {
            trigger_error("Given parameter is not a message instance.",
                          E_USER_ERROR);
            return;
        }

        // Set value using serialized message.
        $this->value = $msg->serializeToString();

        // Set type url.
        $pool = \Google\Protobuf\Internal\DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName(get_class($msg));
        $fully_qualified_name = $desc->getFullName();
        $this->type_url = GPBUtil::TYPE_URL_PREFIX . $fully_qualified_name;
    }

    /**
     * This method returns whether the type_url in any_message is corresponded
     * to the given class.
     * @param string $klass The fully qualified PHP class name of a proto message type.
     */
    public function is($klass)
    {
        $pool = \Google\Protobuf\Internal\DescriptorPool::getGeneratedPool();
        $desc = $pool->getDescriptorByClassName($klass);
        $fully_qualified_name = $desc->getFullName();
        $type_url = GPBUtil::TYPE_URL_PREFIX . $fully_qualified_name;
        return $this->type_url === $type_url;
    }
}
