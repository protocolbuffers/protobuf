<?php

define("GOOGLE_INTERNAL_NAMESPACE", "Google\\Protobuf\\Internal\\");
define("GOOGLE_NAMESPACE", "Google\\Protobuf\\");
define("GOOGLE_GPBMETADATA_NAMESPACE", "GPBMetadata\\Google\\Protobuf\\Internal\\");

function protobuf_autoloader_impl($class, $prefix) {
    $length = strlen($prefix);
    if ((substr($class, 0, $length) === $prefix)) {
        $path = '../php/src/' . implode('/', array_map('ucwords', explode('\\', $class))) . '.php';
        include_once $path;
    }
}

function protobuf_autoloader($class) {
    protobuf_autoloader_impl($class, GOOGLE_INTERNAL_NAMESPACE);
    protobuf_autoloader_impl($class, GOOGLE_NAMESPACE);
    protobuf_autoloader_impl($class, GOOGLE_GPBMETADATA_NAMESPACE);
}

spl_autoload_register('protobuf_autoloader');
