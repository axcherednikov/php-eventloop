--TEST--
EventLoop error handler catches microtask exceptions
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

EventLoop::setErrorHandler(function (\Throwable $e) {
    echo "handled: " . $e->getMessage() . "\n";
});

EventLoop::queue(function () {
    throw new RuntimeException("microtask failed");
});

EventLoop::queue(function () {
    echo "continued\n";
});

EventLoop::run();
EventLoop::setErrorHandler(null);

echo "done\n";

?>
--EXPECT--
handled: microtask failed
continued
done
