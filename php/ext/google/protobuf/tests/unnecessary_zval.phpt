--TEST--
unnecessary zval
--FILE--
<?php
var_dump(new \stdClass());
?>
--EXPECT--
object(stdClass)#1 (0) {
}
