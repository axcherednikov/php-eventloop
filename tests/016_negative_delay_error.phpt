--TEST--
EventLoop::delay() rejects negative delay
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

try {
    EventLoop::delay(-1.0, function () {});
} catch (\ValueError $e) {
    echo "Caught: " . $e->getMessage() . "\n";
}

try {
    EventLoop::repeat(-0.5, function () {});
} catch (\ValueError $e) {
    echo "Caught: " . $e->getMessage() . "\n";
}

echo "Done\n";

?>
--EXPECTF--
Caught: EventLoop\EventLoop::delay(): Argument #1 ($delay) must be non-negative, %s given
Caught: EventLoop\EventLoop::repeat(): Argument #1 ($interval) must be non-negative, %s given
Done
