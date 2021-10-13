#!/bin/bash

set -ex

# Install composer
curl -sS https://getcomposer.org/installer | php
mkdir -p "$HOME/bin"
mv composer.phar "$HOME/bin/composer"
PATH="$HOME/bin:$PATH"

# go to the repo root
cd $(dirname $0)/../../..

cd php

composer install
composer test
composer test_c
