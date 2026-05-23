--TEST--
EventLoop::cancel() inside a running delay callback is safe
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\InvalidCallbackError;

$id = EventLoop::delay(0, function (string $callbackId) {
    echo "delay running\n";
    EventLoop::cancel($callbackId);
    echo "delay cancelled self\n";
});

EventLoop::run();

try {
    EventLoop::isEnabled($id);
} catch (InvalidCallbackError) {
    echo "invalid after run\n";
}

?>
--EXPECT--
delay running
delay cancelled self
invalid after run
