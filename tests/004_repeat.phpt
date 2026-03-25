--TEST--
EventLoop::repeat() executes multiple times
--EXTENSIONS--
eventloop
--FILE--
<?php

use EventLoop\EventLoop;

$count = 0;

$id = EventLoop::repeat(0.01, function (string $callbackId) use (&$count) {
    $count++;
    echo "Repeat $count\n";
    if ($count >= 3) {
        EventLoop::cancel($callbackId);
    }
});

EventLoop::run();

echo "Total: $count\n";

?>
--EXPECT--
Repeat 1
Repeat 2
Repeat 3
Total: 3
