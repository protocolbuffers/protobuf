<?php

error_reporting(E_ALL);

function getGeneratedFiles($dir, &$results = array())
{
    $files = scandir($dir);

    foreach ($files as $key => $value) {
        $path = realpath($dir.DIRECTORY_SEPARATOR.$value);
        if (!is_dir($path)) {
            $results[] = $path;
        } else if ($value != "." && $value != "..") {
            getGeneratedFiles($path, $results);
        }
    }
    return $results;
}

foreach (getGeneratedFiles("generated") as $filename)
{
    if (!is_dir($filename)) {
        include_once $filename;
    }

}

