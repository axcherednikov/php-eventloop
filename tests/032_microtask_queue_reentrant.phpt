--TEST--
EventLoop::queue() preserves order when microtasks queue more microtasks
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

EventLoop::queue(function () {
    echo "A\n";
    EventLoop::queue(function () {
        echo "C\n";
    });
});

EventLoop::queue(function () {
    echo "B\n";
});

EventLoop::defer(function () {
    echo "D\n";
});

EventLoop::run();

?>
--EXPECT--
A
B
C
D
