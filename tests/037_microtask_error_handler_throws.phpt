--TEST--
EventLoop stops when microtask error handler throws
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

EventLoop::setErrorHandler(function (\Throwable $e) {
    echo "handled: " . $e->getMessage() . "\n";
    throw new RuntimeException("handler failed");
});

EventLoop::queue(function () {
    throw new RuntimeException("microtask failed");
});

EventLoop::queue(function () {
    echo "should not run\n";
});

try {
    EventLoop::run();
} catch (RuntimeException $e) {
    echo "caught: " . $e->getMessage() . "\n";
}

EventLoop::setErrorHandler(null);

?>
--EXPECT--
handled: microtask failed
caught: handler failed
