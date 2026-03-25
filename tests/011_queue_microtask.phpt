--TEST--
EventLoop::queue() executes microtasks with arguments
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

EventLoop::queue(function (string $a, int $b) {
    echo "Microtask: $a, $b\n";
}, "hello", 42);

EventLoop::queue(function () {
    echo "Microtask without args\n";
});

EventLoop::defer(function () {
    echo "Defer runs after microtasks\n";
});

EventLoop::run();

?>
--EXPECT--
Microtask: hello, 42
Microtask without args
Defer runs after microtasks
