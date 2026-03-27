<?php

declare(strict_types=1);

use EventLoop\EventLoop;

ini_set('memory_limit', '512M');

$loop = EventLoop::class;

printf("Driver: %s\n", EventLoop::getDriver());

require __DIR__ . '/bench.php';
