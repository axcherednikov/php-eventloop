<?php

declare(strict_types=1);

/**
 * Benchmark: Revolt with UvDriver (requires uv)
 *
 * Usage: php bench_revolt_uv.php [iterations]
 */
error_reporting(E_ALL & ~E_DEPRECATED);

ini_set('memory_limit', '512M');

putenv('REVOLT_DRIVER=Revolt\EventLoop\Driver\UvDriver');

require __DIR__ . '/vendor/autoload.php';

$loop = \Revolt\EventLoop::class;

printf("Driver: %s\n", get_class(\Revolt\EventLoop::getDriver()));

require __DIR__ . '/bench.php';
