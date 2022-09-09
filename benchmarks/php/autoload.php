<?php

define("GOOGLE_INTERNAL_NAMESPACE", "Google\\Protobuf\\Internal\\");
define("GOOGLE_NAMESPACE", "Google\\Protobuf\\");
define("GOOGLE_GPBMETADATA_NAMESPACE", "GPBMetadata\\Google\\Protobuf\\");
define("BENCHMARK_NAMESPACE", "Benchmarks");
define("BENCHMARK_GPBMETADATA_NAMESPACE", "GPBMetadata\\Benchmarks");

function protobuf_autoloader_impl($class, $prefix, $include_path) {
    $length = strlen($prefix);
    if ((substr($class, 0, $length) === $prefix)) {
        $path = $include_path . '/' . implode('/', array_map('ucwords', explode('\\', $class))) . '.php';
        include_once $path;
    }
}

function protobuf_autoloader($class) {
    protobuf_autoloader_impl($class, GOOGLE_INTERNAL_NAMESPACE, getenv('PROTOBUF_PHP_SRCDIR'));
    protobuf_autoloader_impl($class, GOOGLE_NAMESPACE, getenv('PROTOBUF_PHP_SRCDIR'));
    protobuf_autoloader_impl($class, GOOGLE_GPBMETADATA_NAMESPACE, getenv('PROTOBUF_PHP_SRCDIR'));
    protobuf_autoloader_impl($class, BENCHMARK_NAMESPACE, getenv('CURRENT_DIR'));
    protobuf_autoloader_impl($class, BENCHMARK_GPBMETADATA_NAMESPACE, getenv('CURRENT_DIR'));
}

spl_autoload_register('protobuf_autoloader');
