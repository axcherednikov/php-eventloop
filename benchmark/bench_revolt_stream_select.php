<?php

declare(strict_types=1);

/**
 * Benchmark: Revolt with StreamSelectDriver (default, no extensions required)
 *
 * Usage: php bench_revolt_stream_select.php [iterations]
 */
error_reporting(E_ALL & ~E_DEPRECATED);

ini_set('memory_limit', '512M');

putenv('REVOLT_DRIVER=Revolt\EventLoop\Driver\StreamSelectDriver');

require __DIR__ . '/vendor/autoload.php';

$loop = \Revolt\EventLoop::class;

printf("Driver: %s\n", get_class(\Revolt\EventLoop::getDriver()));

require __DIR__ . '/bench.php';
