--TEST--
EventLoop::cancel() inside a running deferred callback is safe
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\InvalidCallbackError;

$id = EventLoop::defer(function (string $callbackId) {
    echo "Running\n";
    EventLoop::cancel($callbackId);
    echo "Cancelled self\n";
});

EventLoop::run();

try {
    EventLoop::isEnabled($id);
} catch (InvalidCallbackError $e) {
    echo "Invalid after run\n";
}

?>
--EXPECT--
Running
Cancelled self
Invalid after run
