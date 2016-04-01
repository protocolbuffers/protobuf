<?php


namespace Google\Protobuf;

$pool = get_generated_pool();
$pool->addMessage("TestMessage")
    ->optional("optional_int32_a", "int32", 1)
    ->optional("optional_int32_b", "int32", 2)
    ->finalizeToPool()
    ->finalize();

$test_message = new \TestMessage();

?>
