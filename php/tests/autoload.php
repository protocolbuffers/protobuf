<?php

$autoloadPath = __DIR__ . '/../vendor/autoload.php';

if (!file_exists($autoloadPath)) {
    throw new RuntimeException('Composer autoload.php not found. Please run "composer install" first.');
}

// If the FORCE_C_EXT environment variable is set, we will force the use of the C extension.
if (getenv('FORCE_C_EXT')) {
    require_once __DIR__ . '/force_c_ext.php';
}

return require $autoloadPath;
