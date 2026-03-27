<?php

declare(strict_types=1);

use EventLoop\EventLoop as PhpLoop;
use Revolt\EventLoop as RevoltLoop;

/**
 * @var class-string<PhpLoop|RevoltLoop> $loop
 */

$iterations = (int) ($argv[1] ?? 100000);

printf("Iterations: %d\n\n", $iterations);

/**
 * 1. defer() dispatch throughput
 */
$count = 0;
$start = hrtime(true);

for ($i = 0; $i < $iterations; $i++) {
    $loop::defer(function () use (&$count) {
        $count++;
    });
}
$loop::run();

$elapsed = (hrtime(true) - $start) / 1e9;
printf("defer(): %d callbacks in %.4fs (%.0f ops/sec)\n", $count, $elapsed, $count / $elapsed);

/**
 * 2. delay(0) dispatch throughput
 */
$count = 0;
$start = hrtime(true);

for ($i = 0; $i < $iterations; $i++) {
    $loop::delay(0, function () use (&$count) {
        $count++;
    });
}
$loop::run();

$elapsed = (hrtime(true) - $start) / 1e9;
printf("delay(0): %d callbacks in %.4fs (%.0f ops/sec)\n", $count, $elapsed, $count / $elapsed);

/**
 * 3. repeat(0) dispatch throughput
 */
$count = 0;
$target = $iterations;
$start = hrtime(true);

$loop::repeat(0, function (string $cbId) use (&$count, $target, $loop) {
    $count++;
    if ($count >= $target) {
        $loop::cancel($cbId);
    }
});
$loop::run();

$elapsed = (hrtime(true) - $start) / 1e9;
printf("repeat(0): %d callbacks in %.4fs (%.0f ops/sec)\n", $count, $elapsed, $count / $elapsed);

/**
 * 4. I/O callback registration + cancel overhead
 */
$pair = stream_socket_pair(STREAM_PF_UNIX, STREAM_SOCK_STREAM, STREAM_IPPROTO_IP);
stream_set_blocking($pair[0], false);

$start = hrtime(true);

for ($i = 0; $i < $iterations; $i++) {
    $cbId = $loop::onReadable($pair[0], function () {
    });

    $loop::cancel($cbId);
}

$elapsed = (hrtime(true) - $start) / 1e9;
printf("I/O reg+cancel: %d ops in %.4fs (%.0f ops/sec)\n", $iterations, $elapsed, $iterations / $elapsed);

fclose($pair[0]);
fclose($pair[1]);

/**
 * 5. Fiber suspension/resume
 */
$count = 0;
$suspensionIterations = min($iterations, 50000);
$start = hrtime(true);

for ($i = 0; $i < $suspensionIterations; $i++) {
    $fiber = new Fiber(function () use (&$count, $loop) {
        $suspension = $loop::getSuspension();
        $suspension->suspend();
        $count++;
    });

    $fiber->start();
    $fiber->resume();
}

$elapsed = (hrtime(true) - $start) / 1e9;
printf("fiber suspend/res: %d ops in %.4fs (%.0f ops/sec)\n", $count, $elapsed, $count / $elapsed);
