--TEST--
EventLoop::run() throws when already running
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

EventLoop::defer(function () {
    try {
        EventLoop::run();
    } catch (\Error $e) {
        echo "Caught: " . $e->getMessage() . "\n";
    }
});

EventLoop::run();

echo "Done\n";

?>
--EXPECT--
Caught: The event loop is already running
Done
