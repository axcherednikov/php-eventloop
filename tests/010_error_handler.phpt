--TEST--
EventLoop error handler catches callback exceptions
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

EventLoop::setErrorHandler(function (Throwable $e) {
    echo "Caught: " . $e->getMessage() . "\n";
});

echo "Has handler: " . (EventLoop::getErrorHandler() !== null ? "yes" : "no") . "\n";

EventLoop::defer(function () {
    throw new RuntimeException("test error");
});

EventLoop::defer(function () {
    echo "Second callback runs\n";
});

EventLoop::run();

echo "Loop completed\n";

// Reset
EventLoop::setErrorHandler(null);
echo "Handler cleared: " . (EventLoop::getErrorHandler() === null ? "yes" : "no") . "\n";

?>
--EXPECT--
Has handler: yes
Caught: test error
Second callback runs
Loop completed
Handler cleared: yes
