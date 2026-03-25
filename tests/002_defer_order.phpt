--TEST--
EventLoop::defer() preserves execution order
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$order = [];

EventLoop::defer(function () use (&$order) {
    $order[] = 1;
});

EventLoop::defer(function () use (&$order) {
    $order[] = 2;
});

EventLoop::defer(function () use (&$order) {
    $order[] = 3;
});

EventLoop::run();

echo implode(',', $order) . "\n";

?>
--EXPECT--
1,2,3
