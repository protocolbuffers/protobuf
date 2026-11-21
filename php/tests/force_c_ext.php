<?php

// We have to test this because the command-line argument will fail silently
// if the extension could not be loaded:
//    php -dextension=ext/google/protobuf/modules/protouf.so
if (!extension_loaded("protobuf")) {
    throw new Exception("Protobuf extension not loaded");
}

spl_autoload_register(function($class) {
    if (strpos($class, "Google\\Protobuf") === 0) {
        // Public descriptor.proto classes are intentionally served via PSR-4
        // rather than registered at C-extension startup (wkt.inc).
        if (strpos($class, "Google\\Protobuf\\Internal\\") !== 0) {
            $file = __DIR__ . '/../src/' . str_replace('\\', '/', $class) . '.php';
            if (is_file($file) && strpos(file_get_contents($file), '# source: google/protobuf/descriptor.proto') !== false) {
                return;
            }
        }
        throw new Exception("When using the C extension, we should not load runtime class: " . $class);
    }
}, true, true);
