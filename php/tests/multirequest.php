<?php

if (extension_loaded("protobuf")) {
    require_once('memory_leak_test.php');
    echo "<p>protobuf loaded</p>";
} else {
    echo "<p>protobuf not loaded</p>";
}
