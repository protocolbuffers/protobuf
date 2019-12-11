<?php

if (extension_loaded("protobuf")) {
    echo "<p>protobuf loaded</p>";
    require_once('memory_leak_test.php');
} else {
    echo "<p>protobuf not loaded</p>";
}
