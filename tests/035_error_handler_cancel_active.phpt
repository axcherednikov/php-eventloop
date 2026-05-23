--TEST--
Error handler can cancel an active repeating callback safely
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;
use EventLoop\InvalidCallbackError;

$id = null;

EventLoop::setErrorHandler(function (\Throwable $e) use (&$id) {
    echo "handled: " . $e->getMessage() . "\n";
    EventLoop::cancel($id);
    EventLoop::stop();
});

$id = EventLoop::repeat(0, function () {
    throw new RuntimeException("boom");
});

EventLoop::run();
EventLoop::setErrorHandler(null);

try {
    EventLoop::isEnabled($id);
} catch (InvalidCallbackError) {
    echo "repeat cancelled\n";
}

?>
--EXPECT--
handled: boom
repeat cancelled
