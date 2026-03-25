--TEST--
EventLoop::delay() with zero delay executes immediately
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$order = [];

EventLoop::delay(0, function () use (&$order) {
    $order[] = 'delay-0';
});

EventLoop::defer(function () use (&$order) {
    $order[] = 'defer';
});

EventLoop::run();

// Defer should execute before delay(0) since deferred runs first in the tick
echo implode(',', $order) . "\n";

?>
--EXPECT--
defer,delay-0
