<?php


use PHPUnit\Framework\TestCase;

class UpdateReservedTest extends TestCase
{
    public function testBashScriptIdempotency()
    {
        $repo_root = dirname(dirname(__DIR__));
        $script_path = $repo_root . '/php/update_reserved_words.sh';

        $this->assertFileExists($script_path);

        $files_to_check = [
            'src/google/protobuf/compiler/php/names.cc',
            'php/ext/google/protobuf/names.c',
            'src/google/protobuf/compiler/php/php_generator.cc',
            'php/tests/proto/test_reserved_message_lower.proto',
            'php/tests/proto/test_reserved_message_upper.proto',
            'php/tests/proto/test_reserved_enum_lower.proto',
            'php/tests/proto/test_reserved_enum_upper.proto',
            'php/tests/proto/test_reserved_enum_value_lower.proto',
            'php/tests/proto/test_reserved_enum_value_upper.proto',
            'php/tests/GeneratedClassTest.php',
        ];

        // Store original contents
        $original_contents = [];
        foreach ($files_to_check as $file) {
            $path = $repo_root . '/' . $file;
            $this->assertFileExists($path);
            $original_contents[$file] = file_get_contents($path);
        }

        try {
            // Run the script (this will modify files in place)
            $output = [];
            $return_var = 0;
            exec("bash " . escapeshellarg($script_path), $output, $return_var);
            $this->assertSame(0, $return_var, "Script failed to execute: " . implode("\n", $output));

            // Compare new contents with original
            $changed_files = [];
            foreach ($files_to_check as $file) {
                $path = $repo_root . '/' . $file;
                $new_content = file_get_contents($path);
                if ($new_content !== $original_contents[$file]) {
                    $changed_files[] = $file;
                }
            }

            $msg = "The following files were modified by the script (idempotency check failed). " .
                   "Please run 'bash php/update_reserved_words.sh' to sync them: " .
                   implode(", ", $changed_files);
            $this->assertEmpty($changed_files, $msg);
        } finally {
            // ALWAYS restore original contents to keep filesystem clean
            foreach ($original_contents as $file => $content) {
                file_put_contents($repo_root . '/' . $file, $content);
            }
        }
    }
}
