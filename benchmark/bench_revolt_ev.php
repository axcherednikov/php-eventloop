<?php

declare(strict_types=1);

/**
 * Benchmark: Revolt with EvDriver (requires ev)
 *
 * Usage: php bench_revolt_ev.php [iterations]
 */
error_reporting(E_ALL & ~E_DEPRECATED);

ini_set('memory_limit', '512M');

putenv('REVOLT_DRIVER=Revolt\EventLoop\Driver\EvDriver');

require __DIR__ . '/vendor/autoload.php';

$loop = \Revolt\EventLoop::class;

printf("Driver: %s\n", get_class(\Revolt\EventLoop::getDriver()));

require __DIR__ . '/bench.php';
