<?php

require_once('test_base.php');
require_once('test_util.php');

class ComposerJsonTest extends TestBase
{
    public function testComposerJsonDistIsUpToDate()
    {
        $composerJson = json_decode(file_get_contents(__DIR__ . '/../composer.json'), true);
        $composerJsonDist = json_decode(file_get_contents(__DIR__ . '/../composer.json.dist'), true);

        // We don't want the subrepo to have "scripts" or "config"
        unset(
            $composerJson['scripts'],
            $composerJson['config'],
            $composerJson['autoload-dev']
        );

        $this->assertEquals($composerJson, $composerJsonDist);
    }
}
