<?php

// We have to test this because the command-line argument will fail silently
// if the extension could not be loaded:
//    php -dextension=ext/google/protobuf/modules/protouf.so
if (!extension_loaded("protobuf")) {
    throw new Exception("Protobuf extension not loaded");
}

spl_autoload_register(function($class) {
    if (strpos($class, "Google\\Protobuf") === 0) {
        throw new Exception("When using the C extension, we should not load runtime class: " . $class);
    }
}, true, true);
